# Ultra-Low-Power Dual-MCU IoT Telemetry Node

A production-ready, ultra-low-power firmware architecture for smart IoT nodes (such as smart thermostats and valve controllers), utilizing a dual-MCU setup (STM32 & ESP32) to achieve sub-10 µA standby current drain and multi-year battery lifecycles.

---

## 🏗️ System Architecture

```mermaid
graph TB
    subgraph Power_Supply ["Power & Energy Subsystem"]
        Battery["Battery Source"]
        Regulator["LDO / Power Control"]
    end

    subgraph STM32_Node ["STM32 MCU (Ultra-Low-Power Core)"]
        StopMode["Stop Mode & Clock Gating"]
        GPIO["Unused GPIOs (Analog Mode)"]
        Sensors["Sensors & Battery AFE"]
        ADC["ADC Peripheral"]
        DMA["DMA Circular Buffer in RAM"]
        WakeUp["Wake-Up Interrupt"]
        UART_TX["UART Transmitter"]

        Sensors -->|Continuous Sampling| ADC
        ADC -->|Direct Memory Access| DMA
        DMA -->|Threshold Exceeded| WakeUp
        WakeUp --> UART_TX
    end

    subgraph ESP32_Node ["ESP32 MCU (Wireless Gateway)"]
        UART_RX["UART Receiver & Parser"]
        JSON["JSON Formatter"]
        WiFi["Wi-Fi / MQTT Client"]

        UART_RX --> JSON
        JSON --> WiFi
    end

    subgraph Cloud_Infrastructure ["Cloud Platform"]
        Broker["MQTT Broker"]
        Dashboard["Dashboard"]
    end

    Battery --> Regulator
    Regulator --> STM32_Node
    Regulator --> ESP32_Node
    
    UART_TX -->|Secure UART Packet| UART_RX
    WiFi -->|MQTT Publish via TLS| Broker
    Broker --> Dashboard