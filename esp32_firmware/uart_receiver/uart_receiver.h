#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t timestamp;
    uint8_t payload[64];
    size_t len;
} telemetry_t;

void uart_receiver_init(void);
void uart_receiver_task(void* arg);
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void uart_receiver_init(QueueHandle_t out_queue);
