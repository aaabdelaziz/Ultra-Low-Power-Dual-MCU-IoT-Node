/*
 * systick_delay.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 */

#include "systick_delay.h"
#include "stm32l4xx.h"

void systick_delay_init(void) {
    /* SysTick reload = (clock ticks per ms) - 1. SystemCoreClock is kept
     * up to date by system_stm32l4xx.c whenever the clock tree changes. */
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;
    /* Processor clock as source, no interrupt (we just poll COUNTFLAG),
     * counter enabled. */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        /* COUNTFLAG is set by hardware each time SysTick wraps (i.e. once
         * per millisecond) and clears itself on read. */
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
            /* wait for the next 1ms tick */
        }
    }
}
