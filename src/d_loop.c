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
//     Local 35 Hz game loop.
//

#include <string.h>

#include "d_loop.h"
#include "doomdef.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#define BACKUPTICS 8

static ticcmd_t ticdata[BACKUPTICS];
static int maketic;
static int lasttime;
static loop_interface_t *loop_interface;

int gametic;
boolean singletics = false;

static boolean BuildNewTic(void) {
    I_StartTic();
    loop_interface->ProcessEvents();
    loop_interface->RunMenu();

    // Bound input buffering when rendering or loading takes a long time.
    if (maketic - gametic >= 5)
        return false;

    ticcmd_t *cmd = &ticdata[maketic % BACKUPTICS];
    memset(cmd, 0, sizeof(*cmd));
    loop_interface->BuildTiccmd(cmd, maketic);
    ++maketic;
    return true;
}

void D_UpdateTics(void) {
    if (singletics)
        return;

    int nowtime = I_GetTime();
    int newtics = nowtime - lasttime;
    lasttime = nowtime;

    while (newtics-- > 0)
        if (!BuildNewTic())
            break;
}

void D_StartGameLoop(void) { lasttime = I_GetTime(); }

void TryRunTics(void) {
    static int oldentertics;
    int entertic = I_GetTime();
    int realtics = entertic - oldentertics;
    int counts;

    oldentertics = entertic;
    if (singletics)
        BuildNewTic();
    else
        D_UpdateTics();

    int availabletics = maketic - gametic;
    if (realtics < availabletics - 1)
        counts = realtics + 1;
    else if (realtics < availabletics)
        counts = realtics;
    else
        counts = availabletics;
    if (counts < 1)
        counts = 1;

    while (maketic < gametic + counts) {
        D_UpdateTics();
        if (maketic < gametic)
            I_Error("TryRunTics: maketic < gametic");
        // Keep menus and the display responsive while waiting for a tic.
        if (I_GetTime() - entertic > 0)
            return;
        I_Sleep(1);
    }

    while (counts-- > 0) {
        loop_interface->RunTic(&ticdata[gametic % BACKUPTICS]);
        ++gametic;
        D_UpdateTics();
    }
}

void D_RegisterLoopCallbacks(loop_interface_t *callbacks) { loop_interface = callbacks; }
