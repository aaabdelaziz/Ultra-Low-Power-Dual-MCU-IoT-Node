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

/**
 * @brief Configures USART1 (pins/baud/driver install) for the STM32 link
 * and records the queue that validated telemetry frames get pushed onto.
 * Must be called once, before uart_receiver_task() is started, since the
 * task reads from the UART peripheral this function brings up.
 *
 * @param data_queue Queue to push each successfully-parsed and
 * CRC-verified telemetry_payload_t onto (consumed by mqtt_publisher_task
 * on the wifi_mqtt_client side). Ownership stays with the caller; this
 * component only holds the handle.
 */
void uart_receiver_init(QueueHandle_t data_queue);

/**
 * @brief FreeRTOS task entry point: continuously reads UART bytes, runs
 * them through the frame-sync/length/payload/CRC state machine defined in
 * uart_receiver.cpp, and on a valid frame ACKs the STM32 and pushes the
 * payload onto the queue given to uart_receiver_init(). Never returns
 * under normal operation - intended to run for the lifetime of the app.
 *
 * @param pvParameters Unused (required by the FreeRTOS task signature).
 */
void uart_receiver_task(void *pvParameters);

#endif // UART_RECEIVER_H
