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
//	Refresh of things, i.e. objects represented by sprites.
//


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include "doomdef.h"

#include "i_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "doomstat.h"


#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100

//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);


typedef struct {
    int x1;
    int x2;

    int column;
    int topclip;
    int bottomclip;
} maskdraw_t;


//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t pspritescale;
fixed_t pspriteiscale;

lighttable_t **spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
int16_t negonearray[SCREENWIDTH];
int16_t screenheightarray[SCREENWIDTH];


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t *sprites;
int numsprites;

spriteframe_t sprtemp[29];
int maxframe;
char *spritename;


//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void
R_InstallSpriteLump
(int lump,
 uint32_t frame,
 uint32_t rotation,
 bool flipped) {
    if (frame >= 29 || rotation > 8)
        I_Error("R_InstallSpriteLump: "
                "Bad frame characters in lump %i", lump);

    if ((int) frame > maxframe)
        maxframe = frame;

    if (rotation == 0) {
        // the lump should be used for all rotations
        if (sprtemp[frame].rotate == SPRITE_ROTATION_NONE)
            I_Error("R_InitSprites: Sprite %s frame %c has "
                    "multip rot=0 lump", spritename, 'A' + (int)frame);

        if (sprtemp[frame].rotate == SPRITE_ROTATION_EIGHT)
            I_Error("R_InitSprites: Sprite %s frame %c has rotations "
                    "and a rot=0 lump", spritename, 'A' + (int)frame);

        sprtemp[frame].rotate = SPRITE_ROTATION_NONE;
        for (int r = 0; r < 8; r++) {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (uint8_t) flipped;
        }
        return;
    }

    // the lump is only used for one rotation
    if (sprtemp[frame].rotate == SPRITE_ROTATION_NONE)
        I_Error("R_InitSprites: Sprite %s frame %c has rotations "
                "and a rot=0 lump", spritename, 'A' + (int)frame);

    sprtemp[frame].rotate = SPRITE_ROTATION_EIGHT;

    // make 0 based
    rotation--;
    if (sprtemp[frame].lump[rotation] != -1)
        I_Error("R_InitSprites: Sprite %s : %c : %c "
                "has two lumps mapped to it",
                spritename, 'A' + (int)frame, '1' + (int)rotation);

    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (uint8_t) flipped;
}


//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs(char **namelist) {
    int frame;
    int rotation;
    int patched;

    // count the number of sprite names
    char **check = namelist;
    while (*check != NULL)
        check++;

    numsprites = check - namelist;

    if (!numsprites)
        return;

    sprites = Z_Malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);

    int start = firstspritelump - 1;
    int end = lastspritelump + 1;

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints
    for (int i = 0; i < numsprites; i++) {
        spritename = namelist[i];
        memset(sprtemp, -1, sizeof(sprtemp));

        maxframe = -1;

        // scan the lumps,
        //  filling in the frames for whatever is found
        for (int l = start + 1; l < end; l++) {
            if (!strncasecmp(lumpinfo[l].name, spritename, 4)) {
                frame = lumpinfo[l].name[4] - 'A';
                rotation = lumpinfo[l].name[5] - '0';

                if (modifiedgame)
                    patched = W_GetNumForName(lumpinfo[l].name);
                else
                    patched = l;

                R_InstallSpriteLump(patched, frame, rotation, false);

                if (lumpinfo[l].name[6]) {
                    frame = lumpinfo[l].name[6] - 'A';
                    rotation = lumpinfo[l].name[7] - '0';
                    R_InstallSpriteLump(l, frame, rotation, true);
                }
            }
        }

        // check the frames that were found for completeness
        if (maxframe == -1) {
            sprites[i].numframes = 0;
            continue;
        }

        maxframe++;

        for (frame = 0; frame < maxframe; frame++) {
            switch (sprtemp[frame].rotate) {
                case SPRITE_ROTATION_UNSET:
                    // no rotations were found for that frame at all
                    I_Error("R_InitSprites: No patches found "
                            "for %s frame %c", spritename, frame + 'A');
                    break;

                case SPRITE_ROTATION_NONE:
                    // only the first rotation is needed
                    break;

                case SPRITE_ROTATION_EIGHT:
                    // must have all 8 frames
                    for (rotation = 0; rotation < 8; rotation++)
                        if (sprtemp[frame].lump[rotation] == -1)
                            I_Error("R_InitSprites: Sprite %s frame %c "
                                    "is missing rotations",
                                    spritename, frame + 'A');
                    break;
            }
        }

        // allocate space for the frames present and copy sprtemp to it
        sprites[i].numframes = maxframe;
        sprites[i].spriteframes =
                Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
        memcpy(sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
    }
}


//
// GAME FUNCTIONS
//
vissprite_t vissprites[MAXVISSPRITES];
vissprite_t *vissprite_p;
int newvissprite;


//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(char **namelist) {
    for (int i = 0; i < SCREENWIDTH; i++) {
        negonearray[i] = -1;
    }

    R_InitSpriteDefs(namelist);
}


//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void) {
    vissprite_p = vissprites;
}


//
// R_NewVisSprite
//
vissprite_t overflowsprite;

vissprite_t *R_NewVisSprite(void) {
    if (vissprite_p == &vissprites[MAXVISSPRITES])
        return &overflowsprite;

    vissprite_p++;
    return vissprite_p - 1;
}


//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
int16_t *mfloorclip;
int16_t *mceilingclip;

fixed_t spryscale;
fixed_t sprtopscreen;

void R_DrawMaskedColumn(column_t *column) {
    fixed_t basetexturemid = dc_texturemid;

    for (; column->topdelta != 0xff;) {
        // calculate unclipped screen coordinates
        //  for post
        int topscreen = sprtopscreen + spryscale * column->topdelta;
        int bottomscreen = topscreen + spryscale * column->length;

        dc_yl = (topscreen + FRACUNIT - 1) >> FRACBITS;
        dc_yh = (bottomscreen - 1) >> FRACBITS;

        if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x] - 1;
        if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x] + 1;

        if (dc_yl <= dc_yh) {
            dc_source = (uint8_t *) column + 3;
            dc_texturemid = basetexturemid - (column->topdelta << FRACBITS);
            // dc_source = (byte *)column + 3 - column->topdelta;

            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc();
        }
        column = (column_t *) ((uint8_t *) column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
}


//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
void
R_DrawVisSprite
(vissprite_t *vis,
 int x1,
 int x2) {
    patch_t *patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    dc_colormap = vis->colormap;

    if (!dc_colormap) {
        // NULL colormap = shadow draw
        colfunc = fuzzcolfunc;
    } else if (vis->mobjflags & MF_TRANSLATION) {
        colfunc = transcolfunc;
        dc_translation = translationtables - 256 +
                         ((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));
    }

    dc_iscale = abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    fixed_t frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale) {
        int texturecolumn = frac >> FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
            I_Error("R_DrawSpriteRange: bad texturecolumn");
#endif
        column_t *column = (column_t *)((uint8_t *)patch + LONG(patch->columnofs[texturecolumn]));
        R_DrawMaskedColumn(column);
    }

    colfunc = basecolfunc;
}


//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite(mobj_t *thing) {
    int lump;

    bool flip;

    // transform the origin point
    fixed_t tr_x = thing->x - viewx;
    fixed_t tr_y = thing->y - viewy;

    fixed_t gxt = FixedMul(tr_x, viewcos);
    fixed_t gyt = -FixedMul(tr_y, viewsin);

    fixed_t tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    fixed_t xscale = FixedDiv(projection, tz);

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    fixed_t tx = -(gyt + gxt);

    // too far off the side?
    if (abs(tx) > (tz << 2))
        return;

    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if ((uint32_t) thing->sprite >= (uint32_t) numsprites)
        I_Error("R_ProjectSprite: invalid sprite number %i ",
                thing->sprite);
#endif
    spritedef_t *sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
    if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame %i : %i ",
                thing->sprite, thing->frame);
#endif
    spriteframe_t *sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate == SPRITE_ROTATION_EIGHT) {
        // choose a different rotation based on player view
        angle_t ang = R_PointToAngle(thing->x, thing->y);
        uint32_t rot = (ang - thing->angle + (uint32_t)(ANG45 / 2) * 9) >> 29;
        lump = sprframe->lump[rot];
        flip = (bool) sprframe->flip[rot];
    } else {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = (bool) sprframe->flip[0];
    }

    // calculate edges of the shape
    tx -= spriteoffset[lump];
    int x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    int x2 = ((centerxfrac + FixedMul(tx, xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vissprite_t *vis = R_NewVisSprite();
    vis->mobjflags = thing->flags;
    vis->scale = xscale << detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = thing->z + spritetopoffset[lump];
    vis->texturemid = vis->gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
    fixed_t iscale = FixedDiv(FRACUNIT, xscale);

    if (flip) {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -iscale;
    } else {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);
    vis->patch = lump;

    // get light level
    if (thing->flags & MF_SHADOW) {
        // shadow draw
        vis->colormap = NULL;
    } else if (fixedcolormap) {
        // fixed map
        vis->colormap = fixedcolormap;
    } else if (thing->frame & FF_FULLBRIGHT) {
        // full bright
        vis->colormap = colormaps;
    } else {
        // diminished light
        int index = xscale >> (LIGHTSCALESHIFT - detailshift);

        if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE - 1;

        vis->colormap = spritelights[index];
    }
}


//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites(sector_t *sec) {
    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;

    // Well, now it will be done.
    sec->validcount = validcount;

    int lightnum = (sec->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // Handle all things in sector.
    for (mobj_t *thing = sec->thinglist; thing; thing = thing->snext)
        R_ProjectSprite(thing);
}


//
// R_DrawPSprite
//
void R_DrawPSprite(pspdef_t *psp) {
    vissprite_t avis;

    // decide which patch to use
#ifdef RANGECHECK
    if ((uint32_t) psp->state->sprite >= (uint32_t) numsprites)
        I_Error("R_ProjectSprite: invalid sprite number %i ",
                psp->state->sprite);
#endif
    spritedef_t *sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ((psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame %i : %i ",
                psp->state->sprite, psp->state->frame);
#endif
    spriteframe_t *sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

    int lump = sprframe->lump[0];
    bool flip = (bool)sprframe->flip[0];

    // calculate edges of the shape
    fixed_t tx = psp->sx - 160 * FRACUNIT;

    tx -= spriteoffset[lump];
    int x1 = (centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS;

    // off the right side
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    int x2 = ((centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vissprite_t *vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER << FRACBITS) + FRACUNIT / 2 - (psp->sy - spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
    vis->scale = pspritescale << detailshift;

    if (flip) {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump] - 1;
    } else {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4 * 32
        || viewplayer->powers[pw_invisibility] & 8) {
        // shadow draw
        vis->colormap = NULL;
    } else if (fixedcolormap) {
        // fixed color
        vis->colormap = fixedcolormap;
    } else if (psp->state->frame & FF_FULLBRIGHT) {
        // full bright
        vis->colormap = colormaps;
    } else {
        // local light
        vis->colormap = spritelights[MAXLIGHTSCALE - 1];
    }

    R_DrawVisSprite(vis, vis->x1, vis->x2);
}


//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites(void) {
    int i;
    pspdef_t *psp;

    // get light level
    int lightnum = (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    // add all active psprites
    for (i = 0, psp = viewplayer->psprites;
         i < NUMPSPRITES;
         i++, psp++) {
        if (psp->state)
            R_DrawPSprite(psp);
    }
}


//
// R_SortVisSprites
//
vissprite_t vsprsortedhead;


void R_SortVisSprites(void) {
    vissprite_t unsorted;

    int count = vissprite_p - vissprites;

    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
        return;

    for (vissprite_t *ds = vissprites; ds < vissprite_p; ds++) {
        ds->next = ds + 1;
        ds->prev = ds - 1;
    }

    vissprites[0].prev = &unsorted;
    unsorted.next = &vissprites[0];
    (vissprite_p - 1)->next = &unsorted;
    unsorted.prev = vissprite_p - 1;

    // pull the vissprites out by scale

    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for (int i = 0; i < count; i++) {
        fixed_t bestscale = INT_MAX;
        vissprite_t *best = unsorted.next;
        for (vissprite_t *ds = unsorted.next; ds != &unsorted; ds = ds->next) {
            if (ds->scale < bestscale) {
                bestscale = ds->scale;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &vsprsortedhead;
        best->prev = vsprsortedhead.prev;
        vsprsortedhead.prev->next = best;
        vsprsortedhead.prev = best;
    }
}


//
// R_DrawSprite
//
static int16_t clipbot[SCREENWIDTH];
static int16_t cliptop[SCREENWIDTH];

void R_DrawSprite(vissprite_t *spr) {
    fixed_t scale;
    fixed_t lowscale;

    for (int x = spr->x1; x <= spr->x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (drawseg_t *ds = ds_p - 1; ds >= drawsegs; ds--) {
        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2
            || ds->x2 < spr->x1
            || (!ds->silhouette
                && !ds->maskedtexturecol)) {
            // does not cover sprite
            continue;
        }

        int r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        int r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2) {
            lowscale = ds->scale2;
            scale = ds->scale1;
        } else {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if (scale < spr->scale
            || (lowscale < spr->scale
                && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline))) {
            // masked mid texture?
            if (ds->maskedtexturecol)
                R_RenderMaskedSegRange(ds, r1, r2);
            // seg is behind sprite
            continue;
        }


        // clip this piece of the sprite
        int silhouette = ds->silhouette;

        if (spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if (spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;

        if (silhouette == 1) {
            // bottom sil
            for (int x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        } else if (silhouette == 2) {
            // top sil
            for (int x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        } else if (silhouette == 3) {
            // both
            for (int x = r1; x <= r2; x++) {
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (int x = spr->x1; x <= spr->x2; x++) {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite(spr, spr->x1, spr->x2);
}


//
// R_DrawMasked
//
void R_DrawMasked(void) {
    R_SortVisSprites();

    if (vissprite_p > vissprites) {
        // draw all vissprites back to front
        for (vissprite_t *spr = vsprsortedhead.next; spr != &vsprsortedhead; spr = spr->next) {
            R_DrawSprite(spr);
        }
    }

    // render any remaining masked mid textures
    for (drawseg_t *ds = ds_p - 1; ds >= drawsegs; ds--)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!viewangleoffset)
        R_DrawPlayerSprites();
}



