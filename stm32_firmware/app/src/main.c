/*
 * main.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#include <stdint.h>
#include "power_manager.h"
#include "dma_adc.h"
#include "uart_handshake.h"

// Delay function stub
void delay_ms(uint32_t ms) {
    // Implement simple SysTick delay or dummy loop
    (void)ms;
}

int main(void) {
    // 1. Initialize system clock
    // SystemClock_Config();
    
    // 2. Initialize low power features
    pm_init();
    pm_configure_unused_gpios_analog();
    pm_disable_unused_clocks();
    
    // 3. Initialize peripherals
    uart_handshake_init();
    dma_adc_init();
    
    // 4. Start background data acquisition
    dma_adc_start();
    
    // Main event loop
    while (1) {
        if (dma_adc_is_data_ready()) {
            // DMA has filled the circular buffer, process the data
            
            telemetry_payload_t data_packet;
            // E.g., Use an RTC or SysTick if enabled, otherwise 0
            data_packet.timestamp_ms = 0; 
            
            // Calculate averages from the DMA buffer
            data_packet.battery_mv = dma_adc_get_averaged_channel(0);
            data_packet.temp_sensor = dma_adc_get_averaged_channel(1);
            data_packet.adc_ch1 = dma_adc_get_averaged_channel(2);
            data_packet.status_flags = 0x01; // System OK
            
            // Clear flag before starting transmission
            dma_adc_clear_data_ready();
            
            // Wake up ESP32
            uart_wake_esp32();
            
            // Send data and wait for ACK
            bool ack_received = uart_send_telemetry_and_wait_ack(&data_packet);
            
            if (!ack_received) {
                // Handle NACK or Timeout (e.g., store locally, retry, or flag error)
                data_packet.status_flags |= 0x02; // Comm Error
            }
        }
        
        // If data is not ready, or transmission is complete, go to deep sleep
        // The ADC DMA or an external RTC will wake the CPU
        pm_enter_stop_mode();
        
        // After waking up from STOP mode, execution resumes here.
        // Clocks might need to be reconfigured depending on the MCU.
    }
    
    return 0; // Unreachable
}
