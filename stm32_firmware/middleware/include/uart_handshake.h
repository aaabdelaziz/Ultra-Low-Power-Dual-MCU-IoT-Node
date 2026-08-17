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
 * @brief Wakes up the ESP32 (via GPIO toggle or UART break).
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
