// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

#include "i_video.h"
#include "config.h"
#include "d_event.h"
#include "d_main.h"
#include "i_system.h"
#include "v_video.h"
#include "z_zone.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "doomkeys.h"
#include "tables.h"

#include "doomgeneric.h"

bool palette_changed;
struct color colors[256];

void I_GetEvent(void);

// The screen buffer; this is modified to draw things to the screen
uint8_t *I_VideoBuffer = NULL;

// Gamma correction level to use
int usegamma = 0;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} col_t;

// Palette converted to RGB565
uint16_t rgb565_palette[256];

void I_InitGraphics(void) {
    printf("I_InitGraphics: LCD output: %dx%d, RGB565 (%d bpp)\n",
           DOOMGENERIC_RESX, DOOMGENERIC_RESY, DOOMGENERIC_LCD_BPP);

    printf("I_InitGraphics: DOOM screen size: w x h: %d x %d\n", SCREENWIDTH, SCREENHEIGHT);

    printf("I_InitGraphics: Nearest-neighbor scaling: %dx%d -> %dx%d\n",
           SCREENWIDTH, SCREENHEIGHT, DOOMGENERIC_RESX, DOOMGENERIC_RESY);

    I_VideoBuffer = (uint8_t *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);

    extern void I_InitInput(void);
    I_InitInput();
}

void I_ShutdownGraphics(void) { Z_Free(I_VideoBuffer); }

void I_StartFrame(void) {}

void I_StartTic(void) { I_GetEvent(); }

void I_UpdateNoBlit(void) {}

//
// I_FinishUpdate
//
void I_FinishUpdate(void) {
    for (int y = 0; y < DOOMGENERIC_RESY; y++) {
        const int source_y = y * SCREENHEIGHT / DOOMGENERIC_RESY;
        const uint8_t *source_row = I_VideoBuffer + source_y * SCREENWIDTH;
        pixel_t *destination_row = DG_ScreenBuffer + y * DOOMGENERIC_RESX;

        for (int x = 0; x < DOOMGENERIC_RESX; x++) {
            const int source_x = x * SCREENWIDTH / DOOMGENERIC_RESX;
            destination_row[x] = source_row[source_x];
        }
    }

    DG_DrawFrame();
}

//
// I_ReadScreen
//
void I_ReadScreen(uint8_t *scr) { memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT); }

//
// I_SetPalette
//
#define GFX_RGB565(r, g, b)                                                                        \
    ((((r & 0xF8) >> 3) << 11) | (((g & 0xFC) >> 2) << 5) | ((b & 0xF8) >> 3))
#define GFX_RGB565_R(color) ((0xF800 & color) >> 11)
#define GFX_RGB565_G(color) ((0x07E0 & color) >> 5)
#define GFX_RGB565_B(color) (0x001F & color)

void I_SetPalette(const uint8_t *palette) {
    for (int i = 0; i < 256; i++) {
        // Read raw RGB values for palette entry i
        uint8_t r = palette[i * 3 + 0];
        uint8_t g = palette[i * 3 + 1];
        uint8_t b = palette[i * 3 + 2];

        // Apply gamma correction lookup table
        uint8_t r_gamma = gammatable[usegamma][r];
        uint8_t g_gamma = gammatable[usegamma][g];
        uint8_t b_gamma = gammatable[usegamma][b];

        // Convert to 16-bit RGB565 and store in the lookup table
        rgb565_palette[i] = GFX_RGB565(r_gamma, g_gamma, b_gamma);
    }

    palette_changed = true;
}

// Given an RGB value, find the closest matching palette index.
int I_GetPaletteIndex(int r, int g, int b) {
    col_t color;

    printf("I_GetPaletteIndex\n");

    int best = 0;
    int best_diff = INT_MAX;

    for (int i = 0; i < 256; ++i) {
        color.r = GFX_RGB565_R(rgb565_palette[i]);
        color.g = GFX_RGB565_G(rgb565_palette[i]);
        color.b = GFX_RGB565_B(rgb565_palette[i]);

        int diff = (r - color.r) * (r - color.r) + (g - color.g) * (g - color.g) +
                   (b - color.b) * (b - color.b);

        if (diff < best_diff) {
            best = i;
            best_diff = diff;
        }

        if (diff == 0) {
            break;
        }
    }

    return best;
}

void I_BeginRead(void) {}

void I_EndRead(void) {}

void I_EnableLoadingDisk(void) {}

void I_BindVideoVariables(void) {}

void I_DisplayFPSDots(bool dots_on) {}

void I_CheckIsScreensaver(void) {}
