/*
 * power_manager.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes power management features.
 * Should be called early in main().
 */
void pm_init(void);

/**
 * @brief Configures unused GPIO pins to Analog mode.
 * This eliminates leakage current on floating input pins.
 */
void pm_configure_unused_gpios_analog(void);

/**
 * @brief Gates clocks to unused peripherals to save power.
 */
void pm_disable_unused_clocks(void);

/**
 * @brief Enters STOP 2 mode (deepest RAM-retention deep sleep on
 * STM32L4): CPU, most clocks, and most peripherals are stopped, but SRAM
 * and register contents are preserved. Woken up by the DMA1 Channel1
 * transfer-complete interrupt (raised when dma_adc.c's circular buffer
 * finishes a lap) - this design does not use EXTI or RTC as a wakeup
 * source. Returns after the wakeup interrupt has been serviced.
 */
void pm_enter_stop_mode(void);

/**
 * @brief Enters SLEEP mode (CPU stopped, peripherals active).
 * Useful when waiting for DMA transfers to complete.
 */
void pm_enter_sleep_mode(void);

#endif // POWER_MANAGER_H
