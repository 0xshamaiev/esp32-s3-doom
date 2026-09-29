// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

#include "doomgeneric.h"
#include "i_system.h"
#include "m_argv.h"

pixel_t *DG_ScreenBuffer = NULL;

void D_DoomMain(void);

void doomgeneric_Create(int argc, char **argv) {
    myargc = argc;
    myargv = argv;

    DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(*DG_ScreenBuffer));
    if (DG_ScreenBuffer == NULL)
        I_Error("Unable to allocate framebuffer");

    DG_Init();
    D_DoomMain();
}
