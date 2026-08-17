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
 * @brief Enters STOP mode (Deep Sleep).
 * CPU and core peripherals are stopped. Woken up by EXTI or RTC.
 */
void pm_enter_stop_mode(void);

/**
 * @brief Enters SLEEP mode (CPU stopped, peripherals active).
 * Useful when waiting for DMA transfers to complete.
 */
void pm_enter_sleep_mode(void);

#endif // POWER_MANAGER_H
