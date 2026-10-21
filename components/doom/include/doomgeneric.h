// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>

#ifndef DOOMGENERIC_RESX
#define DOOMGENERIC_RESX 320
#endif

#ifndef DOOMGENERIC_RESY
#define DOOMGENERIC_RESY 200
#endif

// The display backend converts indexed pixels to RGB565 for the LCD.
#define DOOMGENERIC_LCD_BPP 16

typedef uint8_t pixel_t;

extern pixel_t *DG_ScreenBuffer;

void doomgeneric_Create(int argc, char **argv);

void doomgeneric_Tick(void);

void DG_Init(void);

void DG_DrawFrame(void);

void DG_SleepMs(uint32_t ms);

uint32_t DG_GetTicksMs(void);

int DG_GetKey(int *pressed, uint8_t *key);

#endif // DOOM_GENERIC
