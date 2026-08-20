cmake_minimum_required(VERSION 3.16)

# Standard ESP-IDF project bootstrap. This must run *before* project() below:
# it sets CMAKE_TOOLCHAIN_FILE to ESP-IDF's xtensa cross-compiler toolchain
# file so the upcoming project() call configures for the target MCU instead
# of the host compiler. (A previous version of this file called a stray
# `project(esp32_firmware)` here first, which locked in the host AppleClang
# compiler into the CMake cache before this include ever ran - that bug is
# what this ordering avoids.)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32_telemetry_node)
