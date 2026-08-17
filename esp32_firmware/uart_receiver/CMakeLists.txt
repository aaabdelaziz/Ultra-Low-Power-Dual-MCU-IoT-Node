cmake_minimum_required(VERSION 3.16)
project(uart_receiver)

add_library(uart_receiver STATIC uart_receiver.cpp)
target_include_directories(uart_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
idf_component_register(SRCS "uart_receiver.cpp" INCLUDE_DIRS ".")
