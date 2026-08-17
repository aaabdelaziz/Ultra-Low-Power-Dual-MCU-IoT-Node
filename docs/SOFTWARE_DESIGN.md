# Software Design — Ultra-Low-Power Dual-MCU IoT Telemetry Node

Overview
--------
This project pairs an ultra-low-power STM32 MCU (sampling sensor data and performing low-power aggregation) with an ESP32 co-processor (wireless transport and cloud integration). The STM32 is responsible for deterministic, low-power ADC sampling and waking the ESP32 only when telemetry is ready. The ESP32 runs FreeRTOS, parses incoming frames from UART, and publishes telemetry over Wi‑Fi/MQTT.

Architecture
------------
- STM32 (`stm32_firmware/`): ADC + DMA sampling, power manager, UART handshake.
- ESP32 (`esp32_firmware/`): `uart_receiver` task, `wifi_mqtt_client` task, main application.
- Communication: framed UART protocol with start byte, length, payload, CRC16.

Data Flow
---------
1. STM32 samples ADC using DMA into a circular buffer.
2. On aggregation or threshold, STM32 performs a UART handshake with ESP32 and transmits one or more telemetry frames.
3. ESP32 `uart_receiver` task reads bytes from UART, decodes frames via a small state machine, validates CRC, and forwards parsed telemetry to a FreeRTOS queue.
4. `wifi_mqtt_client` subscribes to the telemetry queue, formats JSON payloads, and publishes them to the configured MQTT topic.

STM32 Initialization & Operational Loop
---------------------------------------
- Initialize clocks, GPIO, ADC, DMA, and UART.
- Configure ADC with DMA circular buffer; use a small aggregation window to reduce wakeups.
- Main loop (low-power):
  - Enter STOP or SLEEP modes.
  - Wake on timer or external event, read aggregated samples.
  - If telemetry is ready, assert a GPIO or send a short handshake byte sequence over UART.
  - Transmit framed telemetry (start, len, payload, CRC16).
  - Return to low-power mode.

ESP32 FreeRTOS Architecture
---------------------------
- `uart_receiver` task:
  - Runs at moderate priority.
  - Reads UART bytes non-blocking, runs a state machine to assemble frames, validates CRC16, and pushes parsed telemetry to a queue.
- `wifi_mqtt_client` task:
  - Responsible for Wi‑Fi connection and MQTT lifecycle.
  - Subscribes to the telemetry queue and publishes messages as JSON.
  - Handles reconnect/backoff and minimal dynamic allocations.
- Main initializes the UART, queue, and creates both tasks.

Operational Considerations
--------------------------
- Keep frames small and deterministic to minimize ESP32 awake time.
- Use CRC16 for frame integrity; keep the CRC implementation small and testable.
- Prefer static buffers and preallocated queues on ESP32 to avoid heap fragmentation.
- STM32 should minimize time out of low-power states — handshake must be quick and idempotent.

Open Questions
--------------
- Exact STM32 target and HAL vs LL choice.
- MQTT broker and credentials handling strategy (certificate vs username/password).
## Software Design — Ultra-Low-Power Dual-MCU IoT Telemetry Node

This document outlines the architecture, data flow, and operational logic for the STM32 (low-power sensor node) and ESP32 (connectivity gateway) components.

### High-level architecture

- STM32: low-power MCU responsible for sensor sampling, ADC via DMA, power management, and a UART-based telemetry handshake.
- ESP32: Wi‑Fi + MQTT gateway running FreeRTOS. Receives framed telemetry over UART, validates frames, and publishes telemetry to a broker.
- Communication: Simple framed UART protocol with SYNC bytes, LENGTH, PAYLOAD, CRC16 and an ACK byte (0x06).

## STM32 Firmware

### Initialization (main.c)
- `pm_init()` — configure clocks, set unused GPIOs to analog, enable wake sources.
- `dma_adc_init()` — configure ADC channels and DMA in circular mode.
- `uart_handshake_init()` — configure UART (baud, pins, RX/TX) and wake GPIO.
- `dma_adc_start()` — start continuous conversions into DMA buffer.

### Operational loop
- The main loop is event-driven and targets lowest-power operation: enter STOP (or Standby) and wake on DMA/RTC/EXTI.
- On DMA buffer completion interrupt:
  - Set `data_ready_flag` and wake the main context.
  - Read averaged ADC samples via `dma_adc_get_averaged_channel()`.
  - Build `telemetry_payload_t` (timestamp, sample(s), status, optional sensors).
  - Call `uart_wake_esp32()` to ensure the ESP32 is awake.
  - Call `uart_send_frame_and_wait_ack()` which:
    - Builds a frame: SYNC1, SYNC2, LENGTH, PAYLOAD, CRC16 (LSB, MSB).
    - Sends bytes over UART.
    - Waits with timeout for `0x06` (ACK) from ESP32; retries N times if needed.
- After successful transmission or failure handling, clear the flag and return to low-power.

### Key STM32 modules
- `dma_adc.c/h` — ADC and DMA setup, buffering, averaging helpers.
- `power_manager.c/h` — configure STOP modes, wake pins, GPIO power states.
- `uart_handshake.c/h` — UART frame builder, CRC16, transmit with ACK handling.

## ESP32 Firmware (FreeRTOS)

### Overall design
- `app_main()` initializes NVS, Wi‑Fi, MQTT client, and creates two main tasks:
  - `uart_receiver_task`: parses incoming UART frames and pushes validated payloads to a queue.
  - `mqtt_publisher_task`: waits on the queue, formats payloads to JSON, and publishes via MQTT.

### uart_receiver_task
- Uses the IDF UART driver in blocking or event-driven mode.
- Implements a byte-wise state machine: SYNC1 -> SYNC2 -> LEN -> PAYLOAD -> CRC.
- On valid CRC: sends ACK (0x06) back to STM32 and posts the payload to `xQueueSend()`.

### wifi_mqtt_client / mqtt_publisher_task
- Handles Wi‑Fi connection lifecycle including reconnection/backoff.
- Maintains an `esp_mqtt_client_handle_t` and publishes telemetry as compact JSON.
- Uses `esp_mqtt_client_publish()` with QoS 0 or 1 depending on configuration.

## Files and Components (what was added)

- `docs/SOFTWARE_DESIGN.md` — this document (expanded and clarified).
- `docs/1_implementation_plan.md` — project tasks and open questions.
- `esp32_firmware/` — CMake project with `main.cpp`, `uart_receiver/`, `wifi_mqtt_client/` components.
- `stm32_firmware/` — CMake project with `main.c`, `dma_adc`, `power_manager`, and `uart_handshake` modules.
- `setup_env.sh` — convenience script to prepare toolchains and environment variables.

## Operational notes and robustness

- UART: use hardware RTS/CTS if available; otherwise implement software wake/pulse with a GPIO.
- CRC: use CRC-16-IBM (polynomial 0x8005) or CRC-16-CCITT depending on interoperability needs.
- Retries: STM32 should backoff exponentially when no ACK is received.

## Extensibility

- Adding sensors to STM32: place drivers under `stm32_firmware/drivers/` and extend `telemetry_payload_t`.
- Adding cloud features on ESP32: implement new modules in `esp32_firmware/components/` and register topic/subscriptions via `wifi_mqtt_client`.

## Open questions

- Desired MQTT QoS and authentication strategy (anonymous broker vs. token-based auth).
- Expected telemetry interval and acceptable latency (influences power strategy).

---
If you'd like, I can now create the CMake projects, skeleton source files, and a setup script. Tell me if you want specific Wi‑Fi/MQTT broker details or STM32 target MCU specifics.
