#include "dma_adc.h"
#include <stddef.h>

void dma_adc_init(void) {
    // Configure ADC + DMA (platform-specific)
}

int dma_adc_has_data(void) {
    return 0; // Placeholder
}

size_t dma_adc_read(uint16_t* buf, size_t max_len) {
    (void)buf; (void)max_len; return 0;
}
#include "dma_adc.h"
#include <stdlib.h>

static volatile int data_ready = 0;

void dma_adc_init(void)
{
    // TODO: configure ADC, DMA channels and circular buffer
}

void dma_adc_start(void)
{
}

bool dma_adc_is_data_ready(void)
{
    return data_ready != 0;
}

void dma_adc_get_averaged_channel(telemetry_payload_t *out)
{
    // placeholder - fill with dummy data
    out->timestamp = 0;
    out->sample = 0;
}

void dma_adc_clear_data_ready(void)
{
    data_ready = 0;
}
