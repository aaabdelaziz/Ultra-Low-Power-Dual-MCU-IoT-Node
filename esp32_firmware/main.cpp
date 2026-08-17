#include "uart_receiver/uart_receiver.h"
#include "wifi_mqtt_client/wifi_mqtt_client.h"
#include <stdio.h>

extern "C" void app_main()
{
    // Initialize modules
    uart_receiver_init();
    wifi_mqtt_init();

    // Create tasks (platform-specific). Placeholders call task entry points directly.
    // In real FreeRTOS: xTaskCreate(uart_receiver_task, "uart_rx", 4096, NULL, 5, NULL);
    // xTaskCreate(wifi_mqtt_task, "wifi", 8192, NULL, 5, NULL);

    printf("esp32_firmware: initialized\n");
}
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "uart_receiver/uart_receiver.h"
#include "wifi_mqtt_client/wifi_mqtt_client.h"

static QueueHandle_t telemetry_queue = nullptr;

extern "C" void app_main()
{
    // Queue for passing telemetry payloads from UART task to MQTT publisher
    telemetry_queue = xQueueCreate(10, 256);

    if (telemetry_queue == NULL) {
        printf("Failed to create telemetry queue\n");
        return;
    }

    uart_receiver_init(telemetry_queue);
    wifi_mqtt_init(telemetry_queue);

    // Tasks are created inside the component init functions
    // app_main can perform lightweight supervision or enter low-power loop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
