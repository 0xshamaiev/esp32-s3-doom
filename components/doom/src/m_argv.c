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
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h" // haleyjd 20110212: warning fix
#include "m_misc.h"

int myargc;
char **myargv;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

int M_CheckParmWithArgs(const char *check, const int num_args) {
    for (int i = 1; i < myargc - num_args; i++) {
        if (!strcasecmp(check, myargv[i]))
            return i;
    }

    return 0;
}

//
// M_ParmExists
//
// Returns true if the given parameter exists in the program's command
// line arguments, false if not.
//

bool M_ParmExists(char *check) { return M_CheckParm(check) != 0; }

int M_CheckParm(const char *check) { return M_CheckParmWithArgs(check, 0); }

// Return the name of the executable used to start the program:

char *M_GetExecutableName(void) {
    char *sep = strrchr(myargv[0], DIR_SEPARATOR);

    if (sep == NULL) {
        return myargv[0];
    }

    return sep + 1;
}