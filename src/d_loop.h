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
#ifndef D_LOOP_H
#define D_LOOP_H

#include "d_ticcmd.h"
#include "doomtype.h"

void D_InitSinglePlayer(void);

void D_UpdateTics(void);

void TryRunTics(void);

void D_StartGameLoop(void);

extern bool singletics;
extern int gametic;

#endif
