# System Architecture: Ultra-Low-Power Dual-MCU IoT Telemetry Node

This document provides a detailed breakdown of the hardware components and the architectural flow of the dual-MCU system.

## 1. Hardware Overview

### MCU 1: STM32 (Data Acquisition & Power Management)
- **Primary Role:** Ultra-low-power edge sensing, bare-metal peripheral control, and aggressive power states management.
- **Key Features Used:** 
  - **ADC (Analog-to-Digital Converter):** Used for reading sensor data (e.g., battery voltage, temperature).
  - **DMA (Direct Memory Access):** Configured in circular mode to transfer ADC samples directly to SRAM without waking the CPU.
  - **UART (Universal Asynchronous Receiver-Transmitter):** Used to send formatted payload frames to the ESP32.
  - **Low-Power Modes:** Heavily utilizes STOP mode (Deep Sleep) to achieve sub-10 µA current consumption when not transmitting.

### MCU 2: ESP32 (Connectivity & Gateway)
- **Primary Role:** Wireless connectivity, local buffering, and secure cloud communication.
- **Key Features Used:**
  - **Wi-Fi Subsystem:** Connects to the local network to route data to the internet.
  - **MQTT Client (via ESP-IDF):** Handles publishing telemetry payloads to a remote MQTT broker.
  - **FreeRTOS:** Utilizes concurrent tasks (`uart_receiver_task` and `mqtt_publisher_task`) and a Thread-Safe Queue to buffer incoming UART data before transmission.

### Interconnects
- **UART TX/RX:** Bi-directional communication (STM32 TX -> ESP32 RX, ESP32 TX -> STM32 RX) for telemetry frames and ACKs.
- **WAKE Pin (Optional but recommended):** A dedicated GPIO line from STM32 to ESP32 to pull the ESP32 out of deep sleep before a UART transmission.
- **Ground (GND):** Common ground reference between both MCUs.
- **Logic Level Shifting:** If the STM32 is running at a different voltage than the ESP32 (e.g., STM32 at 1.8V and ESP32 at 3.3V), a logic level shifter is required on the UART and WAKE lines.

---

## 2. System Block Diagram

```mermaid
graph LR
    subgraph STM32 [STM32 Subsystem (Ultra-Low Power)]
        Sensors[Sensors / Battery] -->|Analog| ADC[ADC Peripheral]
        ADC -->|Hardware DMA| SRAM[(SRAM Circular Buffer)]
        SRAM -->|Data Ready| CPU[STM32 CPU Core]
        CPU -->|Pack & CRC| UART1[UART TX/RX]
    end

    subgraph ESP32 [ESP32 Subsystem (Connectivity)]
        UART2[UART RX/TX] -->|Parse & CRC| RX_Task[UART RX Task]
        RX_Task -->|Queue Push| Queue[(FreeRTOS Queue)]
        Queue -->|Queue Pop| MQTT_Task[MQTT Publisher Task]
        MQTT_Task -->|Wi-Fi| Cloud((MQTT Broker))
    end

    UART1 <-->|Telemetry Frame & ACK| UART2
    CPU -->|Wake Signal| ESP32
```

---

## 3. Architecture Flow

### State 1: Deep Sleep & Background Acquisition
1. **STM32** is in **STOP mode**. The CPU clock is gated off. 
2. The ADC is triggered by a hardware timer to sample analog channels.
3. DMA transfers the ADC conversions directly into SRAM.
4. **ESP32** is in **Modem Sleep** or **Light Sleep**, maintaining its Wi-Fi connection with minimal power.

### State 2: Data Ready & Wakeup
1. Once the DMA circular buffer fills (or half-fills, depending on configuration), it fires a hardware interrupt.
2. The STM32 CPU wakes up from STOP mode.
3. The CPU calculates the averages of the sampled channels and populates the `telemetry_payload_t` C-struct.
4. The STM32 toggles the **WAKE Pin** to ensure the ESP32 is ready to receive data.

### State 3: UART Handshake & Transmission
1. STM32 calculates a CRC16 for the payload and constructs a complete `uart_frame_t`.
2. STM32 transmits the frame over UART.
3. ESP32's `uart_receiver_task` reads the incoming bytes, synchronizing on the magic bytes (`0xAA`, `0x55`).
4. ESP32 parses the payload, computes its own CRC16, and compares it against the received CRC.
5. If the CRC matches, ESP32 sends an `ACK` byte (`0x06`) back to the STM32. If it fails, it sends a `NACK` byte (`0x15`).

### State 4: Publishing & Return to Sleep
1. **STM32 Side:** Upon receiving the `ACK`, the STM32 clears its data-ready flags, gates its active clocks, and immediately re-enters STOP mode.
2. **ESP32 Side:** The `uart_receiver_task` pushes the validated payload into a FreeRTOS Queue.
3. The `mqtt_publisher_task` wakes up, pops the payload from the queue, formats it into a JSON string, and publishes it via MQTT.
4. Once the queue is empty, the ESP32 returns to its low-power sleep state.

## 4. Layered Software Design

To ensure separation of concerns, scalability, and ease of adding new device drivers over time, the firmware utilizes a strict layered architecture:

### STM32 Layered Structure
- **Application Layer (`app/`)**: High-level business logic and event loops (`main.c`). It dictates *when* things happen.
- **Middleware Layer (`middleware/`)**: Protocol handling, state machines, and data formatting. It bridges hardware and logic (e.g., `uart_handshake`).
- **Drivers Layer (`drivers/`)**: Hardware Abstraction Layer (HAL) interfacing directly with MCU registers (e.g., `dma_adc`, `power_manager`). Future sensors and I2C/SPI devices will be added here without affecting the application logic.

### ESP32 Layered Structure
- **Application Layer (`main/`)**: The main FreeRTOS application entry point.
- **Components (`components/`)**: Self-contained modular driver/middleware packages (e.g., `uart_receiver`, `wifi_mqtt_client`) that can be plugged or unplugged easily via CMake.
