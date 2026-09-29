// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "doomgeneric.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CHUNK_LINES 10

// My GPIO setup on TinyS3
#define PIN_NUM_MOSI 35
#define PIN_NUM_MISO -1
#define PIN_NUM_SCLK 36
#define PIN_NUM_LCD_DC 7
#define PIN_NUM_LCD_CS 34
#define PIN_NUM_LCD_RST -1
#define PIN_NUM_BK_LIGHT -1

static const char *TAG = "st7789";
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static uint16_t *s_dma_bounce_buf = NULL;

extern uint16_t rgb565_palette[256];

void init_spiffs(void) {
    ESP_LOGI("spiffs", "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
      };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("spiffs", "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE("spiffs", "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE("spiffs", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0;
    size_t used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI("spiffs", "Partition size: total: %d, used: %d", total, used);
    }
}

void init_st7789_display(void) {
    ESP_LOGI(TAG, "Initializing SPI bus...");

    spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DOOMGENERIC_RESX * 10 * sizeof(uint16_t) + 8,
    };

    // SPI2_HOST is default for user SPI peripherals on ESP32-S3
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Installing panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000, // 40 MHz SPI clock
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Installing ST7789 driver...");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG, // ST7789 expects Big Endian smh
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &s_panel_handle));

    // Reset and initialize screen
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));

    // Set screen orientation and 135x240 window offsets inside the 240x320 controller memory
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel_handle, true)); // Landscape (240x135)
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, false, true));

    // Set column and row window offsets (40, 52 for landscape mode on 135x240 panel)
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel_handle, 40, 52));

    // Turn on display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    // Backlight GPIO setup
    // if (PIN_NUM_BK_LIGHT >= 0) {
    //     gpio_config_t bk_gpio_config = {
    //         .mode = GPIO_MODE_OUTPUT,
    //         .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    //     };
    //     gpio_config(&bk_gpio_config);
    //     gpio_set_level(PIN_NUM_BK_LIGHT, 1); // Turn backlight on
    // }

    // Allocate 10-line SRAM DMA Bounce Buffer (320 * 10 * 2 bytes = 6.4 KB)
    s_dma_bounce_buf = (uint16_t *)heap_caps_malloc(DOOMGENERIC_RESX * 10 * sizeof(uint16_t),
                                                    MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (!s_dma_bounce_buf) {
        ESP_LOGE(TAG, "Failed to allocate SRAM DMA bounce buffer!");
        return;
    }

    ESP_LOGI(TAG, "ST7789 Display initialized successfully!");
}

void DG_DrawFrame(void) {
    if (!s_panel_handle || !s_dma_bounce_buf || !DG_ScreenBuffer)
        return;

    pixel_t *src_8bit = DG_ScreenBuffer;

    for (int y = 0; y < DOOMGENERIC_RESY; y += CHUNK_LINES) {
        int dst_idx = 0;

        // Translate 8-bit palette indices -> 16-bit RGB565 in SRAM DMA buffer
        for (int line = 0; line < CHUNK_LINES; line++) {
            int line_offset = (y + line) * DOOMGENERIC_RESX;

            for (int x = 0; x < DOOMGENERIC_RESX; x++) {
                uint8_t palette_idx = src_8bit[line_offset + x];
                s_dma_bounce_buf[dst_idx++] = rgb565_palette[palette_idx];
            }
        }

        // Draw 10-line chunk to ST7789 via esp_lcd
        esp_lcd_panel_draw_bitmap(s_panel_handle, 0, y, DOOMGENERIC_RESX, y + CHUNK_LINES,
                                  s_dma_bounce_buf);
    }
}

void DG_Init(void) {
    // No idea what to do here
}

void DG_SleepMs(uint32_t ms) {
    if (ms == 0)
        return;
    vTaskDelay(pdMS_TO_TICKS(ms));
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
