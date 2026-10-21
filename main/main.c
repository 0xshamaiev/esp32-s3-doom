// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "doomgeneric.h"
#include "display.h"
#include "input.h"
#include "storage.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr size_t framebuffer_size =
    DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(*DG_ScreenBuffer);
// Doom's static arrays fill most of the free internal SRAM, and the LCD driver only
// reads this buffer from the CPU, so PSRAM works for it
static constexpr uint32_t framebuffer_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;

// FreeRTOS keeps splitting the memory frivolously, so I am gonna pin it beforehand
static void __attribute__((constructor)) reserve_framebuffer(void) {
    ESP_EARLY_LOGI("doom", "Attempting to reserve a contiguous %u-byte framebuffer in PSRAM",
                   (unsigned)framebuffer_size);
    DG_ScreenBuffer = heap_caps_malloc(framebuffer_size, framebuffer_caps);
}

void app_main(void) {
    if (DG_ScreenBuffer == NULL) {
        ESP_LOGE("doom", "Cannot reserve %zu-byte framebuffer: PSRAM free=%zu, largest=%zu",
                 framebuffer_size, heap_caps_get_free_size(framebuffer_caps),
                 heap_caps_get_largest_free_block(framebuffer_caps));
        heap_caps_print_heap_info(framebuffer_caps);
        return;
    }
    ESP_LOGI("doom", "Reserved contiguous %zu-byte framebuffer in PSRAM at %p",
             framebuffer_size, (void *)DG_ScreenBuffer);

    ESP_LOGI("doom", "ESP32-S3 Doom firmware ready (%dx%d framebuffer)",
             DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    ESP_LOGI("doom", "Game startup awaits display, input, WAD storage, and memory configuration");

    storage_init();
    display_init(DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    input_init();

    // Idk how to bolt on a proper debug shell here, so I am gonna hardcode arguments
    char *argv[] = {
        "doom",
        "-iwad",
        "/spiffs/doom1.wad",
        "-nosound",
    };
    uint8_t argc = sizeof(argv) / sizeof(argv[0]);

    doomgeneric_Create(argc, argv);

    while (true) {
        doomgeneric_Tick();
        vTaskDelay(1);
    }
}
