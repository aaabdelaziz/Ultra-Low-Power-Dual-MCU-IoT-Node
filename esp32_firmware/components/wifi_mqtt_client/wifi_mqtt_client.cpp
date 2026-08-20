/*
 * wifi_mqtt_client.cpp
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include "wifi_mqtt_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

// Note: Configure these via menuconfig in a real ESP-IDF project
#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define MQTT_BROKER_URI CONFIG_ESP_MQTT_BROKER_URI
#define MQTT_TOPIC     "telemetry/node_1"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static QueueHandle_t in_queue;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retry connecting to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // Start MQTT once we have an IP
        if (mqtt_client) {
            esp_mqtt_client_start(mqtt_client);
        }
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

void wifi_mqtt_init(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    // For demonstration, you would normally replace WIFI_SSID/PASS with actual strings or menuconfig values.
    // If menuconfig is not used, replace with hardcoded strings (e.g., "my_ssid").
    //
    // Zero-init the whole struct, then fill in only ssid/password via
    // strncpy. wifi_sta_config_t has 30+ members (scan_method, pmf_cfg,
    // he_*/vht_* 802.11 feature flags, ...); a partial designated
    // initializer naming only .ssid/.password left the rest at their
    // (correct, safe) zero defaults, but GCC's -Wmissing-field-initializers
    // treats that as an error in this C++ build. `= {}` says the same
    // thing without tripping the check. ssid/password are fixed-size
    // uint8_t arrays, not pointers, so they need strncpy rather than `=`.
    wifi_config_t wifi_config = {};
    #if defined(CONFIG_ESP_WIFI_SSID)
    strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS, sizeof(wifi_config.sta.password));
    #else
    strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), "YOUR_SSID", sizeof(wifi_config.sta.ssid));
    strncpy(reinterpret_cast<char *>(wifi_config.sta.password), "YOUR_PASSWORD", sizeof(wifi_config.sta.password));
    #endif

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    #if defined(CONFIG_ESP_MQTT_BROKER_URI)
    const char * broker_uri = MQTT_BROKER_URI;
    #else
    const char * broker_uri = "mqtt://test.mosquitto.org";
    #endif

    // Same missing-field-initializers reasoning as wifi_config above:
    // esp_mqtt_client_config_t has broker/credentials/session/network/
    // task/buffer/outbox sub-structs; only broker.address.uri is set here,
    // account it explicitly instead of via a partial aggregate literal.
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = broker_uri;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    // ESP_EVENT_ANY_ID is `#define ESP_EVENT_ANY_ID -1` (a plain int).
    // esp_mqtt_client_register_event's second parameter is the enum type
    // esp_mqtt_event_id_t; C++ (unlike C) will not implicitly convert an
    // int to an enum, so this needs an explicit cast.
    esp_mqtt_client_register_event(mqtt_client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
}

void mqtt_publisher_task(void *pvParameters) {
    in_queue = (QueueHandle_t)pvParameters;
    telemetry_payload_t data;
    
    char json_payload[128];

    while(1) {
        if (xQueueReceive(in_queue, &data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Publishing Telemetry Data...");
            
            // Format data as JSON
            snprintf(json_payload, sizeof(json_payload), 
                "{\"ts\":%lu,\"batt_mv\":%u,\"temp\":%u,\"adc\":%u,\"status\":%u}",
                (unsigned long)data.timestamp_ms,
                data.battery_mv,
                data.temp_sensor,
                data.adc_ch1,
                data.status_flags
            );

            // Publish to broker (QoS 1)
            if (mqtt_client) {
                esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, json_payload, 0, 1, 0);
            }
        }
    }
}
