/*
 * uart_handshake.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef UART_HANDSHAKE_H
#define UART_HANDSHAKE_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol_spec.h"

/**
 * @brief Initializes the UART peripheral for communication with ESP32.
 */
void uart_handshake_init(void);

/**
 * @brief Wakes the ESP32 out of deep/modem sleep with a >=10ms low-to-high
 * GPIO pulse on WAKE_PIN (see board_config.h), matching the RTC GPIO wake
 * source configured on the ESP32 side. Blocks for the pulse duration plus
 * a fixed settle delay, so the ESP32's own UART peripheral is up before
 * the caller starts sending frame bytes.
 */
void uart_wake_esp32(void);

/**
 * @brief Sends a telemetry payload frame to the ESP32.
 * 
 * @param payload Pointer to the telemetry payload struct
 * @return true if ACK received, false on timeout/NACK
 */
bool uart_send_telemetry_and_wait_ack(const telemetry_payload_t *payload);

#endif // UART_HANDSHAKE_H
