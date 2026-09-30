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
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//


#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "z_zone.h"

#include "i_swap.h"
#include "m_argv.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"


void P_SpawnMapThing(mapthing_t * mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int numvertexes;
vertex_t *vertexes;

int numsegs;
seg_t *segs;

int numsectors;
sector_t *sectors;

int numsubsectors;
subsector_t *subsectors;

int numnodes;
node_t *nodes;

int numlines;
line_t *lines;

int numsides;
side_t *sides;

static int totallines;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int bmapwidth;
int bmapheight; // size in mapblocks
int16_t *blockmap; // int for larger maps
// offsets in blockmap are from here
int16_t *blockmaplump;
// origin of block map
fixed_t bmaporgx;
fixed_t bmaporgy;
// for thing chains
mobj_t **blocklinks;


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
uint8_t *rejectmatrix;


// Maintain single and multi player starting spots.

mapthing_t playerstarts[MAXPLAYERS];


//
// P_LoadVertexes
//
void P_LoadVertexes(int lump) {
    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);

    // Load data into cache.
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapvertex_t *ml = (mapvertex_t *)data;
    vertex_t *li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (int i = 0; i < numvertexes; i++, li++, ml++) {
        li->x = SHORT(ml->x) << FRACBITS;
        li->y = SHORT(ml->y) << FRACBITS;
    }

    // Free buffer memory.
    W_ReleaseLumpNum(lump);
}

//
// GetSectorAtNullAddress
//
sector_t *GetSectorAtNullAddress(void) {
    static bool null_sector_is_initialized = false;
    static sector_t null_sector;

    if (!null_sector_is_initialized) {
        memset(&null_sector, 0, sizeof(null_sector));
        I_GetMemoryValue(0, &null_sector.floorheight, 4);
        I_GetMemoryValue(4, &null_sector.ceilingheight, 4);
        null_sector_is_initialized = true;
    }

    return &null_sector;
}

//
// P_LoadSegs
//
void P_LoadSegs(int lump) {
    numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
    memset(segs, 0, numsegs * sizeof(seg_t));
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapseg_t *ml = (mapseg_t *)data;
    seg_t *li = segs;
    for (int i = 0; i < numsegs; i++, li++, ml++) {
        li->v1 = &vertexes[SHORT(ml->v1)];
        li->v2 = &vertexes[SHORT(ml->v2)];

        li->angle = (SHORT(ml->angle)) << 16;
        li->offset = (SHORT(ml->offset)) << 16;
        int linedef = SHORT(ml->linedef);
        line_t *ldef = &lines[linedef];
        li->linedef = ldef;
        int side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        if (ldef->flags & ML_TWOSIDED) {
            int sidenum = ldef->sidenum[side ^ 1];

            // If the sidenum is out of range, this may be a "glass hack"
            // impassible window.  Point at side #0 (this may not be
            // the correct Vanilla behavior; however, it seems to work for
            // OTTAWAU.WAD, which is the one place I've seen this trick
            // used).

            if (sidenum < 0 || sidenum >= numsides) {
                li->backsector = GetSectorAtNullAddress();
            } else {
                li->backsector = sides[sidenum].sector;
            }
        } else {
            li->backsector = 0;
        }
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors(int lump) {
    numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapsubsector_t *ms = (mapsubsector_t *)data;
    memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
    subsector_t *ss = subsectors;

    for (int i = 0; i < numsubsectors; i++, ss++, ms++) {
        ss->numlines = SHORT(ms->numsegs);
        ss->firstline = SHORT(ms->firstseg);
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSectors
//
void P_LoadSectors(int lump) {
    numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
    sectors = Z_Malloc(numsectors * sizeof(sector_t), PU_LEVEL, 0);
    memset(sectors, 0, numsectors * sizeof(sector_t));
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapsector_t *ms = (mapsector_t *)data;
    sector_t *ss = sectors;
    for (int i = 0; i < numsectors; i++, ss++, ms++) {
        ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
        ss->ceilingheight = SHORT(ms->ceilingheight) << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SHORT(ms->lightlevel);
        ss->special = SHORT(ms->special);
        ss->tag = SHORT(ms->tag);
        ss->thinglist = NULL;
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadNodes
//
void P_LoadNodes(int lump) {
    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    nodes = Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapnode_t *mn = (mapnode_t *)data;
    node_t *no = nodes;

    for (int i = 0; i < numnodes; i++, no++, mn++) {
        no->x = SHORT(mn->x) << FRACBITS;
        no->y = SHORT(mn->y) << FRACBITS;
        no->dx = SHORT(mn->dx) << FRACBITS;
        no->dy = SHORT(mn->dy) << FRACBITS;
        for (int j = 0; j < 2; j++) {
            no->children[j] = SHORT(mn->children[j]);
            for (int k = 0; k < 4; k++)
                no->bbox[j][k] = SHORT(mn->bbox[j][k]) << FRACBITS;
        }
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadThings
//
void P_LoadThings(int lump) {
    mapthing_t spawnthing;

    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);
    int numthings = W_LumpLength(lump) / sizeof(mapthing_t);

    mapthing_t *mt = (mapthing_t *)data;
    for (int i = 0; i < numthings; i++, mt++) {
        bool spawn = true;

        // Do not spawn cool, new monsters if !commercial
        if (gamemode != commercial) {
            switch (SHORT(mt->type)) {
                case 68: // Arachnotron
                case 64: // Archvile
                case 88: // Boss Brain
                case 89: // Boss Shooter
                case 69: // Hell Knight
                case 67: // Mancubus
                case 71: // Pain Elemental
                case 65: // Former Human Commando
                case 66: // Revenant
                case 84: // Wolf SS
                    spawn = false;
                    break;
            }
        }
        if (spawn == false)
            break;

        // Do spawn all other stuff.
        spawnthing.x = SHORT(mt->x);
        spawnthing.y = SHORT(mt->y);
        spawnthing.angle = SHORT(mt->angle);
        spawnthing.type = SHORT(mt->type);
        spawnthing.options = SHORT(mt->options);

        P_SpawnMapThing(&spawnthing);
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs(int lump) {
    numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines = Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
    memset(lines, 0, numlines * sizeof(line_t));
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    maplinedef_t *mld = (maplinedef_t *)data;
    line_t *ld = lines;
    for (int i = 0; i < numlines; i++, mld++, ld++) {
        ld->flags = SHORT(mld->flags);
        ld->special = SHORT(mld->special);
        ld->tag = SHORT(mld->tag);
        vertex_t *v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
        vertex_t *v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;

        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else {
            if (FixedDiv(ld->dy, ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x) {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        } else {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y) {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        } else {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        ld->sidenum[0] = SHORT(mld->sidenum[0]);
        ld->sidenum[1] = SHORT(mld->sidenum[1]);

        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;

        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs(int lump) {
    numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
    memset(sides, 0, numsides * sizeof(side_t));
    uint8_t *data = W_CacheLumpNum(lump, PU_STATIC);

    mapsidedef_t *msd = (mapsidedef_t *)data;
    side_t *sd = sides;
    for (int i = 0; i < numsides; i++, msd++, sd++) {
        sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
        sd->rowoffset = SHORT(msd->rowoffset) << FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[SHORT(msd->sector)];
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadBlockMap
//
void P_LoadBlockMap(int lump) {
    int lumplen = W_LumpLength(lump);
    int count = lumplen / 2;

    blockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, blockmaplump);
    blockmap = blockmaplump + 4;

    // Swap all short integers to native byte ordering.

    for (int i = 0; i < count; i++) {
        blockmaplump[i] = SHORT(blockmaplump[i]);
    }

    // Read the header

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // Clear out mobj chains

    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
}


//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines(void) {
    sector_t *sector;
    fixed_t bbox[4];

    // look up sector number for each subsector
    subsector_t *ss = subsectors;
    for (int i = 0; i < numsubsectors; i++, ss++) {
        seg_t *seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    line_t *li = lines;
    totallines = 0;
    for (int i = 0; i < numlines; i++, li++) {
        totallines++;
        li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector) {
            li->backsector->linecount++;
            totallines++;
        }
    }

    // build line tables for each sector
    line_t **linebuffer = Z_Malloc(totallines * sizeof(line_t *), PU_LEVEL, 0);

    for (int i = 0; i < numsectors; ++i) {
        // Assign the line buffer for this sector

        sectors[i].lines = linebuffer;
        linebuffer += sectors[i].linecount;

        // Reset linecount to zero so in the next stage we can count
        // lines into the list.

        sectors[i].linecount = 0;
    }

    // Assign lines to sectors

    for (int i = 0; i < numlines; ++i) {
        li = &lines[i];

        if (li->frontsector != NULL) {
            sector = li->frontsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }

        if (li->backsector != NULL && li->frontsector != li->backsector) {
            sector = li->backsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }
    }

    // Generate bounding boxes for sectors

    sector = sectors;
    for (int i = 0; i < numsectors; i++, sector++) {
        M_ClearBox(bbox);

        for (int j = 0; j < sector->linecount; j++) {
            li = sector->lines[j];

            M_AddToBox(bbox, li->v1->x, li->v1->y);
            M_AddToBox(bbox, li->v2->x, li->v2->y);
        }

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
        sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

        // adjust bounding box to map blocks
        int block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight - 1 : block;
        sector->blockbox[BOXTOP] = block;

        block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM] = block;

        block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth - 1 : block;
        sector->blockbox[BOXRIGHT] = block;

        block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT] = block;
    }
}

// Pad the REJECT lump with extra data when the lump is too small,
// to simulate a REJECT buffer overflow in Vanilla Doom.

static void PadRejectArray(uint8_t *array, uint32_t len) {
    uint32_t padvalue;

    // Values to pad the REJECT array with:

    uint32_t rejectpad[4] =
    {
        ((totallines * 4 + 3) & ~3) + 24, // Size
        0, // Part of z_zone block header
        50, // PU_LEVEL
        0x1d4a11 // DOOM_CONST_ZONEID
    };

    // Copy values from rejectpad into the destination array.

    uint8_t *dest = array;

    for (uint32_t i = 0; i < len && i < sizeof(rejectpad); ++i) {
        uint32_t byte_num = i % 4;
        *dest = (rejectpad[i / 4] >> (byte_num * 8)) & 0xff;
        ++dest;
    }

    // We only have a limited pad size.  Print a warning if the
    // REJECT lump is too small.

    if (len > sizeof(rejectpad)) {
        fprintf(stderr, "PadRejectArray: REJECT lump too short to pad! (%" PRIu32 " > %zu)\n",
                len, sizeof(rejectpad));

        // Pad remaining space with 0 (or 0xff, if specified on command line).

        if (M_CheckParm("-reject_pad_with_ff")) {
            padvalue = 0xff;
        } else {
            padvalue = 0xf00;
        }

        memset(array + sizeof(rejectpad), padvalue, len - sizeof(rejectpad));
    }
}

static void P_LoadReject(int lumpnum) {
    // Calculate the size that the REJECT lump *should* be.

    int minlength = (numsectors * numsectors + 7) / 8;

    // If the lump meets the minimum length, it can be loaded directly.
    // Otherwise, we need to allocate a buffer of the correct size
    // and pad it with appropriate data.

    int lumplen = W_LumpLength(lumpnum);

    if (lumplen >= minlength) {
        rejectmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
    } else {
        rejectmatrix = Z_Malloc(minlength, PU_LEVEL, &rejectmatrix);
        W_ReadLump(lumpnum, rejectmatrix);

        PadRejectArray(rejectmatrix + lumplen, minlength - lumplen);
    }
}

//
// P_SetupLevel
//
void
P_SetupLevel
(int episode,
 int map,
 int playermask,
 skill_t skill) {
    char lumpname[9];

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (int i = 0; i < MAXPLAYERS; i++) {
        players[i].killcount = players[i].secretcount
                               = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1;

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start();

    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

    // UNUSED W_Profile ();
    P_InitThinkers();

    // find map name
    if (gamemode == commercial) {
        if (map < 10)
            snprintf(lumpname, 9, "map0%i", map);
        else
            snprintf(lumpname, 9, "map%i", map);
    } else {
        lumpname[0] = 'E';
        lumpname[1] = '0' + episode;
        lumpname[2] = 'M';
        lumpname[3] = '0' + map;
        lumpname[4] = 0;
    }

    int lumpnum = W_GetNumForName(lumpname);

    leveltime = 0;

    // note: most of this ordering is important
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

    P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);

    P_GroupLines();
    P_LoadReject(lumpnum + ML_REJECT);

    P_LoadThings(lumpnum + ML_THINGS);

    // set up world state
    P_SpawnSpecials();

    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
        R_PrecacheLevel();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());
}


//
// P_Init
//
void P_Init(void) {
    P_InitSwitchList();
    P_InitPicAnims();
    R_InitSprites(sprnames);
}



