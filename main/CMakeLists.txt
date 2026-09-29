# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

idf_component_register(
    SRCS
        main.c
        doomgeneric_esp32.c
    REQUIRES
        esp_lcd
        esp_driver_spi
        esp_driver_gpio
        esp_timer
        esp_psram
        spiffs
    PRIV_REQUIRES doom log
)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
