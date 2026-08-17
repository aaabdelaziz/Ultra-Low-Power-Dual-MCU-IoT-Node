cmake_minimum_required(VERSION 3.16)
project(wifi_mqtt_client)

add_library(wifi_mqtt_client STATIC wifi_mqtt_client.cpp)
target_include_directories(wifi_mqtt_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
idf_component_register(SRCS "wifi_mqtt_client.cpp" INCLUDE_DIRS ".")
