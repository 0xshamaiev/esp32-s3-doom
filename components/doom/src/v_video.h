// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

//
// Copyright(C) 1993-1996 Id Software, Inc.
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
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"
#include "m_fixed.h"
#include <stdint.h>

// Needed because we are refering to patches.
#include "v_patch.h"

//
// VIDEO
//

#define CENTERY (SCREENHEIGHT / 2)

extern fixed_t dirtybox[4];

// Allocates buffer screens, call before R_Init.
void V_Init(void);

// Draw a block from the specified source screen to the screen.

void V_CopyRect(int srcx, int srcy, uint8_t *source, int width, int height, int destx, int desty);

void V_DrawPatch(int x, int y, patch_t *patch);

void V_DrawPatchFlipped(int x, int y, patch_t *patch);

void V_DrawPatchDirect(int x, int y, patch_t *patch);

// Draw a linear block of pixels into the view buffer.

void V_DrawBlock(int x, int y, int width, int height, uint8_t *src);

void V_MarkRect(int x, int y, int width, int height);

void V_DrawFilledBox(int x, int y, int w, int h, int c);

void V_DrawHorizLine(int x, int y, int w, int c);

void V_DrawVertLine(int x, int y, int h, int c);

void V_DrawBox(int x, int y, int w, int h, int c);

// Draw a raw screen lump

void V_DrawRawScreen(uint8_t *raw);

// Temporarily switch to using a different buffer to draw graphics, etc.

void V_UseBuffer(uint8_t *buffer);

// Return to using the normal screen buffer to draw graphics.

void V_RestoreBuffer(void);

// Save a screenshot of the current screen to a file, named in the
// format described in the string passed to the function, eg.
// "DOOM%02i.pcx"

void V_ScreenShot(char *format);

#endif
