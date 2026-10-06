# SPDX-FileCopyrightText: 2026 Danylo Shamaiev
# SPDX-License-Identifier: GPL-3.0-or-later

idf_component_register(
    SRCS
        main.c
        doomgeneric_esp32.c
        display_st7789.c
        storage_spiffs.c
        input_usb_hid.c
        hid_report.c
    REQUIRES
        esp_lcd
        esp_driver_spi
        esp_driver_gpio
        esp_timer
        esp_psram
        spiffs
        usb_host_hid
    PRIV_REQUIRES doom log
    # Doom calls DG_* hooks that app_main does not reference directly.
    WHOLE_ARCHIVE
)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
