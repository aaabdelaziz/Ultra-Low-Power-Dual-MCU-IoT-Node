/*
 * power_manager.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 * Register-level power/clock management for STM32L476. Every function here
 * only touches RCC/PWR/GPIO/SCB registers directly (no vendor HAL) so the
 * exact power cost of each step is visible and controllable.
 */

#include "power_manager.h"
#include "board_config.h"
#include "stm32l4xx.h"

void pm_init(void) {
    /* The PWR peripheral registers (PWR->CR1, PWR->SCR, ...) are gated by
     * the RCC APB1 clock enable below; without this, any write to PWR->*
     * is silently ignored. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
}

void pm_configure_unused_gpios_analog(void) {
    /* To minimize power consumption, unused GPIOs are configured to Analog
     * mode: this disconnects the Schmitt-trigger input buffer, which is
     * the source of leakage current on a floating pin.
     *
     * GPIOA is the only port this project touches (see board_config.h), so
     * only GPIOA needs the clock enabled and the sweep applied. A board
     * using additional ports would repeat this pattern per port. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* Set every pin's MODER field to 0b11 (Analog) first ... */
    GPIOA->MODER = 0xFFFFFFFFU;

    /* ... then re-open exactly the pins this firmware actually drives:
     * WAKE_PIN as a digital output, and the two USART2 pins as their
     * alternate function. The ADC input pins (PA1/PA4/PA5) are left in
     * Analog mode on purpose - that IS the correct mode for an ADC input,
     * dma_adc_init() does not need to touch their MODER again. */
    GPIOA->MODER &= ~(0x3U << (WAKE_PIN * 2U));
    GPIOA->MODER |=  (0x1U << (WAKE_PIN * 2U));  /* 0b01 = General purpose output */

    GPIOA->MODER &= ~(0x3U << (UART_TX_PIN * 2U));
    GPIOA->MODER |=  (0x2U << (UART_TX_PIN * 2U)); /* 0b10 = Alternate function */
    GPIOA->MODER &= ~(0x3U << (UART_RX_PIN * 2U));
    GPIOA->MODER |=  (0x2U << (UART_RX_PIN * 2U));
}

void pm_disable_unused_clocks(void) {
    /* Peripherals this firmware never uses are left un-clocked by default
     * (RCC enable bits reset to 0), so there is nothing extra to disable
     * here for a fresh boot. This function stays as the documented place
     * to gate off any clock a future driver enables but later stops
     * needing (e.g. a temporarily-used timer), keeping that decision in
     * one auditable spot instead of scattered across drivers. */
}

void pm_enter_stop_mode(void) {
    /* STOP 2 is STM32L4's deepest RAM-retention mode: core, most clocks
     * and most peripherals are off; SRAM and register contents survive.
     * Wakeup here is via the DMA1 transfer-complete interrupt configured
     * in dma_adc_init() (EXTI/RTC wakeup sources are not used by this
     * design). */

    /* 1. Clear any stale wakeup flags left over from the previous cycle,
     *    otherwise PWR can refuse to re-enter STOP mode. */
    PWR->SCR |= PWR_SCR_CWUF;

    /* 2. Select STOP 2 (lowest-power retention mode; fine here since the
     *    ADC only runs while the CPU is awake processing a completed DMA
     *    batch, not during the sleep window itself). */
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS) | PWR_CR1_LPMS_STOP2;

    /* 3. SLEEPDEEP tells the Cortex-M4 core to route the next WFI into a
     *    deep-sleep request instead of plain Sleep mode; PWR->CR1.LPMS
     *    then picks which deep-sleep flavor (STOP2 here). */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* 4. Halt the core here until an enabled interrupt (DMA1 Channel1
     *    transfer-complete) wakes it back up. */
    __WFI();

    /* -- Execution resumes here after the wakeup interrupt -- */

    /* 5. Leave SLEEPDEEP cleared so any incidental WFI elsewhere in the
     *    codebase (e.g. future additions) defaults back to plain Sleep. */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    /* 6. STM32L4 always restarts on MSI after STOP2 regardless of the
     *    clock that was active before sleeping. system_stm32l4xx.c leaves
     *    SystemCoreClock at the MSI default (4 MHz) and this firmware
     *    never reconfigures the PLL, so no explicit re-init is required
     *    here; SystemCoreClock is already correct for delay_ms()/USART2. */
}

void pm_enter_sleep_mode(void) {
    /* Plain SLEEP mode: only the CPU clock stops, peripheral clocks (and
     * therefore DMA) keep running. Used instead of STOP2 when the code
     * only needs to idle briefly waiting on a peripheral, not go through
     * a full low-power/wake cycle. Not currently called by app/src/main.c
     * but kept available for that use case per power_manager.h's API. */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
}
