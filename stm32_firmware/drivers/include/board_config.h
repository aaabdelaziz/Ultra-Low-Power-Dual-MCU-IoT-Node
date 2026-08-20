/*
 * board_config.h
 *
 * Single source of truth for the STM32L476 pin/peripheral assignments used
 * across the drivers layer (dma_adc, power_manager, systick_delay) and the
 * middleware layer (uart_handshake). Centralizing the mapping here means
 * every module agrees on which physical pin does what, and it is the one
 * place to edit when porting this firmware to a different PCB.
 *
 * Target MCU: STM32L476RG (Cortex-M4F, 1 MB Flash / 128 KB SRAM).
 * Pin numbers below are GPIOA pin indices (0-15).
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* GPIOA pin used to pulse the ESP32 out of deep sleep before a UART burst. */
#define WAKE_PIN                 0U   /* PA0, push-pull output */

/* USART2 pins (Alternate Function 7), the STM32<->ESP32 telemetry link. */
#define UART_TX_PIN              2U   /* PA2 */
#define UART_RX_PIN              3U   /* PA3 */
#define UART_GPIO_AF             7U   /* AF7 = USART2 on PA2/PA3 */

/*
 * ADC1 input pins/channels sampled into the circular DMA buffer. The order
 * here (battery, temp, generic) matches ADC_CHANNELS indices 0/1/2 in
 * dma_adc.c, which in turn matches the fields app/src/main.c reads out of
 * telemetry_payload_t (battery_mv, temp_sensor, adc_ch1).
 */
#define ADC_CH_BATTERY_PIN       1U    /* PA1  -> ADC1_IN6  (battery_mv)  */
#define ADC_CH_BATTERY_NUM       6U
#define ADC_CH_TEMP_PIN          4U    /* PA4  -> ADC1_IN9  (temp_sensor) */
#define ADC_CH_TEMP_NUM          9U
#define ADC_CH_GENERIC_PIN       5U    /* PA5  -> ADC1_IN10 (adc_ch1)     */
#define ADC_CH_GENERIC_NUM      10U

#endif /* BOARD_CONFIG_H */
