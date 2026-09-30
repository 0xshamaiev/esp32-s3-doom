// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "doomgeneric.h"
#include "display.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Be aware that many displays expect different palettes
extern uint16_t rgb565_palette[256];

void DG_DrawFrame(void) {
    display_draw_frame(DG_ScreenBuffer, rgb565_palette);
}

void DG_Init(void) {
    // app_main initializes the display before starting Doom.
}

void DG_SleepMs(uint32_t ms) {
    if (ms == 0)
        return;

    // Round any positive request to at least 1 tick of sleep
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if (ticks == 0)
        ticks = 1;

    vTaskDelay(ticks);
}

uint32_t DG_GetTicksMs(void) {
    // ESP-IDF Free RTOS uses 10ms ticks (100 Hz), which is too much for DOOM.
    // esp_timer_get_time() returns time in microseconds (us) since boot.
    // Divide by 1000 to convert to milliseconds (ms).
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int DG_GetKey(int *pressed, uint8_t *key) {
    // Stubbed: No buttons pressed for now
    *pressed = 0;
    *key = 0;
    return 0;
}
