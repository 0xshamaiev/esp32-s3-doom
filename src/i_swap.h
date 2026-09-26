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
//	Endianess handling, swapping 16bit and 32bit.
//

#ifndef __I_SWAP__
#define __I_SWAP__

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "The ESP32-S3 engine requires a little-endian target"
#endif

#define SYS_LITTLE_ENDIAN
#define SHORT(x) ((signed short)(x))
#define LONG(x) ((signed int)(x))
#define doom_wtohs(x) ((short int)(x))

#endif
