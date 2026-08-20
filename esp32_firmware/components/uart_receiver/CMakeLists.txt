idf_component_register(SRCS "uart_receiver.cpp"
                    INCLUDE_DIRS "include" "../../../common"
                    # ESP-IDF v6 split the old monolithic "driver" component
                    # into per-peripheral components; uart_receiver.cpp
                    # includes driver/uart.h and driver/gpio.h, so it needs
                    # both directly (declaring only "driver" no longer pulls
                    # them in transitively).
                    PRIV_REQUIRES esp_driver_uart esp_driver_gpio)
