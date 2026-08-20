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

/*
 * ESP-IDF application entry point. This is the composition root for the
 * ESP32 side of the dual-MCU node: it does not parse UART frames or talk
 * to Wi-Fi/MQTT itself, it only creates the shared queue, initializes the
 * two components around it, and starts their FreeRTOS tasks.
 *
 * Data flow: uart_receiver_task (STM32 -> UART -> parse -> queue) feeds
 * telemetry_queue; mqtt_publisher_task drains that same queue and
 * publishes each entry to the cloud broker. app_main() itself never
 * touches a telemetry_payload_t directly.
 */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Ultra-Low-Power Dual-MCU IoT Telemetry Node (ESP32 side)");

    // Create the queue that decouples UART reception from MQTT publishing:
    // sized to buffer up to 10 telemetry packets so a slow/stalled MQTT
    // publish cannot block the UART receiver from ACKing the STM32 in time.
    telemetry_queue = xQueueCreate(10, sizeof(telemetry_payload_t));
    if (telemetry_queue == NULL) {
        // xQueueCreate() only fails on heap exhaustion; nothing downstream
        // is safe to start without the queue, so bail out of app_main()
        // entirely rather than continuing in a half-initialized state.
        ESP_LOGE(TAG, "Failed to create telemetry queue");
        return;
    }

    // Bring up both components before starting any tasks that depend on
    // them: uart_receiver_init() wires the queue in and configures the
    // UART peripheral (so uart_receiver_task has somewhere to push frames
    // and a peripheral to read from as soon as it starts), and
    // wifi_mqtt_init() brings up NVS/Wi-Fi/MQTT (so mqtt_publisher_task
    // has a client handle ready as soon as it starts).
    uart_receiver_init(telemetry_queue);
    wifi_mqtt_init();

    // Start the two worker tasks. uart_receiver_task runs at a slightly
    // higher priority (5 vs 4) than mqtt_publisher_task since draining the
    // UART peripheral promptly matters more than publish latency - a late
    // MQTT publish just delays telemetry, but a late UART read risks
    // missing bytes and dropping a whole frame.
    xTaskCreate(uart_receiver_task, "uart_rx_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 4096, (void*)telemetry_queue, 4, NULL);

    // app_main() itself has no more work to do once the two tasks are
    // running; it just idles forever instead of returning (returning from
    // app_main() would let FreeRTOS delete this task, which is fine, but
    // parking it here leaves a spot to add system-level supervision later
    // without restructuring the entry point).
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        // Optional: Implement modem sleep logic here if the queue is empty
        // ESP32 can manage its own Wi-Fi sleep modes automatically via ESP-IDF power management.
    }
}
