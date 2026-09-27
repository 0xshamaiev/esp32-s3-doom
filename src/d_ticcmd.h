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
//	System specific interface stuff.
//

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include <stdint.h>
#include "doomtype.h"

// Local movement and button input sampled once per game tick.

typedef struct {
    int8_t forwardmove; // *2048 for move
    int8_t sidemove;    // *2048 for move
    int16_t angleturn;         // <<16 for angle delta
    uint8_t buttons;
} ticcmd_t;

#endif
