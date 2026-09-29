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


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>

#include <unistd.h>


#include "config.h"

#include "deh_str.h"
#include "doomtype.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_timer.h"
#include "i_video.h"

#include "i_system.h"

#include "w_wad.h"
#include "z_zone.h"


#define DEFAULT_RAM 8 /* MiB */
#define MIN_RAM     6  /* MiB */


typedef struct atexit_listentry_s atexit_listentry_t;

struct atexit_listentry_s {
    atexit_func_t func;
    bool run_on_error;
    atexit_listentry_t *next;
};

static atexit_listentry_t *exit_funcs = NULL;

void I_AtExit(atexit_func_t func, bool run_on_error) {
    atexit_listentry_t *entry = malloc(sizeof(*entry));

    entry->func = func;
    entry->run_on_error = run_on_error;
    entry->next = exit_funcs;
    exit_funcs = entry;
}

// Tactile feedback function, probably used for the Logitech Cyberman

void I_Tactile(int on, int off, int total) {
}

// Zone memory auto-allocation function that allocates the zone size
// by trying progressively smaller zone sizes until one is found that
// works.

static uint8_t *AutoAllocMemory(int *size, int default_ram, int min_ram) {
    // Allocate the zone memory.  This loop tries progressively smaller
    // zone sizes until a size is found that can be allocated.
    // If we used the -mb command line parameter, only the parameter
    // provided is accepted.

    uint8_t *zonemem = NULL;

    while (zonemem == NULL) {
        // We need a reasonable minimum amount of RAM to start.

        if (default_ram < min_ram) {
            I_Error("Unable to allocate %i MiB of RAM for zone", default_ram);
        }

        // Try to allocate the zone memory.

        *size = default_ram * 1024 * 1024;

        zonemem = malloc(*size);

        // Failed to allocate?  Reduce zone size until we reach a size
        // that is acceptable.

        if (zonemem == NULL) {
            default_ram -= 1;
        }
    }

    return zonemem;
}

uint8_t *I_ZoneBase(int *size) {
    int min_ram;
    int default_ram;

    //!
    // @arg <mb>
    //
    // Specify the heap size, in MiB (default 16).
    //

    int p = M_CheckParmWithArgs("-mb", 1);

    if (p > 0) {
        default_ram = atoi(myargv[p + 1]);
        min_ram = default_ram;
    } else {
        default_ram = DEFAULT_RAM;
        min_ram = MIN_RAM;
    }

    uint8_t *zonemem = AutoAllocMemory(size, default_ram, min_ram);

    printf("zone memory: %p, %x allocated for zone\n",
           zonemem, *size);

    return zonemem;
}

void I_PrintBanner(char *msg) {
    int spaces = 35 - (strlen(msg) / 2);

    for (int i = 0; i < spaces; ++i)
        putchar(' ');

    puts(msg);
}

void I_PrintDivider(void) {
    for (int i = 0; i < 75; ++i) {
        putchar('=');
    }

    putchar('\n');
}

void I_PrintStartupBanner(char *gamedescription) {
    I_PrintDivider();
    I_PrintBanner(gamedescription);
    I_PrintDivider();

    printf(
        " " PACKAGE_NAME " is free software, covered by the GNU General Public\n"
        " License.  There is NO warranty; not even for MERCHANTABILITY or FITNESS\n"
        " FOR A PARTICULAR PURPOSE. You are welcome to change and distribute\n"
        " copies under certain conditions. See the source for more information.\n");

    I_PrintDivider();
}

//
// I_ConsoleStdout
//
// Returns true if stdout is a real console, false if it is a file
//

bool I_ConsoleStdout(void) {
    return 0;
}

//
// I_Init
//
/*
void I_Init (void)
{
    I_CheckIsScreensaver();
    I_InitTimer();

}
void I_BindVariables(void)
{
    I_BindVideoVariables();

    I_BindSoundVariables();
}
*/

//
// I_Quit
//

void I_Quit(void) {
    // Run through all exit functions

    atexit_listentry_t *entry = exit_funcs;

    while (entry != NULL) {
        entry->func();
        entry = entry->next;
    }

    exit(0);
}

//
// I_Error
//

static bool already_quitting = false;

void I_Error(char *error, ...) {
    va_list argptr;

    if (already_quitting) {
        fprintf(stderr, "Warning: recursive call to I_Error detected.\n");
        exit(-1);
    }

    already_quitting = true;

    // Message first.
    va_start(argptr, error);
    //fprintf(stderr, "\nError: ");
    vfprintf(stderr, error, argptr);
    fprintf(stderr, "\n\n");
    va_end(argptr);
    fflush(stderr);

    // Shutdown. Here might be other errors.

    atexit_listentry_t *entry = exit_funcs;

    while (entry != NULL) {
        if (entry->run_on_error) {
            entry->func();
        }

        entry = entry->next;
    }

    // abort();
    exit(-1);
}

//
// Read Access Violation emulation.
//
// From PrBoom+, by entryway.
//

// C:\>debug
// -d 0:0
//
// DOS 6.22:
// 0000:0000  (57 92 19 00) F4 06 70 00-(16 00)
// DOS 7.1:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// Win98:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// DOSBox under XP:
// 0000:0000  (00 00 00 F1) ?? ?? ?? 00-(07 00)

#define DOS_MEM_DUMP_SIZE 10

static const uint8_t mem_dump_dos622[DOS_MEM_DUMP_SIZE] = {
    0x57, 0x92, 0x19, 0x00, 0xF4, 0x06, 0x70, 0x00, 0x16, 0x00
};
static const uint8_t mem_dump_win98[DOS_MEM_DUMP_SIZE] = {
    0x9E, 0x0F, 0xC9, 0x00, 0x65, 0x04, 0x70, 0x00, 0x16, 0x00
};
static const uint8_t mem_dump_dosbox[DOS_MEM_DUMP_SIZE] = {
    0x00, 0x00, 0x00, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00
};
static uint8_t mem_dump_custom[DOS_MEM_DUMP_SIZE];

static const uint8_t *dos_mem_dump = mem_dump_dos622;

bool I_GetMemoryValue(uint32_t offset, void *value, int size) {
    static bool firsttime = true;

    if (firsttime) {
        int val;

        firsttime = false;
        int i = 0;

        //!
        // @category compat
        // @arg <version>
        //
        // Specify DOS version to emulate for NULL pointer dereference
        // emulation.  Supported versions are: dos622, dos71, dosbox.
        // The default is to emulate DOS 7.1 (Windows 98).
        //

        int p = M_CheckParmWithArgs("-setmem", 1);

        if (p > 0) {
            if (!strcasecmp(myargv[p + 1], "dos622")) {
                dos_mem_dump = mem_dump_dos622;
            }
            if (!strcasecmp(myargv[p + 1], "dos71")) {
                dos_mem_dump = mem_dump_win98;
            } else if (!strcasecmp(myargv[p + 1], "dosbox")) {
                dos_mem_dump = mem_dump_dosbox;
            } else {
                for (i = 0; i < DOS_MEM_DUMP_SIZE; ++i) {
                    ++p;

                    if (p >= myargc || myargv[p][0] == '-') {
                        break;
                    }

                    M_StrToInt(myargv[p], &val);
                    mem_dump_custom[i++] = (uint8_t) val;
                }

                dos_mem_dump = mem_dump_custom;
            }
        }
    }

    switch (size) {
        case 1:
            *(uint8_t *)value = dos_mem_dump[offset];
            return true;
        case 2:
            *(uint16_t *)value = dos_mem_dump[offset]
                                          | (dos_mem_dump[offset + 1] << 8);
            return true;
        case 4:
            *(uint32_t *)value = dos_mem_dump[offset]
                                        | (dos_mem_dump[offset + 1] << 8)
                                        | (dos_mem_dump[offset + 2] << 16)
                                        | (dos_mem_dump[offset + 3] << 24);
            return true;
    }

    return false;
}

