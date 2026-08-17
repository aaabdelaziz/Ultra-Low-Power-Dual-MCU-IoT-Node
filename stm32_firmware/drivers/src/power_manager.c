/*
 * power_manager.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include "power_manager.h"

// Note: In a real STM32 project, you would include the specific device header here
// e.g., #include "stm32g0xx.h" or #include "stm32l4xx.h"
// This is a generic/abstracted implementation demonstrating the concepts.

void pm_init(void) {
    // Enable Power Control (PWR) clock
    // RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN; (Example for STM32L4)
}

void pm_configure_unused_gpios_analog(void) {
    // To minimize power consumption, all unused GPIOs should be configured
    // to Analog mode. This disables the digital input trigger circuit, 
    // eliminating leakage current caused by floating inputs.
    
    // Example pseudo-code for GPIOA
    // Enable clock for GPIOA
    // RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    
    // Set all pins to analog mode (0b11 in MODER)
    // GPIOA->MODER = 0xFFFFFFFF;
    
    // Except for pins we actually need (e.g., UART TX/RX, ADC input)
    // GPIOA->MODER &= ~GPIO_MODER_MODE2_Msk; // Clear UART TX
    // GPIOA->MODER |= (0b10 << GPIO_MODER_MODE2_Pos); // Set to Alternate Function
}

void pm_disable_unused_clocks(void) {
    // Disable clocks to all peripherals that are not active during sleep
    // e.g., timers, SPI, I2C, unneeded GPIO ports
    
    // RCC->AHB1ENR &= ~(RCC_AHB1ENR_CRCEN);
    // RCC->APB2ENR &= ~(RCC_APB2ENR_TIM1EN);
}

void pm_enter_stop_mode(void) {
    // STOP mode allows sub-10uA current draw while retaining SRAM.
    // Core is halted, most clocks are stopped.
    
    // 1. Clear Wakeup flags
    // PWR->SCR |= PWR_SCR_CWUF;

    // 2. Select STOP mode (e.g., Stop 2 on STM32L4)
    // PWR->CR1 &= ~PWR_CR1_LPMS;
    // PWR->CR1 |= PWR_CR1_LPMS_STOP2;

    // 3. Set SLEEPDEEP bit in Cortex-M System Control Register
    // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // 4. Wait For Interrupt
    // __WFI();

    // -- Wakes up here after an interrupt --

    // 5. Clear SLEEPDEEP bit
    // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    // 6. Re-configure system clock (usually resets to default internal RC on wake)
    // SystemClock_Config();
}

void pm_enter_sleep_mode(void) {
    // SLEEP mode is used when waiting for a peripheral (like DMA) to finish
    // CPU clock is off, but peripheral clocks remain on.
    
    // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    // __WFI();
}
