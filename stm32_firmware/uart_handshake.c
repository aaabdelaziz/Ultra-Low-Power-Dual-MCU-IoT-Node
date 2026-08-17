#include "uart_handshake.h"
#include <stddef.h>

void uart_handshake_send(const uint16_t* samples, size_t n) {
    (void)samples; (void)n;
    // Perform handshake with ESP32 and send framed telemetry over UART
}
#include "uart_handshake.h"
#include <stdint.h>
#include <stdio.h>

void uart_handshake_init(void)
{
    // configure UART pins and baud rate
}

void uart_wake_esp32(void)
{
    // optionally toggle GPIO or send UART break to wake ESP32
}

void uart_send_frame_and_wait_ack(const telemetry_payload_t *payload)
{
    // Build frame and transmit. Wait for ACK (0x06) with timeout.
    (void)payload;
}
