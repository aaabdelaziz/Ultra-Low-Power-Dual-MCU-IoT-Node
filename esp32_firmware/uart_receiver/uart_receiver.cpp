#include "uart_receiver.h"
#include <cstring>
#include <queue>

// Minimal state machine constants
static const uint8_t START_BYTE = 0x7E;

void uart_receiver_init(void) {
    // Initialize UART peripheral (platform-specific). Placeholder.
}

void uart_receiver_task(void* arg) {
    // Example pseudo-logic: read bytes, assemble frames, validate CRC, enqueue telemetry.
    (void)arg;
    uint8_t state = 0;
    uint8_t len = 0;
    uint8_t buf[128];
    size_t idx = 0;

    while (1) {
        // Replace with actual UART read
        // int b = uart_read_timeout();
        // if (b < 0) continue;

        // Pseudo-state machine
        // if (state == 0) { if (b == START_BYTE) state = 1; }
        // else if (state == 1) { len = b; idx = 0; state = 2; }
        // else if (state == 2) { buf[idx++] = b; if (idx == len) { validate_and_queue(buf, len); state = 0; } }

        // For now sleep to avoid busy loop (replace with vTaskDelay in FreeRTOS)
        break;
    }
}
#include "uart_receiver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <cstring>

static QueueHandle_t g_out_queue = nullptr;

enum ParseState { SYNC1, SYNC2, LEN, PAYLOAD, CRC1, CRC2 };

static void uart_receiver_task(void* arg)
{
    uint8_t buf[512];
    ParseState state = SYNC1;
    uint8_t len = 0;
    uint16_t crc = 0;
    uint8_t payload[256];
    size_t payload_pos = 0;

    while (1) {
        int len_read = uart_read_bytes(UART_NUM_1, buf, sizeof(buf), pdMS_TO_TICKS(1000));
        if (len_read <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        for (int i = 0; i < len_read; ++i) {
            uint8_t b = buf[i];
            switch (state) {
                case SYNC1: if (b == 0xAA) state = SYNC2; break;
                case SYNC2: if (b == 0x55) state = LEN; else state = SYNC1; break;
                case LEN: len = b; payload_pos = 0; state = (len == 0 ? CRC1 : PAYLOAD); break;
                case PAYLOAD:
                    payload[payload_pos++] = b;
                    if (payload_pos >= len) state = CRC1;
                    break;
                case CRC1: crc = b; state = CRC2; break;
                case CRC2: crc |= (uint16_t)b << 8;
                    // TODO: validate CRC — for now assume valid
                    // Send ACK back
                    uart_write_bytes(UART_NUM_1, (const char*)"\x06", 1);
                    if (g_out_queue) {
                        // push payload copy to queue
                        uint8_t *pkt = (uint8_t*)malloc(len);
                        if (pkt) {
                            memcpy(pkt, payload, len);
                            xQueueSend(g_out_queue, &pkt, 0);
                        }
                    }
                    state = SYNC1;
                    break;
            }
        }
    }
}

void uart_receiver_init(QueueHandle_t out_queue)
{
    g_out_queue = out_queue;

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);

    xTaskCreate(uart_receiver_task, "uart_receiver", 4096, NULL, 10, NULL);
}
