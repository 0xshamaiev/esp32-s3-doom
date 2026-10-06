// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

// Start the input backend before starting Doom.
void input_init(void);

// Pop one queued key event without blocking. Keys are Doom codes from doomkeys.h.
// Returns false when no event is pending.
bool input_poll(int *pressed, uint8_t *key);
