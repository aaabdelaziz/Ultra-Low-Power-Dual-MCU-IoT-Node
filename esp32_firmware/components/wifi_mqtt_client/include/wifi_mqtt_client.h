/*
 * wifi_mqtt_client.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef WIFI_MQTT_CLIENT_H
#define WIFI_MQTT_CLIENT_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "protocol_spec.h"

/**
 * @brief Brings up NVS, Wi-Fi station mode, and the MQTT client (connect
 * happens asynchronously once an IP is obtained - see wifi_event_handler()
 * in wifi_mqtt_client.cpp). Must be called once, before
 * mqtt_publisher_task() is started, since that task publishes through the
 * client handle this function creates.
 */
void wifi_mqtt_init(void);

/**
 * @brief FreeRTOS task entry point: blocks on a queue of
 * telemetry_payload_t, and for each item received, formats it as JSON and
 * publishes it to the MQTT broker over the connection wifi_mqtt_init()
 * set up. Never returns under normal operation.
 *
 * @param pvParameters The QueueHandle_t to read telemetry from, passed as
 * a `void*` per the FreeRTOS task signature (see main.cpp's xTaskCreate()
 * call, which passes telemetry_queue here).
 */
void mqtt_publisher_task(void *pvParameters);

#endif // WIFI_MQTT_CLIENT_H
