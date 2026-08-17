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
 * @brief Calculate CRC16 (CCITT-FALSE)
 * 
 * @param data Pointer to data buffer
 * @param length Length of data
 * @return uint16_t Calculated CRC
 */
static inline uint16_t calculate_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SPEC_H
