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
//	WAD I/O functions.
//

#include <stdio.h>

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"

// ESP-IDF exposes the application's mounted filesystem through stdio.
wad_file_t *W_OpenFile(char *path) {
    FILE *stream = fopen(path, "rb");
    if (stream == NULL)
        return NULL;

    long length = M_FileLength(stream);
    if (length < 0) {
        fclose(stream);
        return NULL;
    }

    wad_file_t *wad = Z_Malloc(sizeof(*wad), PU_STATIC, NULL);
    wad->stream = stream;
    wad->length = (unsigned int)length;
    return wad;
}

void W_CloseFile(wad_file_t *wad) {
    fclose(wad->stream);
    Z_Free(wad);
}

size_t W_Read(wad_file_t *wad, unsigned int offset, void *buffer, size_t buffer_len) {
    if (fseek(wad->stream, offset, SEEK_SET) != 0)
        return 0;
    return fread(buffer, 1, buffer_len, wad->stream);
}
