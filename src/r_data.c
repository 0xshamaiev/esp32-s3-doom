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
//	Preparation of data for rendering,
//	generation of lookups, caching, retrieval by name.
//

#include <stdio.h>

#include "deh_main.h"
#include "i_swap.h"
#include "i_system.h"
#include "z_zone.h"


#include "w_wad.h"

#include "doomdef.h"
#include "m_misc.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"


#include "r_data.h"

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
// 


//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct {
    short originx;
    short originy;
    short patch;
    short stepdir;
    short colormap;
}

PACKEDATTR mappatch_t;


//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct {
    char name[8];
    int masked;
    short width;
    short height;
    int obsolete;
    short patchcount;
    mappatch_t patches[1];
}

PACKEDATTR maptexture_t;


// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct {
    // Block origin (allways UL),
    // which has allready accounted
    // for the internal origin of the patch.
    short originx;
    short originy;
    int patch;
} texpatch_t;


// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.

typedef struct texture_s texture_t;

struct texture_s {
    // Keep name for switch changing, etc.
    char name[8];
    short width;
    short height;

    // Index in textures list

    int index;

    // Next in hash table chain

    texture_t *next;

    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    short patchcount;
    texpatch_t patches[1];
};


int firstflat;
int lastflat;
int numflats;

int firstpatch;
int lastpatch;
int numpatches;

int firstspritelump;
int lastspritelump;
int numspritelumps;

int numtextures;
texture_t **textures;
texture_t **textures_hashtable;


int *texturewidthmask;
// needed for texture pegging
fixed_t *textureheight;
int *texturecompositesize;
short **texturecolumnlump;
unsigned short **texturecolumnofs;
byte **texturecomposite;

// for global animation
int *flattranslation;
int *texturetranslation;

// needed for pre rendering
fixed_t *spritewidth;
fixed_t *spriteoffset;
fixed_t *spritetopoffset;

lighttable_t *colormaps;


//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//


//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
void
R_DrawColumnInCache
(column_t *patch,
 byte *cache,
 int originy,
 int cacheheight) {
    while (patch->topdelta != 0xff) {
        byte *source = (byte *)patch + 3;
        int count = patch->length;
        int position = originy + patch->topdelta;

        if (position < 0) {
            count += position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
            memcpy(cache + position, source, count);

        patch = (column_t *) ((byte *) patch + patch->length + 4);
    }
}


//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
void R_GenerateComposite(int texnum) {
    int x;
    int i;

    texture_t *texture = textures[texnum];

    byte *block = Z_Malloc(texturecompositesize[texnum], PU_STATIC, &texturecomposite[texnum]);

    short *collump = texturecolumnlump[texnum];
    unsigned short *colofs = texturecolumnofs[texnum];

    // Composite the columns together.
    texpatch_t *patch = texture->patches;

    for (i = 0, patch = texture->patches;
         i < texture->patchcount;
         i++, patch++) {
        patch_t *realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        int x1 = patch->originx;
        int x2 = x1 + SHORT(realpatch->width);

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;

        for (; x < x2; x++) {
            // Column does not have multiple patches?
            if (collump[x] >= 0)
                continue;

            column_t *patchcol =
                (column_t *)((byte *)realpatch + LONG(realpatch->columnofs[x - x1]));
            R_DrawColumnInCache(patchcol,
                                block + colofs[x],
                                patch->originy,
                                texture->height);
        }
    }

    // Now that the texture has been built in column cache,
    //  it is purgable from zone memory.
    Z_ChangeTag(block, PU_CACHE);
}


//
// R_GenerateLookup
//
void R_GenerateLookup(int texnum) {
    byte *patchcount; // patchcount[texture->width]
    int x;
    int i;

    texture_t *texture = textures[texnum];

    // Composited texture not created yet.
    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    short *collump = texturecolumnlump[texnum];
    unsigned short *colofs = texturecolumnofs[texnum];

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    patchcount = (byte *) Z_Malloc(texture->width, PU_STATIC, &patchcount);
    memset(patchcount, 0, texture->width);
    texpatch_t *patch = texture->patches;

    for (i = 0, patch = texture->patches;
         i < texture->patchcount;
         i++, patch++) {
        patch_t *realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        int x1 = patch->originx;
        int x2 = x1 + SHORT(realpatch->width);

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;
        for (; x < x2; x++) {
            patchcount[x]++;
            collump[x] = patch->patch;
            colofs[x] = LONG(realpatch->columnofs[x - x1]) + 3;
        }
    }

    for (x = 0; x < texture->width; x++) {
        if (!patchcount[x]) {
            printf("R_GenerateLookup: column without a patch (%s)\n",
                   texture->name);
            return;
        }
        // I_Error ("R_GenerateLookup: column without a patch");

        if (patchcount[x] > 1) {
            // Use the cached block.
            collump[x] = -1;
            colofs[x] = texturecompositesize[texnum];

            if (texturecompositesize[texnum] > 0x10000 - texture->height) {
                I_Error("R_GenerateLookup: texture %i is >64k",
                        texnum);
            }

            texturecompositesize[texnum] += texture->height;
        }
    }

    Z_Free(patchcount);
}


//
// R_GetColumn
//
byte *
R_GetColumn
(int tex,
 int col) {
    col &= texturewidthmask[tex];
    int lump = texturecolumnlump[tex][col];
    int ofs = texturecolumnofs[tex][col];

    if (lump > 0)
        return (byte *) W_CacheLumpNum(lump, PU_CACHE) + ofs;

    if (!texturecomposite[tex])
        R_GenerateComposite(tex);

    return texturecomposite[tex] + ofs;
}


static void GenerateTextureHashTable(void) {
    textures_hashtable = Z_Malloc(sizeof(texture_t *) * numtextures, PU_STATIC, 0);

    memset(textures_hashtable, 0, sizeof(texture_t *) * numtextures);

    // Add all textures to hash table

    for (int i = 0; i < numtextures; ++i) {
        // Store index

        textures[i]->index = i;

        // Vanilla Doom does a linear search of the texures array
        // and stops at the first entry it finds.  If there are two
        // entries with the same name, the first one in the array
        // wins. The new entry must therefore be added at the end
        // of the hash chain, so that earlier entries win.

        int key = W_LumpNameHash(textures[i]->name) % numtextures;

        texture_t **rover = &textures_hashtable[key];

        while (*rover != NULL) {
            rover = &(*rover)->next;
        }

        // Hook into hash table

        textures[i]->next = NULL;
        *rover = textures[i];
    }
}


//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures(void) {
    int j;

    int *maptex2;
    int *maptex1;

    char name[9];

    int *patchlookup;

    int maxoff2;
    int numtextures2;

    // Load the patch names from pnames.lmp.
    name[8] = 0;
    char *names = W_CacheLumpName(DEH_String("PNAMES"), PU_STATIC);
    int nummappatches = LONG(*((int *)names));
    char *name_p = names + 4;
    patchlookup = Z_Malloc(nummappatches * sizeof(*patchlookup), PU_STATIC, NULL);

    for (int i = 0; i < nummappatches; i++) {
        M_StringCopy(name, name_p + i * 8, sizeof(name));
        patchlookup[i] = W_CheckNumForName(name);
    }
    W_ReleaseLumpName(DEH_String("PNAMES"));

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    int *maptex = maptex1 = W_CacheLumpName(DEH_String("TEXTURE1"), PU_STATIC);
    int numtextures1 = LONG(*maptex);
    int maxoff = W_LumpLength(W_GetNumForName(DEH_String("TEXTURE1")));
    int *directory = maptex + 1;

    if (W_CheckNumForName(DEH_String("TEXTURE2")) != -1) {
        maptex2 = W_CacheLumpName(DEH_String("TEXTURE2"), PU_STATIC);
        numtextures2 = LONG(*maptex2);
        maxoff2 = W_LumpLength(W_GetNumForName(DEH_String("TEXTURE2")));
    } else {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = Z_Malloc(numtextures * sizeof(*textures), PU_STATIC, 0);
    texturecolumnlump = Z_Malloc(numtextures * sizeof(*texturecolumnlump), PU_STATIC, 0);
    texturecolumnofs = Z_Malloc(numtextures * sizeof(*texturecolumnofs), PU_STATIC, 0);
    texturecomposite = Z_Malloc(numtextures * sizeof(*texturecomposite), PU_STATIC, 0);
    texturecompositesize = Z_Malloc(numtextures * sizeof(*texturecompositesize), PU_STATIC, 0);
    texturewidthmask = Z_Malloc(numtextures * sizeof(*texturewidthmask), PU_STATIC, 0);
    textureheight = Z_Malloc(numtextures * sizeof(*textureheight), PU_STATIC, 0);

    int totalwidth = 0;

    //	Really complex printing shit...
    int temp1 = W_GetNumForName(DEH_String("S_START")); // P_???????
    int temp2 = W_GetNumForName(DEH_String("S_END")) - 1;
    int temp3 = ((temp2 - temp1 + 63) / 64) + ((numtextures + 63) / 64);

    // If stdout is a real console, use the classic vanilla "filling
    // up the box" effect, which uses backspace to "step back" inside
    // the box.  If stdout is a file, don't draw the box.

    if (I_ConsoleStdout()) {
        printf("[");
        for (int i = 0; i < temp3 + 9; i++)
            printf(" ");
        printf("]");
        for (int i = 0; i < temp3 + 10; i++)
            printf("\b");
    }

    for (int i = 0; i < numtextures; i++, directory++) {
        if (!(i & 63))
            printf(".");

        if (i == numtextures1) {
            // Start looking in second texture file.
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex + 1;
        }

        int offset = LONG(*directory);

        if (offset > maxoff)
            I_Error("R_InitTextures: bad texture directory");

        maptexture_t *mtexture = (maptexture_t *)((byte *)maptex + offset);

        texture_t *texture = textures[i] =
            Z_Malloc(sizeof(texture_t) + sizeof(texpatch_t) * (SHORT(mtexture->patchcount) - 1),
                     PU_STATIC, 0);

        texture->width = SHORT(mtexture->width);
        texture->height = SHORT(mtexture->height);
        texture->patchcount = SHORT(mtexture->patchcount);

        memcpy(texture->name, mtexture->name, sizeof(texture->name));
        mappatch_t *mpatch = &mtexture->patches[0];
        texpatch_t *patch = &texture->patches[0];

        for (j = 0; j < texture->patchcount; j++, mpatch++, patch++) {
            patch->originx = SHORT(mpatch->originx);
            patch->originy = SHORT(mpatch->originy);
            patch->patch = patchlookup[SHORT(mpatch->patch)];
            if (patch->patch == -1) {
                I_Error("R_InitTextures: Missing patch in texture %s",
                        texture->name);
            }
        }
        texturecolumnlump[i] = Z_Malloc(texture->width * sizeof(**texturecolumnlump), PU_STATIC, 0);
        texturecolumnofs[i] = Z_Malloc(texture->width * sizeof(**texturecolumnofs), PU_STATIC, 0);

        j = 1;
        while (j * 2 <= texture->width)
            j <<= 1;

        texturewidthmask[i] = j - 1;
        textureheight[i] = texture->height << FRACBITS;

        totalwidth += texture->width;
    }

    Z_Free(patchlookup);

    W_ReleaseLumpName(DEH_String("TEXTURE1"));
    if (maptex2)
        W_ReleaseLumpName(DEH_String("TEXTURE2"));

    // Precalculate whatever possible.

    for (int i = 0; i < numtextures; i++)
        R_GenerateLookup(i);

    // Create translation table for global animation.
    texturetranslation = Z_Malloc((numtextures + 1) * sizeof(*texturetranslation), PU_STATIC, 0);

    for (int i = 0; i < numtextures; i++)
        texturetranslation[i] = i;

    GenerateTextureHashTable();
}


//
// R_InitFlats
//
void R_InitFlats(void) {
    firstflat = W_GetNumForName(DEH_String("F_START")) + 1;
    lastflat = W_GetNumForName(DEH_String("F_END")) - 1;
    numflats = lastflat - firstflat + 1;

    // Create translation table for global animation.
    flattranslation = Z_Malloc((numflats + 1) * sizeof(*flattranslation), PU_STATIC, 0);

    for (int i = 0; i < numflats; i++)
        flattranslation[i] = i;
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps(void) {
    firstspritelump = W_GetNumForName(DEH_String("S_START")) + 1;
    lastspritelump = W_GetNumForName(DEH_String("S_END")) - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_Malloc(numspritelumps * sizeof(*spritewidth), PU_STATIC, 0);
    spriteoffset = Z_Malloc(numspritelumps * sizeof(*spriteoffset), PU_STATIC, 0);
    spritetopoffset = Z_Malloc(numspritelumps * sizeof(*spritetopoffset), PU_STATIC, 0);

    for (int i = 0; i < numspritelumps; i++) {
        if (!(i & 63))
            printf(".");

        patch_t *patch = W_CacheLumpNum(firstspritelump + i, PU_CACHE);
        spritewidth[i] = SHORT(patch->width) << FRACBITS;
        spriteoffset[i] = SHORT(patch->leftoffset) << FRACBITS;
        spritetopoffset[i] = SHORT(patch->topoffset) << FRACBITS;
    }
}


//
// R_InitColormaps
//
void R_InitColormaps(void) {
    // Load in the light tables,
    //  256 byte align tables.
    int lump = W_GetNumForName(DEH_String("COLORMAP"));
    colormaps = W_CacheLumpNum(lump, PU_STATIC);
}


//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData(void) {
    R_InitTextures();
    printf(".");
    R_InitFlats();
    printf(".");
    R_InitSpriteLumps();
    printf(".");
    R_InitColormaps();
}


//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName(char *name) {
    char namet[9];

    int i = W_CheckNumForName(name);

    if (i == -1) {
        namet[8] = 0;
        memcpy(namet, name, 8);
        I_Error("R_FlatNumForName: %s not found", namet);
    }
    return i - firstflat;
}


//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName(char *name) {
    // "NoTexture" marker.
    if (name[0] == '-')
        return 0;

    int key = W_LumpNameHash(name) % numtextures;

    texture_t *texture = textures_hashtable[key];

    while (texture != NULL) {
        if (!strncasecmp(texture->name, name, 8))
            return texture->index;

        texture = texture->next;
    }

    return -1;
}


//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int R_TextureNumForName(char *name) {
    int i = R_CheckTextureNumForName(name);

    if (i == -1) {
        I_Error("R_TextureNumForName: %s not found",
                name);
    }
    return i;
}


//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
int flatmemory;
int texturememory;
int spritememory;

void R_PrecacheLevel(void) {
    int lump;

    if (demoplayback)
        return;

    // Precache flats.
    char *flatpresent = Z_Malloc(numflats, PU_STATIC, NULL);
    memset(flatpresent, 0, numflats);

    for (int i = 0; i < numsectors; i++) {
        flatpresent[sectors[i].floorpic] = 1;
        flatpresent[sectors[i].ceilingpic] = 1;
    }

    flatmemory = 0;

    for (int i = 0; i < numflats; i++) {
        if (flatpresent[i]) {
            lump = firstflat + i;
            flatmemory += lumpinfo[lump].size;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    Z_Free(flatpresent);

    // Precache textures.
    char *texturepresent = Z_Malloc(numtextures, PU_STATIC, NULL);
    memset(texturepresent, 0, numtextures);

    for (int i = 0; i < numsides; i++) {
        texturepresent[sides[i].toptexture] = 1;
        texturepresent[sides[i].midtexture] = 1;
        texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[skytexture] = 1;

    texturememory = 0;
    for (int i = 0; i < numtextures; i++) {
        if (!texturepresent[i])
            continue;

        texture_t *texture = textures[i];

        for (int j = 0; j < texture->patchcount; j++) {
            lump = texture->patches[j].patch;
            texturememory += lumpinfo[lump].size;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    Z_Free(texturepresent);

    // Precache sprites.
    char *spritepresent = Z_Malloc(numsprites, PU_STATIC, NULL);
    memset(spritepresent, 0, numsprites);

    for (thinker_t *th = thinkercap.next; th != &thinkercap; th = th->next) {
        if (th->function.acp1 == (actionf_p1) P_MobjThinker)
            spritepresent[((mobj_t *) th)->sprite] = 1;
    }

    spritememory = 0;
    for (int i = 0; i < numsprites; i++) {
        if (!spritepresent[i])
            continue;

        for (int j = 0; j < sprites[i].numframes; j++) {
            spriteframe_t *sf = &sprites[i].spriteframes[j];
            for (int k = 0; k < 8; k++) {
                lump = firstspritelump + sf->lump[k];
                spritememory += lumpinfo[lump].size;
                W_CacheLumpNum(lump, PU_CACHE);
            }
        }
    }

    Z_Free(spritepresent);
}




