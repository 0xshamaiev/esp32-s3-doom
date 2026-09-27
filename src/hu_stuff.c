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
// DESCRIPTION:  Heads-up displays
//

#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"

#include "z_zone.h"

#include "deh_main.h"
#include "i_swap.h"
#include "i_video.h"

#include "hu_lib.h"
#include "hu_stuff.h"
#include "m_controls.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE (mapnames[(gameepisode - 1) * 9 + gamemap - 1])
#define HU_TITLE2 (mapnames_commercial[gamemap - 1])
#define HU_TITLEHEIGHT 1
#define HU_TITLEX 0
#define HU_TITLEY (167 - SHORT(hu_font[0]->height))

char *player_names[] = {HUSTR_PLRGREEN, HUSTR_PLRINDIGO, HUSTR_PLRBROWN, HUSTR_PLRRED};

static player_t *plr;
patch_t *hu_font[HU_FONTSIZE];
static hu_textline_t w_title;

static bool message_on;
bool message_dontfuckwithme;
static bool message_nottobefuckedwith;

static hu_stext_t w_message;
static int message_counter;

extern int showMessages;

static bool headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char *mapnames[] = // DOOM shareware/registered/retail (Ultimate) names.
    {HUSTR_E1M1, HUSTR_E1M2, HUSTR_E1M3, HUSTR_E1M4, HUSTR_E1M5,
     HUSTR_E1M6, HUSTR_E1M7, HUSTR_E1M8, HUSTR_E1M9,

     HUSTR_E2M1, HUSTR_E2M2, HUSTR_E2M3, HUSTR_E2M4, HUSTR_E2M5,
     HUSTR_E2M6, HUSTR_E2M7, HUSTR_E2M8, HUSTR_E2M9,

     HUSTR_E3M1, HUSTR_E3M2, HUSTR_E3M3, HUSTR_E3M4, HUSTR_E3M5,
     HUSTR_E3M6, HUSTR_E3M7, HUSTR_E3M8, HUSTR_E3M9,

     HUSTR_E4M1, HUSTR_E4M2, HUSTR_E4M3, HUSTR_E4M4, HUSTR_E4M5,
     HUSTR_E4M6, HUSTR_E4M7, HUSTR_E4M8, HUSTR_E4M9};

// Doom II map names.
char *mapnames_commercial[] = {
    // DOOM 2 map names.

    HUSTR_1,  HUSTR_2,  HUSTR_3,  HUSTR_4,  HUSTR_5,  HUSTR_6,  HUSTR_7,  HUSTR_8,  HUSTR_9,
    HUSTR_10, HUSTR_11,

    HUSTR_12, HUSTR_13, HUSTR_14, HUSTR_15, HUSTR_16, HUSTR_17, HUSTR_18, HUSTR_19, HUSTR_20,

    HUSTR_21, HUSTR_22, HUSTR_23, HUSTR_24, HUSTR_25, HUSTR_26, HUSTR_27, HUSTR_28, HUSTR_29,
    HUSTR_30, HUSTR_31, HUSTR_32,
};

void HU_Init(void) {
    char buffer[9];

    // load the heads-up font
    int j = HU_FONTSTART;
    for (int i = 0; i < HU_FONTSIZE; i++) {
        DEH_snprintf(buffer, 9, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *)W_CacheLumpName(buffer, PU_STATIC);
    }
}

void HU_Stop(void) { headsupactive = false; }

void HU_Start(void) {
    char *s;

    if (headsupactive)
        HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;

    // create the message widget
    HUlib_initSText(&w_message, HU_MSGX, HU_MSGY, HU_MSGHEIGHT, hu_font, HU_FONTSTART, &message_on);

    // create the map title widget
    HUlib_initTextLine(&w_title, HU_TITLEX, HU_TITLEY, hu_font, HU_FONTSTART);

    switch (gamemission) {
    case doom:
        s = HU_TITLE;
        break;
    case doom2:
        s = HU_TITLE2;
        break;
    default:
        s = "Unknown level";
        break;
    }

    // dehacked substitution to get modified level name

    s = DEH_String(s);

    while (*s)
        HUlib_addCharToTextLine(&w_title, *(s++));

    headsupactive = true;
}

void HU_Drawer(void) {
    HUlib_drawSText(&w_message);
    if (automapactive)
        HUlib_drawTextLine(&w_title, false);
}

void HU_Erase(void) {
    HUlib_eraseSText(&w_message);
    HUlib_eraseTextLine(&w_title);
}

void HU_Ticker(void) {
    // tick down message counter if message is up
    if (message_counter && !--message_counter) {
        message_on = false;
        message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme) {
        // display message if necessary
        if ((plr->message && !message_nottobefuckedwith) ||
            (plr->message && message_dontfuckwithme)) {
            HUlib_addMessageToSText(&w_message, 0, plr->message);
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }
    } // else message_on = false;
}

bool HU_Responder(event_t *ev) {
    if (ev->type == ev_keydown && ev->data1 == key_message_refresh) {
        message_on = true;
        message_counter = HU_MSGTIMEOUT;
        return true;
    }
    return false;
}