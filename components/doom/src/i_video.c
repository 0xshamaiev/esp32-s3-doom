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

static const char rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "i_video.h"
#include "config.h"
#include "d_event.h"
#include "d_main.h"
#include "i_system.h"
#include "m_argv.h"
#include "v_video.h"
#include "z_zone.h"
#include <inttypes.h>
#include <stdint.h>

#include "doomkeys.h"
#include "tables.h"

#include "doomgeneric.h"

#include <stdlib.h>

#include <fcntl.h>

#include <stdarg.h>

#include <sys/types.h>

struct FB_BitField {
    uint32_t offset; /* beginning of bitfield	*/
    uint32_t length; /* length of bitfield		*/
};

struct FB_ScreenInfo {
    uint32_t xres; /* visible resolution		*/
    uint32_t yres;
    uint32_t xres_virtual; /* virtual resolution		*/
    uint32_t yres_virtual;

    uint32_t bits_per_pixel; /* guess what			*/

    /* >1 = FOURCC			*/
    struct FB_BitField red;   /* bitfield in s_Fb mem if true color, */
    struct FB_BitField green; /* else only length is significant */
    struct FB_BitField blue;
    struct FB_BitField transp; /* transparency			*/
};

static struct FB_ScreenInfo s_Fb;
int fb_scaling = 1;

bool palette_changed;
struct color colors[256];

void I_GetEvent(void);

// The screen buffer; this is modified to draw things to the screen

uint8_t *I_VideoBuffer = NULL;

// If true, game is running as a screensaver

bool screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isn't visible, don't render the screen

bool screenvisible;

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
    memset(&s_Fb, 0, sizeof(struct FB_ScreenInfo));
    s_Fb.xres = DOOMGENERIC_RESX;
    s_Fb.yres = DOOMGENERIC_RESY;
    s_Fb.xres_virtual = s_Fb.xres;
    s_Fb.yres_virtual = s_Fb.yres;
    // The Waveshare 1.14" display expects RGB565
    s_Fb.bits_per_pixel = 16;

    s_Fb.blue.length = 5;
    s_Fb.green.length = 6;
    s_Fb.red.length = 5;
    s_Fb.transp.length = 0;

    s_Fb.blue.offset = 11;
    s_Fb.green.offset = 5;
    s_Fb.red.offset = 0;
    s_Fb.transp.offset = 16;

    printf("I_InitGraphics: framebuffer: x_res: %" PRIu32 ", y_res: %" PRIu32
           ", x_virtual: %" PRIu32 ", y_virtual: %" PRIu32 ", bpp: "
           "%" PRIu32 "\n",
           s_Fb.xres, s_Fb.yres, s_Fb.xres_virtual, s_Fb.yres_virtual, s_Fb.bits_per_pixel);

    printf("I_InitGraphics: framebuffer: RGBA: %" PRIu32 "%" PRIu32 "%" PRIu32 "%" PRIu32
           ", red_off: %" PRIu32 ", green_off: %" PRIu32 ", blue_off: %" PRIu32 ", "
           "transp_off: %" PRIu32 "\n",
           s_Fb.red.length, s_Fb.green.length, s_Fb.blue.length, s_Fb.transp.length,
           s_Fb.red.offset, s_Fb.green.offset, s_Fb.blue.offset, s_Fb.transp.offset);

    printf("I_InitGraphics: DOOM screen size: w x h: %d x %d\n", SCREENWIDTH, SCREENHEIGHT);

    int i = M_CheckParmWithArgs("-scaling", 1);
    if (i > 0) {
        i = atoi(myargv[i + 1]);
        fb_scaling = i;
        printf("I_InitGraphics: Scaling factor: %d\n", fb_scaling);
    } else {
        fb_scaling = s_Fb.xres / SCREENWIDTH;
        if ((int)(s_Fb.yres / SCREENHEIGHT) < fb_scaling)
            fb_scaling = s_Fb.yres / SCREENHEIGHT;
        printf("I_InitGraphics: Auto-scaling factor: %d\n", fb_scaling);
    }

    /* Allocate screen to draw to */
    I_VideoBuffer = (uint8_t *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    // For DOOM to draw on

    screenvisible = true;

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
    uint8_t *line_in = I_VideoBuffer;
    uint8_t *line_out = DG_ScreenBuffer;

    int y = SCREENHEIGHT;

    while (y--) {
        for (int i = 0; i < fb_scaling; i++) {
            // Write 8-bit palette indices directly into DG_ScreenBuffer
            for (int x = 0; x < SCREENWIDTH; x++) {
                uint8_t pixel_idx = line_in[x];
                for (int s_x = 0; s_x < fb_scaling; s_x++) {
                    *line_out++ = pixel_idx; // 1 byte per pixel
                }
            }
        }
        line_in += SCREENWIDTH;
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