# Followed Design Implementation Plan — Telemetry Node

1. Create firmware skeletons
   - `esp32_firmware/` with `uart_receiver` and `wifi_mqtt_client` components.
   - `stm32_firmware/` with power, ADC/DMA, and UART handshake modules.

2. Implement UART framed protocol
   - Define frame format and CRC16 algorithm (test vectors).

3. Implement STM32 low-power sampling loop
   - Use DMA circular buffer, aggregate samples, and wake/handshake logic.

4. Implement ESP32 parsing and MQTT publishing
   - Ensure robust reconnection and minimal memory allocations.

5. Integration and testing
   - Unit test CRC implementation.
   - Hardware-in-loop tests with serial connection.

Open questions:
- Target STM32 part/Toolchain details? (cubeMX, HAL, or LL?)
- Preferred MQTT broker and topic structure.

Implementation tasks (concrete):

- Create `esp32_firmware/` skeleton and CMake build file.
- Implement `uart_receiver` FreeRTOS task that assembles frames and validates CRC16.
- Implement `wifi_mqtt_client` that connects to SSID and publishes MQTT JSON payloads.
- Create `stm32_firmware/` skeleton with CMake, `dma_adc`, `power_manager`, and `uart_handshake` modules.
- Add a `setup_env.sh` helper to document and bootstrap toolchain installs.

Acceptance criteria:
- Each firmware directory builds (placeholder CMake) and contains task entry points.
- UART framing unit tests or test vectors are documented in `docs/`.
