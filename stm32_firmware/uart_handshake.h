#pragma once
#include <stddef.h>
void uart_handshake_send(const uint16_t* samples, size_t n);
#pragma once
#include "dma_adc.h"

void uart_handshake_init(void);
void uart_wake_esp32(void);
void uart_send_frame_and_wait_ack(const telemetry_payload_t *payload);
