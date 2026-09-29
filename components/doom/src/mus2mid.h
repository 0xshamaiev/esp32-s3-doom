// TinyS3 port changes: Copyright (C) 2026 Danylo Shamaiev.
// Modified 2026-09-26 through 2026-09-29; see docs/licensing.md.
// These port changes are licensed under GPL-2.0-or-later.
// Original code retains its upstream terms and notices below.

#ifndef MUS2MID_H
#define MUS2MID_H

#include "doomtype.h"
#include "memio.h"

bool mus2mid(MEMFILE *musinput, MEMFILE *midioutput);

#endif /* #ifndef MUS2MID_H */
