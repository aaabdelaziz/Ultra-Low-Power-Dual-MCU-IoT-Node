#include "dma_adc.h"
#include "power_manager.h"
#include "uart_handshake.h"
#include <stdio.h>

int main(void) {
    // Initialize hardware (clocks, GPIO, ADC, DMA)
    power_manager_init();
    dma_adc_init();

    // Main low-power sampling loop (placeholder)
    while (1) {
        // Wait for timer or event
        // aggregate samples, then perform handshake and send telemetry
        if (dma_adc_has_data()) {
            uint16_t buf[64];
            size_t n = dma_adc_read(buf, sizeof(buf)/sizeof(buf[0]));
            (void)n;
            uart_handshake_send(buf, n);
        }
        // Enter low-power mode (platform-specific)
        break;
    }
    return 0;
}
#include <stdint.h>
#include <stdbool.h>
#include "dma_adc.h"
#include "power_manager.h"
#include "uart_handshake.h"

int main(void)
{
    pm_init();
    pm_configure_unused_gpios_analog();

    dma_adc_init();
    dma_adc_start();

    uart_handshake_init();

    while (1) {
        pm_enter_stop_mode();

        if (dma_adc_is_data_ready()) {
            telemetry_payload_t payload;
            dma_adc_get_averaged_channel(&payload);
            uart_wake_esp32();
            uart_send_frame_and_wait_ack(&payload);
            dma_adc_clear_data_ready();
        }
    }
    return 0;
}
