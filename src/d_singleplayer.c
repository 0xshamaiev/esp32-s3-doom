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
//     Single-player game initialization and ticker callbacks.
//

#include "d_loop.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_menu.h"

// Input for the only active player.
ticcmd_t *localcmds;

static void RunTic(ticcmd_t *cmds) {
    extern boolean advancedemo;

    localcmds = cmds;
    if (advancedemo)
        D_DoAdvanceDemo();
    G_Ticker();
}

static loop_interface_t doom_loop_interface = {.ProcessEvents = D_ProcessEvents,
                                               .BuildTiccmd = G_BuildTiccmd,
                                               .RunTic = RunTic,
                                               .RunMenu = M_Ticker};

void D_InitSinglePlayer(void) {
    consoleplayer = displayplayer = 0;
    for (unsigned int i = 0; i < MAXPLAYERS; ++i)
        playeringame[i] = i == 0;

    lowres_turn = M_CheckParm("-record") > 0 && M_CheckParm("-longtics") == 0;
    D_RegisterLoopCallbacks(&doom_loop_interface);
}