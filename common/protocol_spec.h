/*
 * protocol_spec.h
 *
 * Ultra-Low-Power Dual-MCU IoT Telemetry Node
 *
 *  Created on: Aug 11, 2026
 *      Author:
 *          - Ahmed Abdelaziz
 */

#ifndef PROTOCOL_SPEC_H
#define PROTOCOL_SPEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sync bytes for UART frame detection
#define PROTOCOL_SYNC_BYTE_1 0xAA
#define PROTOCOL_SYNC_BYTE_2 0x55

// ACK and NACK responses from ESP32 to STM32
#define PROTOCOL_ACK_BYTE  0x06
#define PROTOCOL_NACK_BYTE 0x15

// Data structures must be packed to ensure identical alignment on both MCUs
#pragma pack(push, 1)

/**
 * @brief Telemetry payload structure
 * Represents the data collected by the STM32.
 */
typedef struct {
    uint32_t timestamp_ms; // Uptime or RTC timestamp
    uint16_t battery_mv;   // Battery voltage in millivolts
    uint16_t temp_sensor;  // Example sensor value
    uint16_t adc_ch1;      // General ADC channel
    uint8_t  status_flags; // Flags (e.g., motor on, errors)
} telemetry_payload_t;

/**
 * @brief Complete UART Frame Structure
 */
typedef struct {
    uint8_t sync1;
    uint8_t sync2;
    uint16_t length; // Length of the payload
    telemetry_payload_t payload;
    uint16_t crc16;  // CRC16 of the payload
} uart_frame_t;

#pragma pack(pop)

/**
 * @brief Calculate CRC16 (CCITT-FALSE): poly 0x1021, init 0xFFFF, no
 * reflection, no final XOR. Used by both MCU sides to validate a
 * telemetry_payload_t's integrity across the UART link (see
 * uart_send_telemetry_and_wait_ack() on the STM32 side and
 * uart_receiver_task()'s STATE_CRC_H case on the ESP32 side) - both must
 * call this exact function so their computed CRCs agree.
 *
 * @param data Pointer to the byte buffer to checksum (the telemetry
 * payload, not the whole frame - sync/length/crc fields are not covered).
 * @param length Number of bytes in `data` to checksum.
 * @return uint16_t Calculated CRC16 value.
 */
static inline uint16_t calculate_crc16(const uint8_t *data, uint16_t length) {
    /* CCITT-FALSE starts from an all-ones register instead of zero; this
     * is what distinguishes it from plain CRC-16/CCITT. */
    uint16_t crc = 0xFFFF;

    /* Process the buffer one byte at a time. */
    for (uint16_t i = 0; i < length; i++) {
        /* Mix the next input byte into the top 8 bits of the CRC register
         * via XOR - the standard "byte-at-a-time" CRC construction. */
        crc ^= (uint16_t)data[i] << 8;

        /* Shift the polynomial through the register one bit at a time,
         * 8 times per input byte (i.e. once per bit of that byte). */
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                /* Top bit was 1: shifting it out would lose information,
                 * so XOR in the CCITT polynomial (0x1021) to fold it back
                 * into the register - this is the actual "division" step
                 * of the CRC algorithm. */
                crc = (crc << 1) ^ 0x1021;
            } else {
                /* Top bit was 0: nothing to fold in, just shift. */
                crc <<= 1;
            }
        }
    }

    /* CCITT-FALSE has no final XOR, so the register value after processing
     * every byte is the final CRC as-is. */
    return crc;
}

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SPEC_H
