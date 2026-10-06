# Display code

The hardware code is split up like this:

- `main/main.c` reserves the framebuffer, starts things up, and runs the game loop.
- `main/doomgeneric_esp32.c` has the `DG_*` hooks for drawing, timing, and input.
- `main/storage_spiffs.c` mounts the SPIFFS partition with the WAD in it.
- `main/input_usb_hid.c` reads the USB keyboard; `main/hid_report.c` decodes its reports.
- `main/display_st7789.c` handles the LCD: pins, SPI speed, panel settings,
  rotation, pixel conversion, and the DMA buffer.

## How drawing works

`main/display.h` has just two functions. It doesn't need Doom or ESP-IDF headers.

Startup calls `display_init(width, height)` once. Then `DG_DrawFrame()` calls
`display_draw_frame(framebuffer, palette)` whenever Doom has a frame ready.

The framebuffer has one byte per pixel. That byte is an index into the palette,
not a color. Pixels go left to right, then top to bottom, with no gaps between
rows.

The palette has 256 `uint16_t` RGB565 colors. Red uses bits 11–15, green uses
5–10, and blue uses 0–4. The driver deals with whatever format and byte order
the LCD needs.

Finish reading the frame and palette before returning. Doom can change both
for the next frame, so don't leave DMA reading them in the background.

The ST7789 driver converts and sends ten rows at a time, or fewer at the bottom.
It waits for each transfer before reusing the DMA buffer. At 240 × 135, that
buffer is 4,800 bytes. The 32,400-byte game framebuffer stays in internal SRAM.

## Want a different LCD?

1. Write a new driver file with the two functions from `main/display.h`.
   Put its pins, setup, and drawing code there.
2. Swap `display_st7789.c` for your file in `main/CMakeLists.txt`. Build one
   display driver at a time. Change the ESP-IDF dependencies there if needed.
3. If the size changes, update `DOOMGENERIC_RESX` and `DOOMGENERIC_RESY` in
   `components/doom/include/doomgeneric.h`. Startup passes those sizes to the
   driver. Doom scales its output to match.

The `DG_*` hooks and SPIFFS code can stay as they are.
