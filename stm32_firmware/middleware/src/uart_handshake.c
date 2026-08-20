/*
 * uart_handshake.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 * Middleware layer: owns the USART2 peripheral registers and builds/sends
 * the framed telemetry protocol defined in common/protocol_spec.h. Sits
 * above the register-level drivers (power_manager, dma_adc) and below
 * app/src/main.c, which only calls the three functions declared in
 * uart_handshake.h.
 */

#include "uart_handshake.h"
#include "board_config.h"
#include "systick_delay.h"
#include "stm32l4xx.h"
#include <stddef.h>

void uart_handshake_init(void) {
    /* 1. Clock USART2. GPIOA's clock and the pins' MODER=Alternate Function
     *    setting are already handled by pm_configure_unused_gpios_analog();
     *    this function only needs to pick which alternate function (AF7 =
     *    USART2) those two pins carry. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* 2. AFRL selects the alternate function per pin, 4 bits/pin, pins 0-7
     *    live in AFR[0]. Clear each pin's nibble first, then write AF7. */
    GPIOA->AFR[0] &= ~((0xFU << (UART_TX_PIN * 4U)) | (0xFU << (UART_RX_PIN * 4U)));
    GPIOA->AFR[0] |= (UART_GPIO_AF << (UART_TX_PIN * 4U))
                    | (UART_GPIO_AF << (UART_RX_PIN * 4U));

    /* 3. Baud rate: with OVER8=0 (16x oversampling, CR1 reset default),
     *    BRR is simply the clock divided by the target baud rate. USART2
     *    is clocked from PCLK1, which equals SystemCoreClock here because
     *    this firmware never touches the APB1 prescaler (RCC->CFGR.PPRE1
     *    stays at its /1 reset value). */
    USART2->BRR = (uint32_t)(SystemCoreClock / 115200U);

    /* 4. 8 data bits, no parity, 1 stop bit are all reset defaults
     *    (CR1.M[1:0]=00, CR1.PCE=0, CR2.STOP=00), so only enable the
     *    transmitter, receiver, and the peripheral itself. */
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;
}

void uart_wake_esp32(void) {
    /* ESP32 may be in deep/modem sleep; a >=10ms low-to-high pulse on
     * WAKE_PIN is enough to bring it out of deep sleep via an RTC GPIO
     * wake source configured on the ESP32 side. BSRR is a single atomic
     * write, so "set" and "reset" each need only one instruction and no
     * read-modify-write race. */
    GPIOA->BSRR = (1U << WAKE_PIN);          /* set: pulse starts   */
    delay_ms(10);
    GPIOA->BSRR = (1U << (WAKE_PIN + 16U));  /* reset: pulse ends   */

    /* Give the ESP32 time to finish waking and bring its own UART
     * peripheral up before the first frame byte arrives. */
    delay_ms(100);
}

/* Blocks until `expected_byte` arrives on USART2 RX, or `timeout_loops`
 * polls have elapsed with nothing matching. This is a coarse, clock-speed
 * dependent timeout (not millisecond-accurate) - acceptable here because
 * the caller only needs "did an ACK show up at all", not precise timing. */
static bool uart_wait_for_byte(uint8_t expected_byte, uint32_t timeout_loops) {
    for (uint32_t timer = 0; timer < timeout_loops; timer++) {
        if ((USART2->ISR & USART_ISR_RXNE) != 0U) {
            /* Reading RDR also clears RXNE. */
            uint8_t rx_data = (uint8_t)(USART2->RDR & 0xFFU);
            if (rx_data == expected_byte) {
                return true;
            }
        }
    }
    return false;
}

/* Blocking transmit: waits for TXE (previous byte moved out of TDR into
 * the shift register) before loading the next one. Simple and correct for
 * this design's small, infrequent frames; a DMA-driven transmit would be
 * the next optimization if frame size or frequency grew significantly. */
static void uart_send_buffer(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        while ((USART2->ISR & USART_ISR_TXE) == 0U) {
            /* wait for the transmit data register to empty */
        }
        USART2->TDR = data[i];
    }
    /* Wait for the last byte to actually leave the shift register (TC)
     * before returning, so callers can safely assume the wire is idle. */
    while ((USART2->ISR & USART_ISR_TC) == 0U) {
        /* wait for transmission complete */
    }
}

bool uart_send_telemetry_and_wait_ack(const telemetry_payload_t *payload) {
    if (payload == NULL) {
        return false;
    }

    uart_frame_t frame;
    frame.sync1 = PROTOCOL_SYNC_BYTE_1;
    frame.sync2 = PROTOCOL_SYNC_BYTE_2;
    frame.length = sizeof(telemetry_payload_t);
    frame.payload = *payload;
    frame.crc16 = calculate_crc16((const uint8_t *)&frame.payload, frame.length);

    uart_send_buffer((const uint8_t *)&frame, sizeof(uart_frame_t));

    /* 100000 is an arbitrary busy-poll iteration count, not a calibrated
     * time value - see uart_wait_for_byte()'s comment. */
    return uart_wait_for_byte(PROTOCOL_ACK_BYTE, 100000);
}
