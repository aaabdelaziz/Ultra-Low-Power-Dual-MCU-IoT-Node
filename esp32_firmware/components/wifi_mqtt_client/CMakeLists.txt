idf_component_register(SRCS "wifi_mqtt_client.cpp"
                    INCLUDE_DIRS "include" "../../../common"
                    REQUIRES mqtt nvs_flash esp_wifi esp_event esp_system)
