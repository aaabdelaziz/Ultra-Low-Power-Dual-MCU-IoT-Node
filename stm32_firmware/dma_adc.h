#pragma once
#include <stddef.h>

void dma_adc_init(void);
int dma_adc_has_data(void);
size_t dma_adc_read(uint16_t* buf, size_t max_len);
#pragma once
#include <stdint.h>

typedef struct {
    uint32_t timestamp;
    int16_t sample;
} telemetry_payload_t;

void dma_adc_init(void);
void dma_adc_start(void);
bool dma_adc_is_data_ready(void);
void dma_adc_get_averaged_channel(telemetry_payload_t *out);
void dma_adc_clear_data_ready(void);
