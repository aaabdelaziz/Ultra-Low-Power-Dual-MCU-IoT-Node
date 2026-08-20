/*
 * main.c
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 * Application layer: owns *when* things happen, not *how*. Every register
 * touch lives in the drivers (power_manager, dma_adc, systick_delay) and
 * middleware (uart_handshake) layers below this file.
 */

#include <stdint.h>
#include <stdbool.h>
#include "power_manager.h"
#include "dma_adc.h"
#include "uart_handshake.h"
#include "systick_delay.h"

int main(void) {
    /* SystemInit() has already run (called by Reset_Handler before main),
     * so SystemCoreClock reflects the post-reset MSI clock here. */

    /* 1. Power/clock setup first: gate off leakage paths and unused clocks
     *    before any peripheral is touched, so every later init step runs
     *    at the lowest baseline power draw available. */
    pm_init();
    pm_configure_unused_gpios_analog();
    pm_disable_unused_clocks();

    /* 2. SysTick-based delay is needed by uart_handshake's wake pulse
     *    timing, so bring it up before the peripherals that depend on it. */
    systick_delay_init();

    /* 3. Peripheral init: UART link to the ESP32, then the ADC/DMA
     *    sampling pipeline. */
    uart_handshake_init();
    dma_adc_init();

    /* 4. Start background data acquisition. From here on, ADC1 + DMA1
     *    free-run in hardware; the CPU is not needed again until DMA1
     *    raises its transfer-complete interrupt (see dma_adc.c). */
    dma_adc_start();

    /* Main event loop: sample -> (maybe) transmit -> sleep, forever. */
    while (1) {
        if (dma_adc_is_data_ready()) {
            /* The DMA circular buffer completed a full lap: average each
             * channel's samples and package them for transmission. */
            telemetry_payload_t data_packet;

            /* No RTC/free-running timer is configured in this design, so
             * timestamping is left at 0; a board that adds an RTC would
             * fill this in here. */
            data_packet.timestamp_ms = 0;

            data_packet.battery_mv  = dma_adc_get_averaged_channel(0);
            data_packet.temp_sensor = dma_adc_get_averaged_channel(1);
            data_packet.adc_ch1     = dma_adc_get_averaged_channel(2);
            data_packet.status_flags = 0x01; /* bit0: system OK */

            /* Clear the flag before transmitting so a DMA batch that
             * completes *during* the UART handshake is not lost - it will
             * simply be picked up on the next loop iteration. */
            dma_adc_clear_data_ready();

            uart_wake_esp32();

            bool ack_received = uart_send_telemetry_and_wait_ack(&data_packet);
            if (!ack_received) {
                /* No retry/backoff logic here by design: the next
                 * DMA-complete cycle will produce a fresh payload and try
                 * again, so a dropped frame is superseded rather than
                 * resent. The comm-error bit is set for visibility only -
                 * `data_packet` is local and about to go out of scope, so
                 * this flag does not currently reach the ESP32 or any
                 * persistent state; a future revision that needs to surface
                 * comm errors downstream would carry this bit into the
                 * *next* transmitted payload instead. */
                data_packet.status_flags |= 0x02;
            }
        }

        /* Whether or not data was ready this iteration, return to the
         * lowest-power state. DMA1's transfer-complete interrupt is what
         * wakes the CPU back up (see pm_enter_stop_mode()'s comment). */
        pm_enter_stop_mode();

        /* Execution resumes here after each wakeup, and the loop
         * re-checks dma_adc_is_data_ready() from the top. */
    }
}
