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
// DESCRIPTION:  none
//


#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

#include "deh_main.h"
#include "deh_misc.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"


#include "g_game.h"


#define SAVEGAMESIZE	0x2c000

void G_ReadDemoTiccmd(ticcmd_t * cmd);
void G_WriteDemoTiccmd(ticcmd_t * cmd);

void G_PlayerReborn(int player);

void G_DoReborn(int playernum);

void G_DoLoadLevel(void);

void G_DoNewGame(void);

void G_DoPlayDemo(void);

void G_DoCompleted(void);

void G_DoVictory(void);

void G_DoWorldDone(void);

void G_DoSaveGame(void);

// Gamestate the last time G_Ticker was called.

gamestate_t oldgamestate;

gameaction_t gameaction;
gamestate_t gamestate;
skill_t gameskill;
boolean respawnmonsters;
int gameepisode;
int gamemap;

// If non-zero, exit the level after this number of minutes.


boolean paused;
boolean sendpause; // send a pause event next tic
boolean sendsave; // send a save event next tic
boolean usergame; // ok to save / end game

boolean timingdemo; // if true, exit with report on completion
boolean nodrawers; // for comparative timing purposes
int starttime; // for comparative timing purposes

boolean viewactive;

boolean playeringame[MAXPLAYERS];
player_t players[MAXPLAYERS];

boolean turbodetected[MAXPLAYERS];

int consoleplayer; // player taking events and displaying
int displayplayer; // view being displayed
int levelstarttic; // gametic at level start
int totalkills, totalitems, totalsecret; // for intermission

char *demoname;
boolean demorecording;
boolean longtics; // cph's doom 1.91 longtics hack
boolean lowres_turn; // low resolution turning for longtics
boolean demoplayback;
byte *demobuffer;
byte *demo_p;
byte *demoend;
boolean singledemo; // quit after playing a demo from cmdline

boolean precache = true; // if true, load all graphics at start

boolean testcontrols = false; // Invoked by setup to test controls
int testcontrols_mousespeed;


wbstartstruct_t wminfo; // parms for world map / intermission


#define MAXPLMOVE		(forwardmove[1])

#define TURBOTHRESHOLD	0x32

fixed_t forwardmove[2] = {0x19, 0x32};
fixed_t sidemove[2] = {0x18, 0x28};
fixed_t angleturn[3] = {640, 1280, 320}; // + slow turn

static int *weapon_keys[] = {
    &key_weapon1,
    &key_weapon2,
    &key_weapon3,
    &key_weapon4,
    &key_weapon5,
    &key_weapon6,
    &key_weapon7,
    &key_weapon8
};

// Set to -1 or +1 to switch to the previous or next weapon.

static int next_weapon = 0;

// Used for prev/next weapon keys.

static const struct {
    weapontype_t weapon;
    weapontype_t weapon_num;
} weapon_order_table[] = {
    {wp_fist, wp_fist},
    {wp_chainsaw, wp_fist},
    {wp_pistol, wp_pistol},
    {wp_shotgun, wp_shotgun},
    {wp_supershotgun, wp_shotgun},
    {wp_chaingun, wp_chaingun},
    {wp_missile, wp_missile},
    {wp_plasma, wp_plasma},
    {wp_bfg, wp_bfg}
};

#define SLOWTURNTICS	6

#define NUMKEYS		256
#define MAX_JOY_BUTTONS 20

static boolean gamekeydown[NUMKEYS];
static int turnheld; // for accelerative turning

static boolean mousearray[MAX_MOUSE_BUTTONS + 1];
static boolean *mousebuttons = &mousearray[1]; // allow [-1]

// mouse values are used once
int mousex;
int mousey;

static int dclicktime;
static boolean dclickstate;
static int dclicks;
static int dclicktime2;
static boolean dclickstate2;
static int dclicks2;

// joystick values are repeated
static int joyxmove;
static int joyymove;
static int joystrafemove;
static boolean joyarray[MAX_JOY_BUTTONS + 1];
static boolean *joybuttons = &joyarray[1]; // allow [-1]

static int savegameslot;
static char savedescription[32];


int vanilla_savegame_limit = 1;
int vanilla_demo_limit = 1;

int G_CmdChecksum(ticcmd_t *cmd) {
    int sum = 0;

    for (size_t i = 0; i < sizeof(*cmd) / 4 - 1; i++)
        sum += ((int *) cmd)[i];

    return sum;
}

static boolean WeaponSelectable(weapontype_t weapon) {
    // Can't select the super shotgun in Doom 1.

    if (weapon == wp_supershotgun && gamemission == doom) {
        return false;
    }

    // These weapons aren't available in shareware.

    if ((weapon == wp_plasma || weapon == wp_bfg)
        && gamemission == doom && gamemode == shareware) {
        return false;
    }

    // Can't select a weapon if we don't own it.

    if (!players[consoleplayer].weaponowned[weapon]) {
        return false;
    }

    // Can't select the fist if we have the chainsaw, unless
    // we also have the berserk pack.

    if (weapon == wp_fist
        && players[consoleplayer].weaponowned[wp_chainsaw]
        && !players[consoleplayer].powers[pw_strength]) {
        return false;
    }

    return true;
}

static int G_NextWeapon(int direction) {
    weapontype_t weapon;
    int i;

    // Find index in the table.

    if (players[consoleplayer].pendingweapon == wp_nochange) {
        weapon = players[consoleplayer].readyweapon;
    } else {
        weapon = players[consoleplayer].pendingweapon;
    }

    for (i = 0; i < arrlen(weapon_order_table); ++i) {
        if (weapon_order_table[i].weapon == weapon) {
            break;
        }
    }

    // Switch weapon. Don't loop forever.
    int start_i = i;
    do {
        i += direction;
        i = (i + arrlen(weapon_order_table)) % arrlen(weapon_order_table);
    } while (i != start_i && !WeaponSelectable(weapon_order_table[i].weapon));

    return weapon_order_table[i].weapon_num;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd(ticcmd_t *cmd, int maketic) {
    int i;
    int tspeed;
    int side;

    memset(cmd, 0, sizeof(ticcmd_t));

    boolean strafe =
        gamekeydown[key_strafe] || mousebuttons[mousebstrafe] || joybuttons[joybstrafe];

    // fraggle: support the old "joyb_speed = 31" hack which
    // allowed an autorun effect

    int speed = key_speed >= NUMKEYS || joybspeed >= MAX_JOY_BUTTONS || gamekeydown[key_speed] ||
                joybuttons[joybspeed];

    int forward = side = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
    if (joyxmove < 0
        || joyxmove > 0
        || gamekeydown[key_right]
        || gamekeydown[key_left])
        turnheld += 1;
    else
        turnheld = 0;

    if (turnheld < SLOWTURNTICS)
        tspeed = 2; // slow turn
    else
        tspeed = speed;

    // let movement keys cancel each other out
    if (strafe) {
        if (gamekeydown[key_right]) {
            // fprintf(stderr, "strafe right\n");
            side += sidemove[speed];
        }
        if (gamekeydown[key_left]) {
            //	fprintf(stderr, "strafe left\n");
            side -= sidemove[speed];
        }
        if (joyxmove > 0)
            side += sidemove[speed];
        if (joyxmove < 0)
            side -= sidemove[speed];
    } else {
        if (gamekeydown[key_right])
            cmd->angleturn -= angleturn[tspeed];
        if (gamekeydown[key_left])
            cmd->angleturn += angleturn[tspeed];
        if (joyxmove > 0)
            cmd->angleturn -= angleturn[tspeed];
        if (joyxmove < 0)
            cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_up]) {
        // fprintf(stderr, "up\n");
        forward += forwardmove[speed];
    }
    if (gamekeydown[key_down]) {
        // fprintf(stderr, "down\n");
        forward -= forwardmove[speed];
    }

    if (joyymove < 0)
        forward += forwardmove[speed];
    if (joyymove > 0)
        forward -= forwardmove[speed];

    if (gamekeydown[key_strafeleft]
        || joybuttons[joybstrafeleft]
        || mousebuttons[mousebstrafeleft]
        || joystrafemove < 0) {
        side -= sidemove[speed];
    }

    if (gamekeydown[key_straferight]
        || joybuttons[joybstraferight]
        || mousebuttons[mousebstraferight]
        || joystrafemove > 0) {
        side += sidemove[speed];
    }

    // buttons

    if (gamekeydown[key_fire] || mousebuttons[mousebfire]
        || joybuttons[joybfire])
        cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use]
        || joybuttons[joybuse]
        || mousebuttons[mousebuse]) {
        cmd->buttons |= BT_USE;
        // clear double clicks if hit use button
        dclicks = 0;
    }

    // If the previous or next weapon button is pressed, the
    // next_weapon variable is set to change weapons when
    // we generate a ticcmd.  Choose a new weapon.

    if (gamestate == GS_LEVEL && next_weapon != 0) {
        i = G_NextWeapon(next_weapon);
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= i << BT_WEAPONSHIFT;
    } else {
        // Check weapon keys.

        for (i = 0; i < arrlen(weapon_keys); ++i) {
            int key = *weapon_keys[i];

            if (gamekeydown[key]) {
                cmd->buttons |= BT_CHANGE;
                cmd->buttons |= i << BT_WEAPONSHIFT;
                break;
            }
        }
    }

    next_weapon = 0;

    // mouse
    if (mousebuttons[mousebforward]) {
        forward += forwardmove[speed];
    }
    if (mousebuttons[mousebbackward]) {
        forward -= forwardmove[speed];
    }

    if (dclick_use) {
        // forward double click
        if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1) {
            dclickstate = mousebuttons[mousebforward];
            if (dclickstate)
                dclicks++;
            if (dclicks == 2) {
                cmd->buttons |= BT_USE;
                dclicks = 0;
            } else
                dclicktime = 0;
        } else {
            dclicktime += 1;
            if (dclicktime > 20) {
                dclicks = 0;
                dclickstate = 0;
            }
        }

        // strafe double click
        boolean bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
        if (bstrafe != dclickstate2 && dclicktime2 > 1) {
            dclickstate2 = bstrafe;
            if (dclickstate2)
                dclicks2++;
            if (dclicks2 == 2) {
                cmd->buttons |= BT_USE;
                dclicks2 = 0;
            } else
                dclicktime2 = 0;
        } else {
            dclicktime2 += 1;
            if (dclicktime2 > 20) {
                dclicks2 = 0;
                dclickstate2 = 0;
            }
        }
    }

    forward += mousey;

    if (strafe)
        side += mousex * 2;
    else
        cmd->angleturn -= mousex * 0x8;

    if (mousex == 0) {
        // No movement in the previous frame

        testcontrols_mousespeed = 0;
    }

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    // special buttons
    if (sendpause) {
        sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave) {
        sendsave = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }

    // low-res turning

    if (lowres_turn) {
        static signed short carry = 0;

        short desired_angleturn = cmd->angleturn + carry;

        // round angleturn to the nearest 256 unit boundary
        // for recording demos with single byte values for turn

        cmd->angleturn = (desired_angleturn + 128) & 0xff00;

        // Carry forward the error from the reduced resolution to the
        // next tic, so that successive small movements can accumulate.

        carry = desired_angleturn - cmd->angleturn;
    }
}


//
// G_DoLoadLevel
//
void G_DoLoadLevel(void) {
    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.

    skyflatnum = R_FlatNumForName(DEH_String(SKYFLATNAME));

    levelstarttic = gametic; // for time calculation

    if (wipegamestate == GS_LEVEL)
        wipegamestate = -1; // force a wipe

    gamestate = GS_LEVEL;

    for (int i = 0; i < MAXPLAYERS; i++) {
        turbodetected[i] = false;
        if (playeringame[i] && players[i].playerstate == PST_DEAD)
            players[i].playerstate = PST_REBORN;
        memset(players[i].frags, 0, sizeof(players[i].frags));
    }

    P_SetupLevel(gameepisode, gamemap, 0, gameskill);
    displayplayer = consoleplayer; // view the guy you are playing
    gameaction = ga_nothing;
    Z_CheckHeap();

    // clear cmd building stuff

    memset(gamekeydown, 0, sizeof(gamekeydown));
    joyxmove = joyymove = joystrafemove = 0;
    mousex = mousey = 0;
    sendpause = sendsave = paused = false;
    memset(mousearray, 0, sizeof(mousearray));
    memset(joyarray, 0, sizeof(joyarray));

    if (testcontrols) {
        players[consoleplayer].message = "Press escape to quit.";
    }
}

static void SetJoyButtons(unsigned int buttons_mask) {
    for (int i = 0; i < MAX_JOY_BUTTONS; ++i) {
        const int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:
        if (!joybuttons[i] && button_on) {
            // Weapon cycling:
            if (i == joybprevweapon) {
                next_weapon = -1;
            } else if (i == joybnextweapon) {
                next_weapon = 1;
            }
        }

        joybuttons[i] = button_on;
    }
}

static void SetMouseButtons(unsigned int buttons_mask) {
    for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i) {
        unsigned int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:

        if (!mousebuttons[i] && button_on) {
            if (i == mousebprevweapon) {
                next_weapon = -1;
            } else if (i == mousebnextweapon) {
                next_weapon = 1;
            }
        }

        mousebuttons[i] = button_on;
    }
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder(event_t *ev) {
    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo &&
        (demoplayback || gamestate == GS_DEMOSCREEN)
    ) {
        if (ev->type == ev_keydown ||
            (ev->type == ev_mouse && ev->data1) ||
            (ev->type == ev_joystick && ev->data1)) {
            M_StartControlPanel();
            return true;
        }
        return false;
    }

    if (gamestate == GS_LEVEL) {
#if 0
        if (devparm &&ev->type == ev_keydown && ev->data1 == ';')
        {
            G_DeathMatchSpawnPlayer(0);
            return true;
        }
#endif
        if (HU_Responder(ev))
            return true; // HUD handled the event
        if (ST_Responder(ev))
            return true; // status window ate it
        if (AM_Responder(ev))
            return true; // automap ate it
    }

    if (gamestate == GS_FINALE) {
        if (F_Responder(ev))
            return true; // finale ate the event
    }

    if (testcontrols && ev->type == ev_mouse) {
        // If we are invoked by setup to test the controls, save the
        // mouse speed so that we can display it on-screen.
        // Perform a low pass filter on this so that the thermometer
        // appears to move smoothly.

        testcontrols_mousespeed = abs(ev->data2);
    }

    // If the next/previous weapon keys are pressed, set the next_weapon
    // variable to change weapons when the next ticcmd is generated.

    if (ev->type == ev_keydown && ev->data1 == key_prevweapon) {
        next_weapon = -1;
    } else if (ev->type == ev_keydown && ev->data1 == key_nextweapon) {
        next_weapon = 1;
    }

    switch (ev->type) {
        case ev_keydown:
            if (ev->data1 == key_pause) {
                sendpause = true;
            } else if (ev->data1 < NUMKEYS) {
                gamekeydown[ev->data1] = true;
            }

            return true; // eat key down events

        case ev_keyup:
            if (ev->data1 < NUMKEYS)
                gamekeydown[ev->data1] = false;
            return false; // always let key up events filter down

        case ev_mouse:
            SetMouseButtons(ev->data1);
            mousex = ev->data2 * (mouseSensitivity + 5) / 10;
            mousey = ev->data3 * (mouseSensitivity + 5) / 10;
            return true; // eat events

        case ev_joystick:
            SetJoyButtons(ev->data1);
            joyxmove = ev->data2;
            joyymove = ev->data3;
            joystrafemove = ev->data4;
            return true; // eat events

        default:
            break;
    }

    return false;
}


//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(void) {
    // do player reborns if needed
    for (int i = 0; i < MAXPLAYERS; i++)
        if (playeringame[i] && players[i].playerstate == PST_REBORN)
            G_DoReborn(i);

    // do things to change the game state
    while (gameaction != ga_nothing) {
        switch (gameaction) {
            case ga_loadlevel:
                G_DoLoadLevel();
                break;
            case ga_newgame:
                G_DoNewGame();
                break;
            case ga_loadgame:
                G_DoLoadGame();
                break;
            case ga_savegame:
                G_DoSaveGame();
                break;
            case ga_playdemo:
                G_DoPlayDemo();
                break;
            case ga_completed:
                G_DoCompleted();
                break;
            case ga_victory:
                F_StartFinale();
                break;
            case ga_worlddone:
                G_DoWorldDone();
                break;
            case ga_screenshot:
                V_ScreenShot("DOOM%02i.%s");
                players[consoleplayer].message = DEH_String("screen shot");
                gameaction = ga_nothing;
                break;
            case ga_nothing:
                break;
        }
    }

    // Copy local input or read the next demo command.

    for (int i = 0; i < MAXPLAYERS; i++) {
        if (playeringame[i]) {
            ticcmd_t *cmd = &players[i].cmd;

            memcpy(cmd, localcmds, sizeof(ticcmd_t));

            if (demoplayback)
                G_ReadDemoTiccmd(cmd);
            if (demorecording)
                G_WriteDemoTiccmd(cmd);

            // check for turbo cheats

            // check ~ 4 seconds whether to display the turbo message.
            // store if the turbo threshold was exceeded in any tics
            // over the past 4 seconds.  offset the checking period
            // for each player so messages are not displayed at the
            // same time.

            if (cmd->forwardmove > TURBOTHRESHOLD) {
                turbodetected[i] = true;
            }

            if ((gametic & 31) == 0
                && ((gametic >> 5) % MAXPLAYERS) == i
                && turbodetected[i]) {
                static char turbomessage[80];
                extern char *player_names[4];
                M_snprintf(turbomessage, sizeof(turbomessage),
                           "%s is turbo!", player_names[i]);
                players[consoleplayer].message = turbomessage;
                turbodetected[i] = false;
            }
        }
    }

    // check for special buttons
    for (int i = 0; i < MAXPLAYERS; i++) {
        if (playeringame[i]) {
            if (players[i].cmd.buttons & BT_SPECIAL) {
                switch (players[i].cmd.buttons & BT_SPECIALMASK) {
                    case BTS_PAUSE:
                        paused ^= 1;
                        if (paused)
                            S_PauseSound();
                        else
                            S_ResumeSound();
                        break;

                    case BTS_SAVEGAME:
                        if (!savedescription[0]) {
                            M_StringCopy(savedescription, "NET GAME",
                                         sizeof(savedescription));
                        }

                        savegameslot =
                                (players[i].cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
                        gameaction = ga_savegame;
                        break;
                }
            }
        }
    }

    // Have we just finished displaying an intermission screen?

    if (oldgamestate == GS_INTERMISSION && gamestate != GS_INTERMISSION) {
        WI_End();
    }

    oldgamestate = gamestate;

    // do main actions
    switch (gamestate) {
        case GS_LEVEL:
            P_Ticker();
            ST_Ticker();
            AM_Ticker();
            HU_Ticker();
            break;

        case GS_INTERMISSION:
            WI_Ticker();
            break;

        case GS_FINALE:
            F_Ticker();
            break;

        case GS_DEMOSCREEN:
            D_PageTicker();
            break;
    }
}


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer(int player) {
    // clear everything else to defaults
    G_PlayerReborn(player);
}


//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel(int player) {
    player_t *p = &players[player];

    memset(p->powers, 0, sizeof (p->powers));
    memset(p->cards, 0, sizeof (p->cards));
    p->mo->flags &= ~MF_SHADOW; // cancel invisibility
    p->extralight = 0; // cancel gun flashes
    p->fixedcolormap = 0; // cancel ir gogles
    p->damagecount = 0; // no palette changes
    p->bonuscount = 0;
}


//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn(int player) {
    int frags[MAXPLAYERS];

    memcpy(frags, players[player].frags, sizeof(frags));
    int killcount = players[player].killcount;
    int itemcount = players[player].itemcount;
    int secretcount = players[player].secretcount;

    player_t *p = &players[player];
    memset(p, 0, sizeof(*p));

    memcpy(players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].itemcount = itemcount;
    players[player].secretcount = secretcount;

    p->usedown = p->attackdown = true; // don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = deh_initial_health; // Use dehacked value
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = deh_initial_bullets;

    for (int i = 0; i < NUMAMMO; i++)
        p->maxammo[i] = maxammo[i];
}

//
// G_DoReborn
//
void G_DoReborn(int playernum) {
    gameaction = ga_loadlevel;
}


void G_ScreenShot(void) {
    gameaction = ga_screenshot;
}


// DOOM Par Times
int pars[4][10] =
{
    {0},
    {0, 30, 75, 120, 90, 165, 180, 180, 30, 165},
    {0, 90, 90, 90, 120, 90, 360, 240, 30, 170},
    {0, 90, 45, 90, 150, 90, 90, 165, 30, 135}
};

// DOOM II Par Times
int cpars[32] =
{
    30, 90, 120, 120, 90, 150, 120, 120, 270, 90, //  1-10
    210, 150, 150, 150, 210, 150, 420, 150, 210, 150, // 11-20
    240, 150, 180, 150, 150, 300, 330, 420, 300, 180, // 21-30
    120, 30 // 31-32
};


//
// G_DoCompleted
//
boolean secretexit;
extern char *pagename;

void G_ExitLevel(void) {
    secretexit = false;
    gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel(void) {
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ((gamemode == commercial)
        && (W_CheckNumForName("map31") < 0))
        secretexit = false;
    else
        secretexit = true;
    gameaction = ga_completed;
}

void G_DoCompleted(void) {
    gameaction = ga_nothing;

    for (int i = 0; i < MAXPLAYERS; i++)
        if (playeringame[i])
            G_PlayerFinishLevel(i); // take away cards and stuff

    if (automapactive)
        AM_Stop();

    if (gamemode != commercial) {
        switch (gamemap) {
            case 8:
                gameaction = ga_victory;
                return;
            case 9:
                for (int i = 0; i < MAXPLAYERS; i++)
                    players[i].didsecret = true;
                break;
        }
    }

    //#if 0  Hmmm - why?
    if (gamemap == 8
        && (gamemode != commercial)) {
        // victory
        gameaction = ga_victory;
        return;
    }

    if (gamemap == 9
        && (gamemode != commercial)) {
        // exit secret level
        for (int i = 0; i < MAXPLAYERS; i++)
            players[i].didsecret = true;
    }
    //#endif


    wminfo.didsecret = players[consoleplayer].didsecret;
    wminfo.epsd = gameepisode - 1;
    wminfo.last = gamemap - 1;

    // wminfo.next is 0 biased, unlike gamemap
    if (gamemode == commercial) {
        if (secretexit)
            switch (gamemap) {
                case 15: wminfo.next = 30;
                    break;
                case 31: wminfo.next = 31;
                    break;
            }
        else
            switch (gamemap) {
                case 31:
                case 32: wminfo.next = 15;
                    break;
                default: wminfo.next = gamemap;
            }
    } else {
        if (secretexit)
            wminfo.next = 8; // go to secret level
        else if (gamemap == 9) {
            // returning from secret level
            switch (gameepisode) {
                case 1:
                    wminfo.next = 3;
                    break;
                case 2:
                    wminfo.next = 5;
                    break;
                case 3:
                    wminfo.next = 6;
                    break;
                case 4:
                    wminfo.next = 2;
                    break;
            }
        } else
            wminfo.next = gamemap; // go to next level
    }

    wminfo.maxkills = totalkills;
    wminfo.maxitems = totalitems;
    wminfo.maxsecret = totalsecret;
    wminfo.maxfrags = 0;

    // Set par time. Doom episode 4 doesn't have a par time, so this
    // overflows into the cpars array. It's necessary to emulate this
    // for statcheck regression testing.
    if (gamemode == commercial)
        wminfo.partime = TICRATE * cpars[gamemap - 1];
    else if (gameepisode < 4)
        wminfo.partime = TICRATE * pars[gameepisode][gamemap];
    else
        wminfo.partime = TICRATE * cpars[gamemap];

    wminfo.pnum = consoleplayer;

    for (int i = 0; i < MAXPLAYERS; i++) {
        wminfo.plyr[i].in = playeringame[i];
        wminfo.plyr[i].skills = players[i].killcount;
        wminfo.plyr[i].sitems = players[i].itemcount;
        wminfo.plyr[i].ssecret = players[i].secretcount;
        wminfo.plyr[i].stime = leveltime;
        memcpy(wminfo.plyr[i].frags, players[i].frags
               , sizeof(wminfo.plyr[i].frags));
    }

    gamestate = GS_INTERMISSION;
    viewactive = false;
    automapactive = false;


    WI_Start(&wminfo);
}


//
// G_WorldDone
//
void G_WorldDone(void) {
    gameaction = ga_worlddone;

    if (secretexit)
        players[consoleplayer].didsecret = true;

    if (gamemode == commercial) {
        switch (gamemap) {
            case 15:
            case 31:
                if (!secretexit)
                    break;
            case 6:
            case 11:
            case 20:
            case 30:
                F_StartFinale();
                break;
        }
    }
}

void G_DoWorldDone(void) {
    gamestate = GS_LEVEL;
    gamemap = wminfo.next + 1;
    G_DoLoadLevel();
    gameaction = ga_nothing;
    viewactive = true;
}


//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
extern boolean setsizeneeded;

void R_ExecuteSetViewSize(void);

char savename[256];

void G_LoadGame(char *name) {
    M_StringCopy(savename, name, sizeof(savename));
    gameaction = ga_loadgame;
}

#define VERSIONSIZE		16


void G_DoLoadGame(void) {
    gameaction = ga_nothing;

    save_stream = fopen(savename, "rb");

    if (save_stream == NULL) {
        return;
    }

    savegame_error = false;

    if (!P_ReadSaveGameHeader()) {
        fclose(save_stream);
        return;
    }

    int savedleveltime = leveltime;

    // load a base level
    G_InitNew(gameskill, gameepisode, gamemap);

    leveltime = savedleveltime;

    // dearchive all the modifications
    P_UnArchivePlayers();
    P_UnArchiveWorld();
    P_UnArchiveThinkers();
    P_UnArchiveSpecials();

    if (!P_ReadSaveGameEOF())
        I_Error("Bad savegame");

    fclose(save_stream);

    if (setsizeneeded)
        R_ExecuteSetViewSize();

    // draw the pattern into the back screen
    R_FillBackScreen();
}


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void
G_SaveGame
(int slot,
 char *description) {
    savegameslot = slot;
    M_StringCopy(savedescription, description, sizeof(savedescription));
    sendsave = true;
}

void G_DoSaveGame(void) {
    char *recovery_savegame_file = NULL;
    char *temp_savegame_file = P_TempSaveGameFile();
    char *savegame_file = P_SaveGameFile(savegameslot);

    // Open the savegame file for writing.  We write to a temporary file
    // and then rename it at the end if it was successfully written.
    // This prevents an existing savegame from being overwritten by
    // a corrupted one, or if a savegame buffer overrun occurs.
    save_stream = fopen(temp_savegame_file, "wb");

    if (save_stream == NULL) {
        // Failed to save the game, so we're going to have to abort. But
        // to be nice, save to somewhere else before we call I_Error().
        recovery_savegame_file = M_TempFile("recovery.dsg");
        save_stream = fopen(recovery_savegame_file, "wb");
        if (save_stream == NULL) {
            I_Error("Failed to open either '%s' or '%s' to write savegame.",
                    temp_savegame_file, recovery_savegame_file);
        }
    }

    savegame_error = false;

    P_WriteSaveGameHeader(savedescription);

    P_ArchivePlayers();
    P_ArchiveWorld();
    P_ArchiveThinkers();
    P_ArchiveSpecials();

    P_WriteSaveGameEOF();

    // Enforce the same savegame size limit as in Vanilla Doom,
    // except if the vanilla_savegame_limit setting is turned off.

    if (vanilla_savegame_limit && ftell(save_stream) > SAVEGAMESIZE) {
        I_Error("Savegame buffer overrun");
    }

    // Finish up, close the savegame file.

    fclose(save_stream);

    if (recovery_savegame_file != NULL) {
        // We failed to save to the normal location, but we wrote a
        // recovery file to the temp directory. Now we can bomb out
        // with an error.
        I_Error("Failed to open savegame file '%s' for writing.\n"
                "But your game has been saved to '%s' for recovery.",
                temp_savegame_file, recovery_savegame_file);
    }

    // Now rename the temporary savegame file to the actual savegame
    // file, overwriting the old savegame if there was one there.

    remove(savegame_file);
    rename(temp_savegame_file, savegame_file);

    gameaction = ga_nothing;
    M_StringCopy(savedescription, "", sizeof(savedescription));

    players[consoleplayer].message = DEH_String(GGSAVED);

    // draw the pattern into the back screen
    R_FillBackScreen();
}


//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
skill_t d_skill;
int d_episode;
int d_map;

void
G_DeferedInitNew
(skill_t skill,
 int episode,
 int map) {
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    gameaction = ga_newgame;
}


void G_DoNewGame(void) {
    demoplayback = false;


    playeringame[1] = playeringame[2] = playeringame[3] = 0;
    respawnparm = false;
    fastparm = false;
    nomonsters = false;
    consoleplayer = 0;
    G_InitNew(d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}


void
G_InitNew
(skill_t skill,
 int episode,
 int map) {
    char *skytexturename;

    if (paused) {
        paused = false;
        S_ResumeSound();
    }

    /*
    // Note: This commented-out block of code was added at some point
    // between the DOS version(s) and the Doom source release. It isn't
    // found in disassemblies of the DOS version and causes IDCLEV and
    // the -warp command line parameter to behave differently.
    // This is left here for posterity.

    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
      episode = 1;

    if ( gamemode == retail )
    {
      if (episode > 4)
	episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1)
	   episode = 1;	// only start episode 1 on shareware
    }
    else
    {
      if (episode > 3)
	episode = 3;
    }
    */

    if (skill > sk_nightmare)
        skill = sk_nightmare;

    if (gameversion >= exe_ultimate) {
        if (episode == 0) {
            episode = 4;
        }
    } else {
        if (episode < 1) {
            episode = 1;
        }
        if (episode > 3) {
            episode = 3;
        }
    }

    if (episode > 1 && gamemode == shareware) {
        episode = 1;
    }

    if (map < 1)
        map = 1;

    if ((map > 9)
        && (gamemode != commercial))
        map = 9;

    if (!D_ValidEpisodeMap(gamemission, gamemode,
                           gamemode == commercial ? 1 : episode, map))
        I_Error("Unsupported episode/map: %d/%d", episode, map);

    M_ClearRandom();

    if (skill == sk_nightmare || respawnparm)
        respawnmonsters = true;
    else
        respawnmonsters = false;

    if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare)) {
        for (int i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics >>= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
    } else if (skill != sk_nightmare && gameskill == sk_nightmare) {
        for (int i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
    }

    // force players to be initialized upon first level load
    for (int i = 0; i < MAXPLAYERS; i++)
        players[i].playerstate = PST_REBORN;

    usergame = true; // will be set false if a demo
    paused = false;
    demoplayback = false;
    automapactive = false;
    viewactive = true;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;

    viewactive = true;

    // Set the sky to use.
    //
    // Note: This IS broken, but it is how Vanilla Doom behaves.
    // See http://doomwiki.org/wiki/Sky_never_changes_in_Doom_II.
    //
    // Because we set the sky here at the start of a game, not at the
    // start of a level, the sky texture never changes unless we
    // restore from a saved game.  This was fixed before the Doom
    // source release, but this IS the way Vanilla DOS Doom behaves.

    if (gamemode == commercial) {
        if (gamemap < 12)
            skytexturename = "SKY1";
        else if (gamemap < 21)
            skytexturename = "SKY2";
        else
            skytexturename = "SKY3";
    } else {
        switch (gameepisode) {
            default:
            case 1:
                skytexturename = "SKY1";
                break;
            case 2:
                skytexturename = "SKY2";
                break;
            case 3:
                skytexturename = "SKY3";
                break;
            case 4: // Special Edition sky
                skytexturename = "SKY4";
                break;
        }
    }

    skytexturename = DEH_String(skytexturename);

    skytexture = R_TextureNumForName(skytexturename);


    G_DoLoadLevel();
}


//
// DEMO RECORDING
//
#define DEMOMARKER		0x80


void G_ReadDemoTiccmd(ticcmd_t *cmd) {
    if (*demo_p == DEMOMARKER) {
        // end of demo data stream
        G_CheckDemoStatus();
        return;
    }
    cmd->forwardmove = ((signed char) *demo_p++);
    cmd->sidemove = ((signed char) *demo_p++);

    // If this is a longtics demo, read back in higher resolution

    if (longtics) {
        cmd->angleturn = *demo_p++;
        cmd->angleturn |= (*demo_p++) << 8;
    } else {
        cmd->angleturn = ((unsigned char) *demo_p++) << 8;
    }

    cmd->buttons = (unsigned char) *demo_p++;
}

// Increase the size of the demo buffer to allow unlimited demos

static void IncreaseDemoBuffer(void) {
    // Find the current size

    int current_length = demoend - demobuffer;

    // Generate a new buffer twice the size
    int new_length = current_length * 2;

    byte *new_demobuffer = Z_Malloc(new_length, PU_STATIC, 0);
    byte *new_demop = new_demobuffer + (demo_p - demobuffer);

    // Copy over the old data

    memcpy(new_demobuffer, demobuffer, current_length);

    // Free the old buffer and point the demo pointers at the new buffer.

    Z_Free(demobuffer);

    demobuffer = new_demobuffer;
    demo_p = new_demop;
    demoend = demobuffer + new_length;
}

void G_WriteDemoTiccmd(ticcmd_t *cmd) {
    if (gamekeydown[key_demo_quit]) // press q to end demo recording
        G_CheckDemoStatus();

    byte *demo_start = demo_p;

    *demo_p++ = cmd->forwardmove;
    *demo_p++ = cmd->sidemove;

    // If this is a longtics demo, record in higher resolution

    if (longtics) {
        *demo_p++ = (cmd->angleturn & 0xff);
        *demo_p++ = (cmd->angleturn >> 8) & 0xff;
    } else {
        *demo_p++ = cmd->angleturn >> 8;
    }

    *demo_p++ = cmd->buttons;

    // reset demo pointer back
    demo_p = demo_start;

    if (demo_p > demoend - 16) {
        if (vanilla_demo_limit) {
            // no more space
            G_CheckDemoStatus();
            return;
        } else {
            // Vanilla demo limit disabled: unlimited
            // demo lengths!

            IncreaseDemoBuffer();
        }
    }

    G_ReadDemoTiccmd(cmd); // make SURE it is exactly the same
}


//
// G_RecordDemo
//
void G_RecordDemo(char *name) {
    usergame = false;
    size_t demoname_size = strlen(name) + 5;
    demoname = Z_Malloc(demoname_size, PU_STATIC, NULL);
    M_snprintf(demoname, demoname_size, "%s.lmp", name);
    int maxsize = 0x20000;

    //!
    // @arg <size>
    // @category demo
    // @vanilla
    //
    // Specify the demo buffer size (KiB)
    //

    int i = M_CheckParmWithArgs("-maxdemo", 1);
    if (i)
        maxsize = atoi(myargv[i + 1]) * 1024;
    demobuffer = Z_Malloc(maxsize, PU_STATIC, NULL);
    demoend = demobuffer + maxsize;

    demorecording = true;
}

// Get the demo version code appropriate for the version set in gameversion.
int G_VanillaVersionCode(void) {
    switch (gameversion) {
        case exe_doom_1_2:
            I_Error("Doom 1.2 does not have a version code!");
        case exe_doom_1_666:
            return 106;
        case exe_doom_1_7:
            return 107;
        case exe_doom_1_8:
            return 108;
        case exe_doom_1_9:
        default: // All other versions are variants on v1.9:
            return 109;
    }
}

void G_BeginRecording(void) {
    //!
    // @category demo
    //
    // Record a high resolution "Doom 1.91" demo.
    //

    longtics = M_CheckParm("-longtics") != 0;

    // If not recording a longtics demo, record in low res

    lowres_turn = !longtics;

    demo_p = demobuffer;

    // Save the right version code for this demo

    if (longtics) {
        *demo_p++ = DOOM_191_VERSION;
    } else {
        *demo_p++ = G_VanillaVersionCode();
    }

    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = 0; // Single-player demo.
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;

    for (int i = 0; i < MAXPLAYERS; i++)
        *demo_p++ = playeringame[i];
}


//
// G_PlayDemo
//

char *defdemoname;

void G_DeferedPlayDemo(char *name) {
    defdemoname = name;
    gameaction = ga_playdemo;
}

// Generate a string describing a demo version

static char *DemoVersionDescription(int version) {
    static char resultbuf[16];

    switch (version) {
        case 104:
            return "v1.4";
        case 105:
            return "v1.5";
        case 106:
            return "v1.6/v1.666";
        case 107:
            return "v1.7/v1.7a";
        case 108:
            return "v1.8";
        case 109:
            return "v1.9";
        default:
            break;
    }

    // Unknown version.  Perhaps this is a pre-v1.4 IWAD?  If the version
    // byte is in the range 0-4 then it can be a v1.0-v1.2 demo.

    if (version >= 0 && version <= 4) {
        return "v1.0/v1.1/v1.2";
    } else {
        M_snprintf(resultbuf, sizeof(resultbuf),
                   "%i.%i (unknown)", version / 100, version % 100);
        return resultbuf;
    }
}

void G_DoPlayDemo(void) {
    gameaction = ga_nothing;
    demobuffer = demo_p = W_CacheLumpName(defdemoname, PU_STATIC);

    int demoversion = *demo_p++;

    if (demoversion == G_VanillaVersionCode()) {
        longtics = false;
    } else if (demoversion == DOOM_191_VERSION) {
        // demo recorded with cph's modified "v1.91" doom exe
        longtics = true;
    } else {
        char *message = "Demo is from a different game version!\n"
                "(read %i, should be %i)\n"
                "\n"
                "*** You may need to upgrade your version "
                "of Doom to v1.9. ***\n"
                "    See: https://www.doomworld.com/classicdoom"
                "/info/patches.php\n"
                "    This appears to be %s.";

        //I_Error(message, demoversion, G_VanillaVersionCode(),
        printf(message, demoversion, G_VanillaVersionCode(),
               DemoVersionDescription(demoversion));
    }

    skill_t skill = *demo_p++;
    int episode = *demo_p++;
    int map = *demo_p++;
    if (*demo_p++ != 0)
        I_Error("Multiplayer demos are not supported");
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    nomonsters = *demo_p++;
    consoleplayer = *demo_p++;

    for (int i = 0; i < MAXPLAYERS; i++)
        playeringame[i] = *demo_p++;

    if (consoleplayer != 0 || !playeringame[0]
        || playeringame[1] || playeringame[2] || playeringame[3])
        I_Error("Multiplayer demos are not supported");

    // don't spend a lot of time in loadlevel
    precache = false;
    G_InitNew(skill, episode, map);
    precache = true;
    starttime = I_GetTime();

    usergame = false;
    demoplayback = true;
}

//
// G_TimeDemo
//
void G_TimeDemo(char *name) {
    //!
    // @vanilla
    //
    // Disable rendering the screen entirely.
    //

    nodrawers = M_CheckParm("-nodraw");

    timingdemo = true;
    singletics = true;

    defdemoname = name;
    gameaction = ga_playdemo;
}


/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

boolean G_CheckDemoStatus(void) {
    if (timingdemo) {
        int endtime = I_GetTime();
        int realtics = endtime - starttime;
        float fps = ((float)gametic * TICRATE) / realtics;

        // Prevent recursive calls
        timingdemo = false;
        demoplayback = false;

        I_Error("timed %i gametics in %i realtics (%f fps)",
                gametic, realtics, fps);
    }

    if (demoplayback) {
        W_ReleaseLumpName(defdemoname);
        demoplayback = false;


        playeringame[1] = playeringame[2] = playeringame[3] = 0;
        respawnparm = false;
        fastparm = false;
        nomonsters = false;
        consoleplayer = 0;

        if (singledemo)
            I_Quit();
        else
            D_AdvanceDemo();

        return true;
    }

    if (demorecording) {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(demoname, demobuffer, demo_p - demobuffer);
        Z_Free(demobuffer);
        demorecording = false;
        I_Error("Demo %s recorded", demoname);
    }

    return false;
} 
 
 
 
