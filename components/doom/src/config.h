// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later
// TinyS3 configuration rewritten 2026-09-26; licensing notice added 2026-09-29.

#ifndef DOOM_CONFIG_H
#define DOOM_CONFIG_H

#define PACKAGE "Doom"
#define PACKAGE_NAME "TinyS3 Doom"
#define PACKAGE_STRING "TinyS3 Doom 0.1"
#define PACKAGE_VERSION "0.1"
#define PROGRAM_PREFIX "doom"
#define VERSION "0.1"

// Override for the filesystem mounted by the application.
#ifndef FILES_DIR
#define FILES_DIR "."
#endif

#endif
