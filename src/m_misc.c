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
//      Miscellaneous.
//

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "doomtype.h"

#include "deh_str.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

//
// Create a directory
//

void M_MakeDirectory(char *path) { mkdir(path, 0755); }

// Check if a file exists

bool M_FileExists(char *filename) {
    FILE *fstream = fopen(filename, "r");

    if (fstream != NULL) {
        fclose(fstream);
        return true;
    }
    // If we can't open because the file is a directory, the
    // "file" exists at least!

    return errno == EISDIR;
}

//
// Determine the length of an open file.
//

int32_t M_FileLength(FILE *handle) {
    // save the current position in the file
    int32_t savedpos = ftell(handle);

    // jump to the end and find the length
    fseek(handle, 0, SEEK_END);
    int32_t length = ftell(handle);

    // go back to the old location
    fseek(handle, savedpos, SEEK_SET);

    return length;
}

//
// M_WriteFile
//

bool M_WriteFile(char *name, void *source, int length) {
    FILE *handle = fopen(name, "wb");

    if (handle == NULL)
        return false;

    int count = fwrite(source, 1, length, handle);
    fclose(handle);

    if (count < length)
        return false;

    return true;
}

//
// M_ReadFile
//

int M_ReadFile(char *name, uint8_t **buffer) {
    FILE *handle = fopen(name, "rb");
    if (handle == NULL)
        I_Error("Couldn't read file %s", name);

    // find the size of the file by seeking to the end and
    // reading the current position

    int length = M_FileLength(handle);

    uint8_t *buf = Z_Malloc(length, PU_STATIC, NULL);
    int count = fread(buf, 1, length, handle);
    fclose(handle);

    if (count < length)
        I_Error("Couldn't read file %s", name);

    *buffer = buf;
    return length;
}

// Returns the path to a temporary file of the given name, stored
// on the filesystem mounted by the application.
//
// The returned value must be freed with free() after use.

char *M_TempFile(char *name) { return M_StringJoin(FILES_DIR, DIR_SEPARATOR_S, name, NULL); }

bool M_StrToInt(const char *str, int *result) {
    return sscanf(str, " 0x%x", result) == 1 || sscanf(str, " 0X%x", result) == 1 ||
           sscanf(str, " 0%o", result) == 1 || sscanf(str, " %d", result) == 1;
}

void M_ExtractFileBase(char *path, char *dest) {
    char *src = path + strlen(path) - 1;

    // Back up until a slash or the start.
    while (src != path && *(src - 1) != DIR_SEPARATOR) {
        src--;
    }

    char *filename = src;

    // Copy up to eight characters
    // Note: Vanilla Doom exits with an error if a filename is specified
    // with a base of more than eight characters.  To remove the 8.3
    // filename limit, instead we simply truncate the name.

    int length = 0;
    memset(dest, 0, 8);

    while (*src != '\0' && *src != '.') {
        if (length >= 8) {
            printf("Warning: Truncated '%s' lump name to '%.8s'.\n", filename, dest);
            break;
        }

        dest[length++] = toupper((int)*src++);
    }
}

//---------------------------------------------------------------------------
//
// PROC M_ForceUppercase
//
// Change string to uppercase.
//
//---------------------------------------------------------------------------

void M_ForceUppercase(char *text) {
    for (char *p = text; *p != '\0'; ++p) {
        *p = toupper(*p);
    }
}

//
// M_StrCaseStr
//
// Case-insensitive version of strstr()
//

char *M_StrCaseStr(char *haystack, char *needle) {
    size_t haystack_len = strlen(haystack);
    size_t needle_len = strlen(needle);

    if (haystack_len < needle_len) {
        return NULL;
    }

    size_t len = haystack_len - needle_len;

    for (size_t i = 0; i <= len; ++i) {
        if (!strncasecmp(haystack + i, needle, needle_len)) {
            return haystack + i;
        }
    }

    return NULL;
}

//
// Safe version of strdup() that checks the string was successfully
// allocated.
//

char *M_StringDuplicate(const char *orig) {
    char *result = strdup(orig);

    if (result == NULL) {
        I_Error("Failed to duplicate string (length %i)\n", strlen(orig));
    }

    return result;
}

//
// String replace function.
//

char *M_StringReplace(const char *haystack, const char *needle, const char *replacement) {
    size_t needle_len = strlen(needle);

    // Iterate through occurrences of 'needle' and calculate the size of
    // the new string.
    size_t result_len = strlen(haystack) + 1;
    const char *p = haystack;

    for (;;) {
        p = strstr(p, needle);
        if (p == NULL) {
            break;
        }

        p += needle_len;
        result_len += strlen(replacement) - needle_len;
    }

    // Construct new string.

    char *result = malloc(result_len);
    if (result == NULL) {
        I_Error("M_StringReplace: Failed to allocate new string");
        return NULL;
    }

    char *dst = result;
    size_t dst_len = result_len;
    p = haystack;

    while (*p != '\0') {
        if (!strncmp(p, needle, needle_len)) {
            M_StringCopy(dst, replacement, dst_len);
            p += needle_len;
            dst += strlen(replacement);
            dst_len -= strlen(replacement);
        } else {
            *dst = *p;
            ++dst;
            --dst_len;
            ++p;
        }
    }

    *dst = '\0';

    return result;
}

// Safe string copy function that works like OpenBSD's strlcpy().
// Returns true if the string was not truncated.

bool M_StringCopy(char *dest, const char *src, size_t dest_size) {
    if (dest_size >= 1) {
        dest[dest_size - 1] = '\0';
        strncpy(dest, src, dest_size - 1);
    } else {
        return false;
    }

    size_t len = strlen(dest);
    return src[len] == '\0';
}

// Safe string concat function that works like OpenBSD's strlcat().
// Returns true if string not truncated.

bool M_StringConcat(char *dest, const char *src, size_t dest_size) {
    size_t offset = strlen(dest);
    if (offset > dest_size) {
        offset = dest_size;
    }

    return M_StringCopy(dest + offset, src, dest_size - offset);
}

// Returns true if 's' begins with the specified prefix.

bool M_StringStartsWith(const char *s, const char *prefix) {
    return strlen(s) > strlen(prefix) && strncmp(s, prefix, strlen(prefix)) == 0;
}

// Returns true if 's' ends with the specified suffix.

bool M_StringEndsWith(const char *s, const char *suffix) {
    return strlen(s) >= strlen(suffix) && strcmp(s + strlen(s) - strlen(suffix), suffix) == 0;
}

// Return a newly-malloced string with all the strings given as arguments
// concatenated together.

char *M_StringJoin(const char *s, ...) {
    const char *v;
    va_list args;

    size_t result_len = strlen(s) + 1;

    va_start(args, s);
    for (;;) {
        v = va_arg(args, const char *);
        if (v == NULL) {
            break;
        }

        result_len += strlen(v);
    }
    va_end(args);

    char *result = malloc(result_len);

    if (result == NULL) {
        I_Error("M_StringJoin: Failed to allocate new string.");
        return NULL;
    }

    M_StringCopy(result, s, result_len);

    va_start(args, s);
    for (;;) {
        v = va_arg(args, const char *);
        if (v == NULL) {
            break;
        }

        M_StringConcat(result, v, result_len);
    }
    va_end(args);

    return result;
}

// Safe, portable vsnprintf().
int M_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args) {
    if (buf_len < 1) {
        return 0;
    }

    // Windows (and other OSes?) has a vsnprintf() that doesn't always
    // append a trailing \0. So we must do it, and write into a buffer
    // that is one byte shorter; otherwise this function is unsafe.
    int result = vsnprintf(buf, buf_len, s, args);

    // If truncated, change the final char in the buffer to a \0.
    // A negative result indicates a truncated buffer on Windows.
    if (result < 0 || result >= buf_len) {
        buf[buf_len - 1] = '\0';
        result = buf_len - 1;
    }

    return result;
}

// Safe, portable snprintf().
int M_snprintf(char *buf, size_t buf_len, const char *s, ...) {
    va_list args;
    va_start(args, s);
    int result = M_vsnprintf(buf, buf_len, s, args);
    va_end(args);
    return result;
}
