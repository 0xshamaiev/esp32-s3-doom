//
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
//     Search for and locate an IWAD file, and initialize according
//     to the IWAD type.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deh_str.h"
#include "d_iwad.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

static const iwad_t iwads[] =
{
    {"doom2.wad", doom2, commercial, "Doom II"},
    {"doom.wad", doom, retail, "Doom"},
    {"doom1.wad", doom, shareware, "Doom Shareware"},
};

// Check if the specified directory contains the specified IWAD
// file, returning the full path to the IWAD if found, or NULL
// if not found.

static char *CheckDirectoryHasIWAD(char *dir, char *iwadname) {
    char *filename;

    // Construct the full path to the IWAD if it is located in
    // this directory, and check if it exists.

    if (!strcmp(dir, ".")) {
        filename = strdup(iwadname);
    } else {
        filename = M_StringJoin(dir, DIR_SEPARATOR_S, iwadname, NULL);
    }

    printf("Trying IWAD file:%s\n", filename);

    if (M_FileExists(filename)) {
        return filename;
    }

    free(filename);

    return NULL;
}

// Search a directory to try to find an IWAD
// Returns the location of the IWAD if found, otherwise NULL.

static char *SearchDirectoryForIWAD(char *dir, int mask, GameMission_t *mission) {
    for (size_t i = 0; i < arrlen(iwads); ++i) {
        if (((1 << iwads[i].mission) & mask) == 0) {
            continue;
        }

        char *filename = CheckDirectoryHasIWAD(dir, DEH_String(iwads[i].name));

        if (filename != NULL) {
            *mission = iwads[i].mission;

            return filename;
        }
    }

    return NULL;
}

// When given an IWAD with the '-iwad' parameter,
// attempt to identify it by its name.

static GameMission_t IdentifyIWADByName(char *name, int mask) {
    char *p = strrchr(name, DIR_SEPARATOR);

    if (p != NULL) {
        name = p + 1;
    }

    GameMission_t mission = none;

    for (size_t i = 0; i < arrlen(iwads); ++i) {
        // Check if the filename is this IWAD name.

        // Only use supported missions:

        if (((1 << iwads[i].mission) & mask) == 0)
            continue;

        // Check if it ends in this IWAD name.

        if (!strcasecmp(name, iwads[i].name)) {
            mission = iwads[i].mission;
            break;
        }
    }

    return mission;
}


// Search an explicit path first, then the application's mounted storage.
char *D_FindWADByName(char *name) {
    if (M_FileExists(name))
        return name;

    char *path = M_StringJoin(FILES_DIR, DIR_SEPARATOR_S, name, NULL);
    if (M_FileExists(path))
        return path;

    free(path);
    return NULL;
}

//
// D_TryWADByName
//
// Searches for a WAD by its filename, or passes through the filename
// if not found.
//

char *D_TryFindWADByName(char *filename) {
    char *result = D_FindWADByName(filename);

    if (result != NULL) {
        return result;
    }
    return filename;
}

//
// FindIWAD
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWADs).
//

char *D_FindIWAD(int mask, GameMission_t *mission) {
    char *result;

    // Check for the -iwad parameter

    //!
    // Specify an IWAD file to use.
    //
    // @arg <file>
    //

    int iwadparm = M_CheckParmWithArgs("-iwad", 1);

    if (iwadparm) {
        // Find the requested IWAD on mounted storage.

        char *iwadfile = myargv[iwadparm + 1];

        result = D_FindWADByName(iwadfile);

        if (result == NULL) {
            I_Error("IWAD file '%s' not found!", iwadfile);
        }

        *mission = IdentifyIWADByName(result, mask);
        if (*mission == none)
            I_Error("Unsupported IWAD '%s'. Use doom1.wad, doom.wad or doom2.wad.", iwadfile);
    } else {
        // Search through the list and look for an IWAD

        printf("-iwad not specified, trying a few iwad names\n");

        result = SearchDirectoryForIWAD(FILES_DIR, mask, mission);
    }

    return result;
}

// Find all IWADs in the IWAD search path matching the given mask.

const iwad_t **D_FindAllIWADs(int mask) {
    const iwad_t **result = malloc(sizeof(iwad_t *) * (arrlen(iwads) + 1));
    int result_len = 0;

    // Try to find all IWADs

    for (int i = 0; i < arrlen(iwads); ++i) {
        if (((1 << iwads[i].mission) & mask) == 0) {
            continue;
        }

        const char *filename = D_FindWADByName(iwads[i].name);

        if (filename != NULL) {
            result[result_len] = &iwads[i];
            ++result_len;
        }
    }

    // End of list

    result[result_len] = NULL;

    return result;
}

//
// Get the IWAD name used for savegames.
//

char *D_SaveGameIWADName(GameMission_t gamemission) {
    // Determine the IWAD name to use for savegames.
    // This determines the directory the savegame files get put into.
    //
    // Note that we match on gamemission rather than on IWAD name.
    // This ensures that doom1.wad and doom.wad saves are stored
    // in the same place.

    for (size_t i = 0; i < arrlen(iwads); ++i) {
        if (gamemission == iwads[i].mission) {
            return iwads[i].name;
        }
    }

    // Default fallback:

    return "unknown.wad";
}

char *D_SuggestIWADName(GameMission_t mission, GameMode_t mode) {
    for (int i = 0; i < arrlen(iwads); ++i) {
        if (iwads[i].mission == mission && iwads[i].mode == mode) {
            return iwads[i].name;
        }
    }

    return "unknown.wad";
}

char *D_SuggestGameName(GameMission_t mission, GameMode_t mode) {
    for (int i = 0; i < arrlen(iwads); ++i) {
        if (iwads[i].mission == mission
            && (mode == indetermined || iwads[i].mode == mode)) {
            return iwads[i].description;
        }
    }

    return "Unknown game?";
}

