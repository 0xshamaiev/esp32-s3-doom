#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>
#include <stdlib.h>

#ifndef DOOMGENERIC_RESX
#define DOOMGENERIC_RESX 240
#endif // DOOMGENERIC_RESX

#ifndef DOOMGENERIC_RESY
#define DOOMGENERIC_RESY 135
#endif // DOOMGENERIC_RESY

#ifdef CMAP256

typedef uint8_t pixel_t;

#else // CMAP256

typedef uint32_t pixel_t;

#endif // CMAP256

extern pixel_t *DG_ScreenBuffer;

#ifdef __cplusplus
extern "C" {
#endif

void doomgeneric_Create(int argc, char **argv);

void doomgeneric_Tick(void);

// Implement below functions for your platform
void DG_Init(void);

void DG_DrawFrame(void);

void DG_SleepMs(uint32_t ms);

uint32_t DG_GetTicksMs(void);

int DG_GetKey(int *pressed, uint8_t *key);

#ifdef __cplusplus
}
#endif

#endif // DOOM_GENERIC
