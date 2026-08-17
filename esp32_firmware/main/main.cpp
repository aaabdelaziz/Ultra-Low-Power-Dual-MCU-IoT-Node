/*
 * main.cpp
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "uart_receiver.h"
#include "wifi_mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

// Queue to pass telemetry data from UART receiver task to MQTT publisher task
static QueueHandle_t telemetry_queue;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Ultra-Low-Power Dual-MCU IoT Telemetry Node (ESP32 side)");

    // Create queue (buffer up to 10 telemetry packets)
    telemetry_queue = xQueueCreate(10, sizeof(telemetry_payload_t));
    if (telemetry_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create telemetry queue");
        return;
    }

    // Initialize subsystems
    uart_receiver_init(telemetry_queue);
    wifi_mqtt_init();

    // Create tasks
    xTaskCreate(uart_receiver_task, "uart_rx_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 4096, (void*)telemetry_queue, 4, NULL);
    
    // Main task can now sleep or monitor the system
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        // Optional: Implement modem sleep logic here if the queue is empty
        // ESP32 can manage its own Wi-Fi sleep modes automatically via ESP-IDF power management.
    }
}
