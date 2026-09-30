// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

#include "doomgeneric.h"
#include "i_system.h"
#include "m_argv.h"
#include "esp_heap_caps.h"
#include <stdio.h>

pixel_t *DG_ScreenBuffer = NULL;

void D_DoomMain(void);

void doomgeneric_Create(int argc, char **argv) {
    myargc = argc;
    myargv = argv;

    // I am forcing ESP32 to put the screen buffer onto SRAM, because I suspect
    // that a buffer placed in PSRAM won't be as performant and frictionless for SPI screen.
    // Edit: A single contiguous block of memory must be allocated beforehand.
    DG_ScreenBuffer = (pixel_t*) heap_caps_malloc(
        DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(pixel_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    DG_Init();
    D_DoomMain();
}
