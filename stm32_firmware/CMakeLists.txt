cmake_minimum_required(VERSION 3.16)
project(stm32_firmware C)

# Placeholder CMake for STM32 firmware; adapt to your cross-toolchain file.
add_executable(stm32_firmware
  main.c
  dma_adc.c
  power_manager.c
  uart_handshake.c
)

target_include_directories(stm32_firmware PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
cmake_minimum_required(VERSION 3.13)
project(stm32_firmware C)

add_executable(stm32_firmware main.c)
cmake_minimum_required(VERSION 3.16)

# Set the toolchain for cross-compiling
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Define the compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

project(stm32_telemetry_node C CXX ASM)

# Optimization and MCU specific flags (Generic Cortex-M4 example)
# Modify these flags for your specific STM32 family (e.g. -mcpu=cortex-m0plus for STM32G0/L0)
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -O3 -Wall -Wextra -fdata-sections -ffunction-sections")

# Define target executable
set(TARGET main)

# Include directories
include_directories(drivers/include)
include_directories(middleware/include)
include_directories(../common)

# Source files
file(GLOB_RECURSE SOURCES 
    "app/src/*.c"
    "drivers/src/*.c"
    "middleware/src/*.c"
)

add_executable(${TARGET}.elf ${SOURCES})

# Note: A real project would require a linker script (.ld) and startup assembly file (.s).
# set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32_FLASH.ld)
# set(CMAKE_EXE_LINKER_FLAGS "-Wl,-gc-sections --specs=nano.specs -T ${LINKER_SCRIPT}")

# Generate .bin and .hex
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET}.elf ${TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET}.elf ${TARGET}.bin
    COMMAND ${CMAKE_SIZE} ${TARGET}.elf
)
