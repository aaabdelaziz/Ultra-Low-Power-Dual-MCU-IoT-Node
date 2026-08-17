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

// Initialize NVS, Wi-Fi, and MQTT
void wifi_mqtt_init(void);

// FreeRTOS Task to handle taking data from queue and publishing via MQTT
void mqtt_publisher_task(void *pvParameters);

#endif // WIFI_MQTT_CLIENT_H
