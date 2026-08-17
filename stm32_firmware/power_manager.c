#include "power_manager.h"

void power_manager_init(void) {
    // Configure power domains, GPIO used for wake/handshake
}

void enter_low_power_mode(void) {
    // Enter MCU-specific low-power mode (STOP, SLEEP)
}
#include "power_manager.h"

void pm_init(void)
{
    // Configure clocks and wake sources
}

void pm_configure_unused_gpios_analog(void)
{
}

void pm_enter_stop_mode(void)
{
    // Enter low-power stop mode (placeholder)
}
