# Ultra-Low-Power Dual-MCU IoT Telemetry Node

This plan outlines the implementation of a full Git repository from scratch for the Dual-MCU (STM32 & ESP32) telemetry node as specified. 

## User Review Required
Please review the proposed file structure and architecture below. Once approved, I will generate all the source code files and configuration directly into the workspace `/Users/ahmedabdelaziz/MyBrain/Topics/SW/GitRepo/IOT/STM32_ultra_lowpower_control`.

## Open Questions
- Do you have a specific STM32 family in mind (e.g., STM32L4, STM32G0)? If not, I will write the code using standard CMSIS register-level approaches or a generic STM32 HAL-like structure adaptable to most STM32L series.
- For the ESP32, I will use the standard ESP-IDF framework conventions. Is that acceptable, or would you prefer PlatformIO/Arduino structure?

## Proposed Changes

### Common Protocol
#### [NEW] common/protocol_spec.h
Defines the UART framing protocol between STM32 and ESP32. Includes magic sync bytes, packet length, payload struct (sensor data), and CRC8/16.

---

### STM32 Firmware (Bare-Metal / Low Power)
#### [NEW] stm32_firmware/CMakeLists.txt
CMake configuration for cross-compiling with `arm-none-eabi-gcc`.
#### [NEW] stm32_firmware/include/power_manager.h
#### [NEW] stm32_firmware/src/power_manager.c
Implements clock gating, configuring unused GPIOs to Analog to prevent leakage, and entering deep sleep (STOP mode) using WFI.
#### [NEW] stm32_firmware/include/dma_adc.h
#### [NEW] stm32_firmware/src/dma_adc.c
Configures the ADC to run with DMA in circular mode, allowing data collection while the CPU sleeps.
#### [NEW] stm32_firmware/include/uart_handshake.h
#### [NEW] stm32_firmware/src/uart_handshake.c
Implements the STM32 side of the UART protocol. It will wake up the ESP32 (e.g., via a GPIO toggle or UART break), send the data, and wait for an ACK.
#### [NEW] stm32_firmware/src/main.c
The main entry point. Sets up peripherals, starts DMA, and enters a low-power sleep loop, waking up only when a full buffer is ready to transmit.

---

### ESP32 Firmware (Connectivity)
#### [NEW] esp32_firmware/CMakeLists.txt
ESP-IDF project CMake file.
#### [NEW] esp32_firmware/main/CMakeLists.txt
Component CMake file.
#### [NEW] esp32_firmware/main/uart_receiver.h
#### [NEW] esp32_firmware/main/uart_receiver.cpp
ESP-IDF UART driver setup, implementing a state machine to parse incoming packets from the STM32 based on `protocol_spec.h`.
#### [NEW] esp32_firmware/main/wifi_mqtt_client.h
#### [NEW] esp32_firmware/main/wifi_mqtt_client.cpp
Handles Wi-Fi connection and MQTT client initialization. Connects to a broker and publishes the parsed telemetry data.
#### [NEW] esp32_firmware/main/main.cpp
FreeRTOS application entry point. Spawns tasks for UART reception and MQTT publishing, utilizing queues for inter-task communication.

---

### Documentation
#### [NEW] README.md
Comprehensive documentation on architecture, low-power strategies, current optimization, and setup/build instructions for both microcontrollers.

## Verification Plan

### Automated Tests
- While running full hardware-in-the-loop tests requires physical hardware, the code will be structurally validated and cross-compiled (if toolchains are available) to ensure syntax and build correctness.

### Manual Verification
- Review the generated code to ensure MISRA alignment, proper register-level/HAL usage, and DMA memory alignment requirements.
- The user will compile and flash the respective firmwares using standard STM32 and ESP-IDF tools.
