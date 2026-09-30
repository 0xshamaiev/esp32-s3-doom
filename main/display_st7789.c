// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "display.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#define CHUNK_LINES 10
#define LCD_SPI_CLOCK_HZ (40 * 1000 * 1000)

// My GPIO setup on TinyS3
#define PIN_NUM_MOSI 35
#define PIN_NUM_MISO -1
#define PIN_NUM_SCLK 36
#define PIN_NUM_LCD_DC 1
#define PIN_NUM_LCD_CS 34
#define PIN_NUM_LCD_RST 2

static const char *TAG = "st7789";
static esp_lcd_panel_io_handle_t s_panel_io = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static uint16_t *s_dma_bounce_buf = NULL;

static int s_frame_width;
static int s_frame_height;

static int chunk_rows(int y) {
    const int remaining = s_frame_height - y;
    return remaining < CHUNK_LINES ? remaining : CHUNK_LINES;
}

static void draw_chunk(int y, int rows) {
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel_handle, 0, y,
                                             s_frame_width, y + rows, s_dma_bounce_buf));
    // A parameter transfer waits for queued color transfers. With command -1
    // and no parameters, this only drains the queue so the buffer can be reused.
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(s_panel_io, -1, NULL, 0));
}

static void configure_waveshare_panel(void) {
    // Panel settings used by Waveshare's 1.14-inch ST7789 driver
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0xC0, (uint8_t[]){0x2C}, 1)); // LCM control
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0xC2, (uint8_t[]){0x01}, 1)); // Enable VDV/VRH settings
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0xC3, (uint8_t[]){0x12}, 1)); // VRH voltage setting
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0xC4, (uint8_t[]){0x20}, 1)); // VDV voltage setting
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0xC6, (uint8_t[]){0x0F}, 1)); // Frame rate control
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(
        s_panel_io, 0x13, NULL, 0)); // Normal display mode
}

void display_init(int width, int height) {
    s_frame_width = width;
    s_frame_height = height;

    const size_t dma_buffer_size = s_frame_width * CHUNK_LINES * sizeof(*s_dma_bounce_buf);
    const spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = dma_buffer_size + 8,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_SPI_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &s_panel_io));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_panel_io, &panel_config, &s_panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    configure_waveshare_panel();

    // Landscape window within the controller's 240x320 RAM
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel_handle, 40, 52));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    s_dma_bounce_buf = heap_caps_malloc(dma_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (!s_dma_bounce_buf) {
        ESP_LOGE(TAG, "Failed to allocate SRAM DMA bounce buffer!");
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    ESP_LOGI(TAG, "ST7789 ready: %dx%d RGB565 at %d MHz",
             s_frame_width, s_frame_height, LCD_SPI_CLOCK_HZ / 1000000);
}

void display_draw_frame(const uint8_t *framebuffer, const uint16_t palette[256]) {
    if (!s_panel_handle || !s_dma_bounce_buf || !framebuffer)
        return;

    for (int y = 0; y < s_frame_height; y += CHUNK_LINES) {
        const int rows = chunk_rows(y);
        int dst_idx = 0;

        // Translate 8-bit palette indices -> 16-bit RGB565 in SRAM DMA buffer
        for (int line = 0; line < rows; line++) {
            int line_offset = (y + line) * s_frame_width;

            for (int x = 0; x < s_frame_width; x++) {
                uint8_t palette_idx = framebuffer[line_offset + x];
                s_dma_bounce_buf[dst_idx++] = palette[palette_idx];
            }
        }

        draw_chunk(y, rows);
    }
}

