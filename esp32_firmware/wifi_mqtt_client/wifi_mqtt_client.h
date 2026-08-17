#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../uart_receiver/uart_receiver.h"

void wifi_mqtt_init(void);
void wifi_mqtt_publish_telemetry(const telemetry_t* t);
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void wifi_mqtt_init(QueueHandle_t in_queue);
