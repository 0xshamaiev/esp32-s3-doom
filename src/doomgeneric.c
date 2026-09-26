#include "doomgeneric.h"
#include "i_system.h"
#include "m_argv.h"

pixel_t *DG_ScreenBuffer = NULL;

void D_DoomMain(void);

void doomgeneric_Create(int argc, char **argv) {
    myargc = argc;
    myargv = argv;
    M_FindResponseFile();

    DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(*DG_ScreenBuffer));
    if (DG_ScreenBuffer == NULL)
        I_Error("Unable to allocate framebuffer");

    DG_Init();
    D_DoomMain();
}
