// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

// Initialize once before drawing. The backend owns its transfer buffers.
void display_init(int width, int height);

// Draw width * height tightly packed, row-major 8-bit palette indices.
// Palette entries are native uint16_t RGB565 values (red in bits 11..15).
// Consume both inputs before returning: Doom may modify them for the next frame.
void display_draw_frame(const uint8_t *framebuffer, const uint16_t palette[256]);
