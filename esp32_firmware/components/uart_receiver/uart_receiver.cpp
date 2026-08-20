/*
 * uart_receiver.cpp
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include "uart_receiver.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UART_RX";

#define UART_NUM UART_NUM_1
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (1024)

static QueueHandle_t out_queue;

void uart_receiver_init(QueueHandle_t data_queue) {
    out_queue = data_queue;
    
    /* Zero-initialize first, then assign field-by-field. A partial
     * designated initializer (`= { .baud_rate = ..., ... }`) leaves any
     * struct member this code doesn't name - like uart_config_t's
     * rx_flow_ctrl_thresh/flags added in newer ESP-IDF versions - as an
     * implicit zero too, but GCC's -Wmissing-field-initializers (an error
     * here, not just a warning, in this C++ build) flags that as
     * suspicious. Explicit `= {}` plus assignments says the same thing
     * without tripping the check, and won't need editing again if the
     * struct grows further. */
    uart_config_t uart_config = {};
    uart_config.baud_rate  = 115200;
    uart_config.data_bits  = UART_DATA_8_BITS;
    uart_config.parity     = UART_PARITY_DISABLE;
    uart_config.stop_bits  = UART_STOP_BITS_1;
    uart_config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

// Sends an ACK or NACK back to the STM32
static void send_response(uint8_t response_byte) {
    uart_write_bytes(UART_NUM, (const char *)&response_byte, 1);
}

void uart_receiver_task(void *pvParameters) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    
    // State machine for parsing the incoming stream
    enum {
        STATE_SYNC1,
        STATE_SYNC2,
        STATE_LEN_L,
        STATE_LEN_H,
        STATE_PAYLOAD,
        STATE_CRC_L,
        STATE_CRC_H
    } state = STATE_SYNC1;
    
    uint16_t expected_length = 0;
    uint16_t payload_idx = 0;
    uint16_t received_crc = 0;
    telemetry_payload_t payload_buf;
    uint8_t *payload_ptr = (uint8_t*)&payload_buf;
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        for (int i = 0; i < len; i++) {
            uint8_t b = data[i];
            switch (state) {
                case STATE_SYNC1:
                    if (b == PROTOCOL_SYNC_BYTE_1) state = STATE_SYNC2;
                    break;
                case STATE_SYNC2:
                    if (b == PROTOCOL_SYNC_BYTE_2) state = STATE_LEN_L;
                    else state = STATE_SYNC1;
                    break;
                case STATE_LEN_L:
                    expected_length = b;
                    state = STATE_LEN_H;
                    break;
                case STATE_LEN_H:
                    expected_length |= (b << 8);
                    if (expected_length == sizeof(telemetry_payload_t)) {
                        state = STATE_PAYLOAD;
                        payload_idx = 0;
                    } else {
                        ESP_LOGW(TAG, "Invalid payload length");
                        state = STATE_SYNC1;
                    }
                    break;
                case STATE_PAYLOAD:
                    payload_ptr[payload_idx++] = b;
                    if (payload_idx >= expected_length) state = STATE_CRC_L;
                    break;
                case STATE_CRC_L:
                    received_crc = b;
                    state = STATE_CRC_H;
                    break;
                case STATE_CRC_H:
                    received_crc |= (b << 8);
                    
                    uint16_t calc_crc = calculate_crc16(payload_ptr, expected_length);
                    if (calc_crc == received_crc) {
                        ESP_LOGI(TAG, "Valid frame received");
                        send_response(PROTOCOL_ACK_BYTE);
                        
                        if (out_queue != NULL) {
                            xQueueSend(out_queue, &payload_buf, portMAX_DELAY);
                        }
                    } else {
                        ESP_LOGE(TAG, "CRC mismatch (Calc: %04X, Recv: %04X)", calc_crc, received_crc);
                        send_response(PROTOCOL_NACK_BYTE);
                    }
                    state = STATE_SYNC1;
                    break;
            }
        }
    }
    
    free(data);
    vTaskDelete(NULL);
}
