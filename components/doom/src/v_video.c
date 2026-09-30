// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "i_system.h"

#include "doomtype.h"

#include "i_swap.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "config.h"
#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#define RANGECHECK

// The screen buffer that the v_video.c code draws to.

static uint8_t *dest_screen = NULL;

fixed_t dirtybox[4];

//
// V_MarkRect
//
void V_MarkRect(int x, int y, int width, int height) {
    // If we are temporarily using an alternate screen, do not
    // affect the update box.

    if (dest_screen == I_VideoBuffer) {
        M_AddToBox(dirtybox, x, y);
        M_AddToBox(dirtybox, x + width - 1, y + height - 1);
    }
}

//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, uint8_t *source, int width, int height, int destx, int desty) {
#ifdef RANGECHECK
    if (srcx < 0 || srcx + width > SCREENWIDTH || srcy < 0 || srcy + height > SCREENHEIGHT ||
        destx < 0 || destx + width > SCREENWIDTH || desty < 0 || desty + height > SCREENHEIGHT) {
        I_Error("Bad V_CopyRect");
    }
#endif

    V_MarkRect(destx, desty, width, height);

    uint8_t *src = source + SCREENWIDTH * srcy + srcx;
    uint8_t *dest = dest_screen + SCREENWIDTH * desty + destx;

    for (; height > 0; height--) {
        memcpy(dest, src, width);
        src += SCREENWIDTH;
        dest += SCREENWIDTH;
    }
}

//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//

void V_DrawPatch(int x, int y, patch_t *patch) {
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
        y + SHORT(patch->height) > SCREENHEIGHT) {
        I_Error(
            "Bad V_DrawPatch x=%i y=%i patch.width=%i patch.height=%i topoffset=%i leftoffset=%i",
            x, y, patch->width, patch->height, patch->topoffset, patch->leftoffset);
    }
#endif

    V_MarkRect(x, y, SHORT(patch->width), SHORT(patch->height));

    int col = 0;
    uint8_t *desttop = dest_screen + y * SCREENWIDTH + x;

    int w = SHORT(patch->width);

    for (; col < w; x++, col++, desttop++) {
        column_t *column = (column_t *)((uint8_t *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff) {
            uint8_t *source = (uint8_t *)column + 3;
            uint8_t *dest = desttop + column->topdelta * SCREENWIDTH;
            int count = column->length;

            while (count--) {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t *)((uint8_t *)column + column->length + 4);
        }
    }
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void V_DrawPatchFlipped(int x, int y, patch_t *patch) {
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0 || x + SHORT(patch->width) > SCREENWIDTH || y < 0 ||
        y + SHORT(patch->height) > SCREENHEIGHT) {
        I_Error("Bad V_DrawPatchFlipped");
    }
#endif

    V_MarkRect(x, y, SHORT(patch->width), SHORT(patch->height));

    int col = 0;
    uint8_t *desttop = dest_screen + y * SCREENWIDTH + x;

    int w = SHORT(patch->width);

    for (; col < w; x++, col++, desttop++) {
        column_t *column = (column_t *)((uint8_t *)patch + LONG(patch->columnofs[w - 1 - col]));

        // step through the posts in a column
        while (column->topdelta != 0xff) {
            uint8_t *source = (uint8_t *)column + 3;
            uint8_t *dest = desttop + column->topdelta * SCREENWIDTH;
            int count = column->length;

            while (count--) {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t *)((uint8_t *)column + column->length + 4);
        }
    }
}

//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//

void V_DrawPatchDirect(int x, int y, patch_t *patch) { V_DrawPatch(x, y, patch); }

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//

void V_DrawBlock(int x, int y, int width, int height, uint8_t *src) {
#ifdef RANGECHECK
    if (x < 0 || x + width > SCREENWIDTH || y < 0 || y + height > SCREENHEIGHT) {
        I_Error("Bad V_DrawBlock");
    }
#endif

    V_MarkRect(x, y, width, height);

    uint8_t *dest = dest_screen + y * SCREENWIDTH + x;

    while (height--) {
        memcpy(dest, src, width);
        src += width;
        dest += SCREENWIDTH;
    }
}

void V_DrawFilledBox(int x, int y, int w, int h, int c) {
    uint8_t *buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (int y1 = 0; y1 < h; ++y1) {
        uint8_t *buf1 = buf;

        for (int x1 = 0; x1 < w; ++x1) {
            *buf1++ = c;
        }

        buf += SCREENWIDTH;
    }
}

void V_DrawHorizLine(int x, int y, int w, int c) {
    uint8_t *buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (int x1 = 0; x1 < w; ++x1) {
        *buf++ = c;
    }
}

void V_DrawVertLine(int x, int y, int h, int c) {
    uint8_t *buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (int y1 = 0; y1 < h; ++y1) {
        *buf = c;
        buf += SCREENWIDTH;
    }
}

void V_DrawBox(int x, int y, int w, int h, int c) {
    V_DrawHorizLine(x, y, w, c);
    V_DrawHorizLine(x, y + h - 1, w, c);
    V_DrawVertLine(x, y, h, c);
    V_DrawVertLine(x + w - 1, y, h, c);
}

//
// Draw a "raw" screen (lump containing raw data to blit directly
// to the screen)
//

void V_DrawRawScreen(uint8_t *raw) { memcpy(dest_screen, raw, SCREENWIDTH * SCREENHEIGHT); }

//
// V_Init
//
void V_Init(void) {
    // no-op!
    // There used to be separate screens that could be drawn to; these are
    // now handled in the upper layers.
}

// Set the buffer that the code draws to.

void V_UseBuffer(uint8_t *buffer) { dest_screen = buffer; }

// Restore screen buffer to the i_video screen buffer.

void V_RestoreBuffer(void) { dest_screen = I_VideoBuffer; }

//
// SCREEN SHOTS
//

typedef struct {
    char manufacturer;
    char version;
    char encoding;
    char bits_per_pixel;

    uint16_t xmin;
    uint16_t ymin;
    uint16_t xmax;
    uint16_t ymax;

    uint16_t hres;
    uint16_t vres;

    uint8_t palette[48];

    char reserved;
    char color_planes;
    uint16_t bytes_per_line;
    uint16_t palette_type;

    char filler[58];
    uint8_t data; // unbounded
}

PACKEDATTR pcx_t;

//
// WritePCXfile
//

void WritePCXfile(char *filename, uint8_t *data, int width, int height, uint8_t *palette) {
    pcx_t *pcx = Z_Malloc(width * height * 2 + 1000, PU_STATIC, NULL);

    pcx->manufacturer = 0x0a; // PCX id
    pcx->version = 5;         // 256 color
    pcx->encoding = 1;        // uncompressed
    pcx->bits_per_pixel = 8;  // 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width - 1);
    pcx->ymax = SHORT(height - 1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset(pcx->palette, 0, sizeof(pcx->palette));
    pcx->color_planes = 1; // chunky image
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2); // not a grey scale
    memset(pcx->filler, 0, sizeof(pcx->filler));

    // pack the image
    uint8_t *pack = &pcx->data;

    for (int i = 0; i < width * height; i++) {
        if ((*data & 0xc0) != 0xc0)
            *pack++ = *data++;
        else {
            *pack++ = 0xc1;
            *pack++ = *data++;
        }
    }

    // write the palette
    *pack++ = 0x0c; // palette ID byte
    for (int i = 0; i < 768; i++)
        *pack++ = *palette++;

    // write output file
    int length = pack - (uint8_t *)pcx;
    M_WriteFile(filename, pcx, length);

    Z_Free(pcx);
}

#ifdef HAVE_LIBPNG
//
// WritePNGfile
//

static void error_fn(png_structp p, png_const_charp s) { printf("libpng error: %s\n", s); }

static void warning_fn(png_structp p, png_const_charp s) { printf("libpng warning: %s\n", s); }

void WritePNGfile(char *filename, uint8_t *data, int width, int height, uint8_t *palette) {
    png_structp ppng;
    png_infop pinfo;
    png_colorp pcolor;
    FILE *handle;
    int i;

    handle = fopen(filename, "wb");
    if (!handle) {
        return;
    }

    ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, error_fn, warning_fn);
    if (!ppng) {
        return;
    }

    pinfo = png_create_info_struct(ppng);
    if (!pinfo) {
        png_destroy_write_struct(&ppng, NULL);
        return;
    }

    png_init_io(ppng, handle);

    png_set_IHDR(ppng, pinfo, width, height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    pcolor = malloc(sizeof(*pcolor) * 256);
    if (!pcolor) {
        png_destroy_write_struct(&ppng, &pinfo);
        return;
    }

    for (i = 0; i < 256; i++) {
        pcolor[i].red = *(palette + 3 * i);
        pcolor[i].green = *(palette + 3 * i + 1);
        pcolor[i].blue = *(palette + 3 * i + 2);
    }

    png_set_PLTE(ppng, pinfo, pcolor, 256);
    free(pcolor);

    png_write_info(ppng, pinfo);

    for (i = 0; i < SCREENHEIGHT; i++) {
        png_write_row(ppng, data + i * SCREENWIDTH);
    }

    png_write_end(ppng, pinfo);
    png_destroy_write_struct(&ppng, &pinfo);
    fclose(handle);
}
#endif

//
// V_ScreenShot
//

void V_ScreenShot(char *format) {
    int i;
    char lbmname[16]; // haleyjd 20110213: BUG FIX - 12 is too small!
    char *ext;

    // find a file name to save it to

#ifdef HAVE_LIBPNG
    extern int png_screenshots;
    if (png_screenshots) {
        ext = "png";
    } else
#endif
    {
        ext = "pcx";
    }

    for (i = 0; i <= 99; i++) {
        M_snprintf(lbmname, sizeof(lbmname), format, i, ext);

        if (!M_FileExists(lbmname)) {
            break; // file doesn't exist
        }
    }

    if (i == 100) {
        I_Error("V_ScreenShot: Couldn't create a PCX");
    }

#ifdef HAVE_LIBPNG
    if (png_screenshots) {
        WritePNGfile(lbmname, I_VideoBuffer, SCREENWIDTH, SCREENHEIGHT,
                     W_CacheLumpName("PLAYPAL", PU_CACHE));
    } else
#endif
    {
        // save the pcx file
        WritePCXfile(lbmname, I_VideoBuffer, SCREENWIDTH, SCREENHEIGHT,
                     W_CacheLumpName("PLAYPAL", PU_CACHE));
    }
}
