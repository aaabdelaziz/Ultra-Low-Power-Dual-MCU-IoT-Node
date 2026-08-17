/*
 * uart_handshake.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include "uart_handshake.h"
#include <stddef.h>

// Note: Generic abstraction replacing specific HAL/CMSIS calls

void uart_handshake_init(void) {
    // 1. Enable UART and GPIO clocks
    // 2. Configure TX/RX pins (Alternate Function)
    // 3. Configure UART (Baudrate: e.g., 115200, 8N1)
    // 4. Enable UART transmitter and receiver
}

void uart_wake_esp32(void) {
    // ESP32 might be in deep sleep. Wake it up by toggling a dedicated Wake pin,
    // or by sending a break character / dummy byte on UART.
    
    // Example: Toggle GPIO
    // GPIOA->BSRR = (1 << WAKE_PIN); // Set
    // delay_ms(10);
    // GPIOA->BSRR = (1 << (WAKE_PIN + 16)); // Reset
    
    // Give ESP32 some time to boot/wake and initialize its UART
    // delay_ms(100);
}

// Blocks and waits for a specific byte on UART RX
static bool uart_wait_for_byte(uint8_t expected_byte, uint32_t timeout_loops) {
    uint32_t timer = 0;
    while (timer < timeout_loops) {
        // if (UART->ISR & UART_ISR_RXNE) {
        //     uint8_t rx_data = (uint8_t)(UART->RDR & 0xFF);
        //     if (rx_data == expected_byte) return true;
        // }
        
        // Simulating condition check
        timer++;
    }
    return false;
}

// Sends a buffer over UART (blocking or via DMA, blocking shown for simplicity)
static void uart_send_buffer(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        // while (!(UART->ISR & UART_ISR_TXE)); // Wait for TX empty
        // UART->TDR = data[i];                 // Send byte
    }
}

bool uart_send_telemetry_and_wait_ack(const telemetry_payload_t *payload) {
    if (payload == NULL) return false;
    
    uart_frame_t frame;
    frame.sync1 = PROTOCOL_SYNC_BYTE_1;
    frame.sync2 = PROTOCOL_SYNC_BYTE_2;
    frame.length = sizeof(telemetry_payload_t);
    frame.payload = *payload;
    
    frame.crc16 = calculate_crc16((const uint8_t*)&frame.payload, frame.length);
    
    // Send the frame
    uart_send_buffer((const uint8_t*)&frame, sizeof(uart_frame_t));
    
    // Wait for ACK
    // 100000 is an arbitrary timeout loop count
    return uart_wait_for_byte(PROTOCOL_ACK_BYTE, 100000); 
}
