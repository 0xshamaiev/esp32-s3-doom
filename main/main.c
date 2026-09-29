// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "doomgeneric.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern void init_spiffs(void);
extern void init_st7789_display(void);

void app_main(void) {
    ESP_LOGI("doom", "ESP32-S3 Doom firmware ready (%dx%d framebuffer)",
             DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    ESP_LOGI("doom", "Game startup awaits display, input, WAD storage, and memory configuration");

    init_spiffs();
    init_st7789_display();

    // Idk how to bolt on a proper debug shell here, so I am gonna hardcode arguments
    char *argv[] = {
        "doom",
        "-iwad",
        "/spiffs/doom1.wad",
        "-nosound",
        "-demo"
    };
    uint8_t argc = sizeof(argv) / sizeof(argv[0]);

    doomgeneric_Create(argc, argv);

    while (true) {
        doomgeneric_Tick();
        vTaskDelay(1);
    }
}
