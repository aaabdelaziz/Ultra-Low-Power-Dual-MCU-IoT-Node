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

/*
 * Wi-Fi/IP event callback, registered (twice, for two different event
 * bases) in wifi_mqtt_init(). ESP-IDF's event loop calls this
 * asynchronously whenever a matching Wi-Fi or IP event fires; it is not
 * called directly by any code in this project.
 *
 * This function only manages the connection itself; starting the MQTT
 * client is deferred to here (on IP_EVENT_STA_GOT_IP) rather than done
 * eagerly in wifi_mqtt_init(), since the MQTT client cannot usefully
 * connect to a broker before the station has an IP address and route to
 * the internet.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Station mode driver has just started; kick off the first
        // connection attempt to the configured AP.
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Lost the AP (or a prior connect attempt failed) - simply retry.
        // No backoff/retry-limit here: for this always-on telemetry node,
        // retrying indefinitely is the desired behavior over giving up.
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

/*
 * MQTT client event callback, registered in wifi_mqtt_init() via
 * esp_mqtt_client_register_event(ESP_EVENT_ANY_ID, ...) - called
 * asynchronously by the MQTT client's own task for every event it emits,
 * not invoked directly elsewhere in this project. Currently used only for
 * connection-state logging; mqtt_publisher_task() does not wait on any of
 * these events before publishing (esp_mqtt_client_publish() internally
 * queues if the client is not yet connected).
 */
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
            // Other events (MQTT_EVENT_PUBLISHED, MQTT_EVENT_SUBSCRIBED,
            // MQTT_EVENT_DATA, ...) are not currently logged; this node
            // only publishes and does not subscribe to anything, so most
            // of the MQTT event surface does not apply here.
            break;
    }
}

/*
 * Brings up the full Wi-Fi + MQTT stack in one call: NVS (required by the
 * Wi-Fi driver to store calibration data), the netif/event-loop
 * infrastructure, the Wi-Fi station itself, and an MQTT client pointed at
 * the configured broker. Wi-Fi connection and MQTT startup both continue
 * asynchronously after this function returns - see wifi_event_handler()
 * for what happens once an IP address is actually obtained.
 */
void wifi_mqtt_init(void) {
    // NVS (non-volatile storage) backs the Wi-Fi driver's PHY calibration
    // data; a fresh/incompatible NVS partition must be erased and
    // reinitialized before esp_wifi_init() below can use it.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Network interface layer + default event loop: required plumbing
    // before any Wi-Fi API call, and the mechanism wifi_event_handler()
    // and mqtt_event_handler() are delivered through.
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Subscribe wifi_event_handler() to every Wi-Fi event (connect,
    // disconnect, ...) and specifically to "got an IP" - the two calls use
    // different event bases (WIFI_EVENT vs IP_EVENT) so both are needed to
    // cover the full connect sequence handled in that callback.
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

    // Create the client but do not start it yet - esp_mqtt_client_start()
    // is only called from wifi_event_handler() once an IP is obtained, so
    // the client isn't attempting a broker connection before the network
    // is actually usable.
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    // ESP_EVENT_ANY_ID is `#define ESP_EVENT_ANY_ID -1` (a plain int).
    // esp_mqtt_client_register_event's second parameter is the enum type
    // esp_mqtt_event_id_t; C++ (unlike C) will not implicitly convert an
    // int to an enum, so this needs an explicit cast.
    esp_mqtt_client_register_event(mqtt_client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
}

/*
 * FreeRTOS task: the sole consumer of the telemetry queue produced by
 * uart_receiver_task(). Blocks until a payload is available, serializes
 * it to a small JSON object, and publishes it to the cloud broker. This
 * is the last stage of the STM32 -> UART -> queue -> MQTT -> cloud
 * pipeline described in the project README.
 */
void mqtt_publisher_task(void *pvParameters) {
    // main.cpp passes telemetry_queue as pvParameters (see xTaskCreate()
    // in main.cpp); cache it in the module-level in_queue so this
    // function's signature can stay the generic FreeRTOS task shape.
    in_queue = (QueueHandle_t)pvParameters;
    telemetry_payload_t data;

    char json_payload[128];

    while(1) {
        // portMAX_DELAY: block indefinitely until a new reading arrives -
        // this task has nothing else to do between telemetry packets, so
        // there is no benefit to polling instead of blocking.
        if (xQueueReceive(in_queue, &data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Publishing Telemetry Data...");

            // Format data as JSON. Field names are intentionally short
            // (ts/batt_mv/temp/adc/status) to keep the payload compact for
            // a low-bandwidth/metered MQTT link; the cloud-side subscriber
            // is expected to know this fixed schema.
            snprintf(json_payload, sizeof(json_payload),
                "{\"ts\":%lu,\"batt_mv\":%u,\"temp\":%u,\"adc\":%u,\"status\":%u}",
                (unsigned long)data.timestamp_ms,
                data.battery_mv,
                data.temp_sensor,
                data.adc_ch1,
                data.status_flags
            );

            // Publish to broker (QoS 1). mqtt_client may be NULL only if
            // wifi_mqtt_init() somehow wasn't called before this task
            // started (a programming error, not a runtime condition), so
            // this check is defensive rather than an expected path.
            if (mqtt_client) {
                esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, json_payload, 0, 1, 0);
            }
        }
    }
}
