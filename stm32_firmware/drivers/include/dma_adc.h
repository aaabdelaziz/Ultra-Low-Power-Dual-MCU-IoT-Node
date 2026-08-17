/*
 * dma_adc.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef DMA_ADC_H
#define DMA_ADC_H

#include <stdint.h>

// Number of ADC channels being sampled
#define ADC_CHANNELS 3
// Depth of the circular buffer per channel
#define ADC_BUFFER_DEPTH 16

/**
 * @brief Initializes the ADC for multi-channel scanning.
 */
void dma_adc_init(void);

/**
 * @brief Starts the ADC conversion using DMA in circular mode.
 */
void dma_adc_start(void);

/**
 * @brief Stops the ADC and DMA transfers.
 */
void dma_adc_stop(void);

/**
 * @brief Checks if a new batch of ADC data is ready.
 * @return 1 if data ready, 0 otherwise
 */
uint8_t dma_adc_is_data_ready(void);

/**
 * @brief Clears the data ready flag.
 */
void dma_adc_clear_data_ready(void);

/**
 * @brief Gets the averaged value for a specific channel.
 * @param channel_index Index of the channel (0 to ADC_CHANNELS-1)
 * @return Averaged 12-bit ADC value
 */
uint16_t dma_adc_get_averaged_channel(uint8_t channel_index);

#endif // DMA_ADC_H
