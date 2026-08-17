/*
 * uart_receiver.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "protocol_spec.h"

// Initialize the UART peripheral
void uart_receiver_init(QueueHandle_t data_queue);

// FreeRTOS Task for receiving and parsing UART data
void uart_receiver_task(void *pvParameters);

#endif // UART_RECEIVER_H
