/*
 * systick_delay.h
 *
 * Minimal blocking millisecond delay built on the Cortex-M4 SysTick timer.
 * Used by power_manager (boot-time settle waits) and uart_handshake (ESP32
 * wake-pulse timing).
 */

#ifndef SYSTICK_DELAY_H
#define SYSTICK_DELAY_H

#include <stdint.h>

/* Configures SysTick to tick once per millisecond off SystemCoreClock.
 * Must be called once, after SystemInit(), before the first delay_ms(). */
void systick_delay_init(void);

/* Busy-waits for approximately `ms` milliseconds. */
void delay_ms(uint32_t ms);

#endif /* SYSTICK_DELAY_H */
