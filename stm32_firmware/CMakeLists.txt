cmake_minimum_required(VERSION 3.16)

# Cross-compile toolchain: must be set before project() so CMake's compiler
# detection uses arm-none-eabi-* instead of the host compiler.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# CMake's default "can this compiler build an executable" check links a
# trivial test program, which fails for a bare-metal target with no OS,
# no _start, and no linker script yet. Skip that check; the real .elf
# link further down proves the toolchain works instead.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32_telemetry_node C ASM)

set(TARGET main)

# STM32L476RG (Cortex-M4F). See CMSIS/Device/ST/STM32L4xx/Include/stm32l4xx.h
# for the full list of devices this header supports if the target changes.
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -DSTM32L476xx -O2 -g -Wall -Wextra -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp")

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32L476RGTX_FLASH.ld)
set(CMAKE_EXE_LINKER_FLAGS
    "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs -Wl,-Map=${TARGET}.map"
)

include_directories(
    drivers/include
    middleware/include
    ../common
    CMSIS/Include
    CMSIS/Device/ST/STM32L4xx/Include
)

# Application/driver/middleware sources (the project's own code).
file(GLOB_RECURSE APP_SOURCES
    "app/src/*.c"
    "drivers/src/*.c"
    "middleware/src/*.c"
)

# CMSIS device support: reset/clock init (system_stm32l4xx.c) and the
# vector table + Reset_Handler (startup_stm32l476xx.s). Both are vendored
# from STMicroelectronics' cmsis_device_l4 pack, not project-original code.
set(CMSIS_SOURCES
    CMSIS/Device/ST/STM32L4xx/Source/Templates/system_stm32l4xx.c
    CMSIS/Device/ST/STM32L4xx/Source/Templates/gcc/startup_stm32l476xx.s
)

add_executable(${TARGET}.elf ${APP_SOURCES} ${CMSIS_SOURCES})

add_custom_command(TARGET ${TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET}.elf ${TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET}.elf ${TARGET}.bin
    COMMAND ${CMAKE_SIZE} ${TARGET}.elf
)
