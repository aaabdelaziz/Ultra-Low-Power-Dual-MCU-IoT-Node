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

/*
 * Configures USART1 for the STM32 link and stores the output queue.
 * See uart_receiver.h for the full contract; this is the implementation.
 */
void uart_receiver_init(QueueHandle_t data_queue) {
    // Store the queue handle now so uart_receiver_task() (started later,
    // by main.cpp, after this function returns) has somewhere to push
    // validated frames as soon as it begins running.
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

    // Install the UART driver's own RX ring buffer (2x BUF_SIZE, no TX
    // ring buffer / event queue needed since transmits here are tiny
    // single-byte ACK/NACK responses sent via send_response()), apply the
    // 115200-8N1 config above, then route TX/RX to the fixed pins this
    // board uses (UART_PIN_NO_CHANGE for RTS/CTS: flow control is unused).
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

/*
 * Sends a single-byte ACK (PROTOCOL_ACK_BYTE) or NACK (PROTOCOL_NACK_BYTE)
 * back to the STM32 over the same UART link, once a frame has been fully
 * received and its CRC checked. This is the ESP32 side's half of the
 * handshake that stm32_firmware/middleware/uart_handshake.c's
 * uart_send_telemetry_and_wait_ack() blocks on.
 */
static void send_response(uint8_t response_byte) {
    uart_write_bytes(UART_NUM, (const char *)&response_byte, 1);
}

/*
 * FreeRTOS task: reads raw UART bytes and runs them through a byte-at-a-
 * time state machine that reassembles the STM32's frame format defined in
 * common/protocol_spec.h (uart_frame_t: sync1, sync2, 16-bit length,
 * telemetry_payload_t payload, 16-bit CRC), all sent little-endian byte-
 * by-byte hence the split _L/_H states per multi-byte field.
 *
 * On a length or CRC mismatch the state machine just resyncs (returns to
 * STATE_SYNC1) rather than erroring out - a corrupted frame is simply
 * discarded and the next DMA-triggered STM32 transmission (see
 * app/src/main.c on the STM32 side) tries again with fresh data, so no
 * retry logic is needed here.
 */
void uart_receiver_task(void *pvParameters) {
    // Scratch buffer for each uart_read_bytes() call; reused across the
    // whole task lifetime rather than allocated per read.
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    // One state per field of uart_frame_t, in wire order. LEN/CRC are
    // 16-bit and arrive as two separate bytes (low byte, then high byte),
    // hence the paired _L/_H states.
    enum {
        STATE_SYNC1,
        STATE_SYNC2,
        STATE_LEN_L,
        STATE_LEN_H,
        STATE_PAYLOAD,
        STATE_CRC_L,
        STATE_CRC_H
    } state = STATE_SYNC1;

    uint16_t expected_length = 0;   // length field read out of the frame
    uint16_t payload_idx = 0;       // write cursor into payload_buf while in STATE_PAYLOAD
    uint16_t received_crc = 0;      // CRC field read out of the frame
    telemetry_payload_t payload_buf;
    // Byte-wise view of payload_buf so the state machine can fill it one
    // byte at a time without needing a separate raw byte buffer.
    uint8_t *payload_ptr = (uint8_t*)&payload_buf;

    while (1) {
        // Short 20-tick (~20ms at 1kHz tick rate) timeout: this task must
        // return to the outer while(1) regularly rather than block
        // indefinitely on UART, so the state machine keeps making
        // progress even during quiet periods between STM32 transmissions.
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        for (int i = 0; i < len; i++) {
            uint8_t b = data[i];
            switch (state) {
                case STATE_SYNC1:
                    // Look for the first sync byte anywhere in the stream;
                    // any other byte is noise/mid-frame garbage and is
                    // simply dropped while staying in this state.
                    if (b == PROTOCOL_SYNC_BYTE_1) state = STATE_SYNC2;
                    break;
                case STATE_SYNC2:
                    // Second sync byte must immediately follow the first;
                    // if it doesn't, this wasn't really a frame start, so
                    // fall back to hunting for SYNC1 again.
                    if (b == PROTOCOL_SYNC_BYTE_2) state = STATE_LEN_L;
                    else state = STATE_SYNC1;
                    break;
                case STATE_LEN_L:
                    // Low byte of the little-endian 16-bit length field.
                    expected_length = b;
                    state = STATE_LEN_H;
                    break;
                case STATE_LEN_H:
                    // High byte completes the length field. Only proceed
                    // if it exactly matches this firmware's known payload
                    // size - the protocol currently carries one fixed
                    // struct type, so any other length means the frame is
                    // malformed or desynchronized, not a different valid
                    // payload variant.
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
                    // Copy each payload byte in as it arrives; once
                    // expected_length bytes have been collected, the
                    // payload is complete and the CRC field follows.
                    payload_ptr[payload_idx++] = b;
                    if (payload_idx >= expected_length) state = STATE_CRC_L;
                    break;
                case STATE_CRC_L:
                    // Low byte of the little-endian 16-bit CRC field.
                    received_crc = b;
                    state = STATE_CRC_H;
                    break;
                case STATE_CRC_H: {
                    // High byte completes the CRC field - the frame is
                    // now fully received. Recompute the CRC over the
                    // payload we just collected and compare against what
                    // the STM32 sent, using the same calculate_crc16()
                    // both sides share (common/protocol_spec.h).
                    received_crc |= (b << 8);

                    uint16_t calc_crc = calculate_crc16(payload_ptr, expected_length);
                    if (calc_crc == received_crc) {
                        ESP_LOGI(TAG, "Valid frame received");
                        // ACK first so the STM32's blocking wait
                        // (uart_send_telemetry_and_wait_ack()) can return
                        // promptly and go back to sleep; the queue push
                        // that follows only affects this ESP32's own
                        // internal handoff to the MQTT publisher.
                        send_response(PROTOCOL_ACK_BYTE);

                        if (out_queue != NULL) {
                            // Blocks forever (portMAX_DELAY) if the queue
                            // is full; acceptable here since the queue is
                            // sized (10 entries, see main.cpp) well above
                            // the rate telemetry actually arrives at, so
                            // this should not stall in practice.
                            xQueueSend(out_queue, &payload_buf, portMAX_DELAY);
                        }
                    } else {
                        // Corrupted frame: NACK it and drop it. No local
                        // retry - the STM32's caller treats a NACK/timeout
                        // as "try again next cycle" rather than resending
                        // immediately (see app/src/main.c on the STM32
                        // side).
                        ESP_LOGE(TAG, "CRC mismatch (Calc: %04X, Recv: %04X)", calc_crc, received_crc);
                        send_response(PROTOCOL_NACK_BYTE);
                    }
                    // Whether ACKed or NACKed, go back to hunting for the
                    // next frame's sync bytes.
                    state = STATE_SYNC1;
                    break;
                }
            }
        }
    }

    // Unreachable under normal operation: the while(1) above never
    // breaks, so this cleanup path only exists for symmetry/documentation
    // of what tearing this task down would require, not because it
    // currently runs.
    free(data);
    vTaskDelete(NULL);
}
