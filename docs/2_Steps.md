Here are step-by-step implementation plan I made for designing the **Ultra-Low-Power Dual-MCU IoT Node** project, designed for maximum energy efficiency and robust telemetry tracking:

### Phase 1: Common Protocol Layer & Data Structures (`/common`)

* **Unified Telemetry Struct:** Design a standardized data structure containing fields such as device ID, battery voltage, temperature, actuator/motor state, and a message sequence number.
* **UART Framing Protocol:** Implement start/end markers (e.g., `0xAA` and `0x55`) combined with a checksum (CRC16) to ensure data integrity during transmission between the microcontrollers.

### Phase 2: STM32 Power Management & Low-Power Firmware (`/stm32_firmware`)

* **Clock Gating:** Initialize the system and disable clock signals to all unused peripherals to minimize active current consumption.
* **GPIO Leakage Mitigation:** Configure all unused GPIO pins to **Analog Mode** to prevent internal current leakage through floating pull-up/pull-down resistors.
* **Stop Mode Configuration:** Program the power control registers to put the STM32 into **Stop Mode**, setting up wake-up triggers via external GPIO interrupts or a low-power RTC timer.

### Phase 3: Background Data Acquisition (`ADC & DMA Pipeline`)

* **Low-Power ADC Configuration:** Set up the Analog-to-Digital Converter for efficient, low-frequency sampling of sensor and battery metrics.
* **DMA Circular Buffer:** Link the ADC to a Direct Memory Access (DMA) circular buffer in RAM so that sensor data is logged continuously in the background while the CPU remains in deep sleep.
* **Threshold Interrupts:** Program the system to wake the CPU only if a critical threshold (such as a sudden battery voltage drop) is crossed.

### Phase 4: Inter-MCU Handshake Protocol (`UART Handshake`)

* **Packet Assembly:** Once data is ready in RAM, wake the STM32 CPU to package the telemetry payload.
* **Wake-Up Pulse & Burst Transmission:** Send a hardware wake-up signal via a dedicated GPIO pin to alert the ESP32, then transmit the data packet over UART at a high baud rate to minimize active time.
* **Return to Sleep:** Immediately shut down the UART line and return the STM32 to deep sleep.

### Phase 5: ESP32 Cloud Connectivity & MQTT Pipeline (`/esp32_firmware`)

* **UART Listening:** Keep the ESP32 in a light sleep/listening state until the STM32 handshake signal triggers data reception.
* **Data Verification:** Validate the incoming packet's checksum and parse the telemetry structure.
* **Wi-Fi Burst & MQTT Publish:** Establish a brief Wi-Fi connection, format the data into a lightweight JSON payload, and publish it securely via **MQTT** to the cloud platform.
* **Sleep Mode Reset:** Drop the Wi-Fi connection and return the ESP32 to sleep immediately after publishing.