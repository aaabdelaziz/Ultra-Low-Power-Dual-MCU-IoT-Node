#!/usr/bin/env bash
set -euo pipefail

echo "Setup helper for STM32 and ESP32 toolchains (informational)."
echo
echo "STM32 (ARM GCC): Install arm-none-eabi toolchain from your package manager or vendor site."
echo "ESP32 (ESP-IDF): Install ESP-IDF and Python dependencies per Espressif docs."
echo
echo "This script documents steps; run interactively to follow instructions."

command -v arm-none-eabi-gcc >/dev/null 2>&1 || echo "arm-none-eabi-gcc not found in PATH"
command -v idf.py >/dev/null 2>&1 || echo "idf.py not found; source the ESP-IDF export script"

echo "Done. Update this script with specific installer commands for your platform."
#!/usr/bin/env bash
set -euo pipefail

echo "Setup script for STM32 and ESP32 toolchains (macOS)"

echo "Install Homebrew packages (you may be prompted):"
echo "  brew install cmake ninja openocd" || true

echo "ESP-IDF: follow official steps at https://docs.espressif.com/ if not installed"

echo "STM32 toolchain: install arm-none-eabi-gcc via Homebrew to cross-compile"
echo "  brew tap ArmMbed/homebrew-formulae && brew install arm-none-eabi-gcc" || true

echo "This script provides hints only. Use the official vendor docs to complete setup."
#!/bin/bash
set -e

echo "Starting Environment Setup for Dual-MCU IoT Telemetry Node (macOS)..."

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew could not be found. Please install it first from https://brew.sh/"
    exit 1
fi

echo "Updating Homebrew..."
brew update

echo "Installing CMake..."
brew install cmake || true

echo "Installing ARM GNU Toolchain (for STM32)..."
# Brew recently changed how they distribute the arm gcc toolchain.
brew install --cask gcc-arm-embedded || echo "Note: If the cask fails, you can install it manually from ARM Developer website."

echo "Installing Flashing Tools (OpenOCD and ST-Link)..."
brew install openocd stlink || true

echo "Installing Python and Git (Prerequisites for ESP-IDF)..."
brew install python git wget ninja dfu-util || true

# ESP-IDF Setup
IDF_INSTALL_DIR="$HOME/esp"
if [ ! -d "$IDF_INSTALL_DIR/esp-idf" ]; then
    echo "ESP-IDF not found. Downloading and installing ESP-IDF v5.2..."
    mkdir -p $IDF_INSTALL_DIR
    cd $IDF_INSTALL_DIR
    git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git
    cd esp-idf
    ./install.sh esp32
    echo "ESP-IDF installed successfully!"
else
    echo "ESP-IDF is already installed at $IDF_INSTALL_DIR/esp-idf"
fi

echo "================================================================"
echo "Setup Complete!"
echo "To build STM32 firmware, 'arm-none-eabi-gcc' and 'cmake' are now installed."
echo ""
echo "To build ESP32 firmware, you MUST export the ESP-IDF variables in your terminal by running:"
echo "    . $HOME/esp/esp-idf/export.sh"
echo "================================================================"
