// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stddef.h>
#include <stdint.h>

// One Keyboard-page input field from a HID report descriptor.
typedef struct {
    uint8_t report_id;   // 0 when the device does not use report IDs
    bool bitmap;         // one bit per usage (modifiers, NKRO) instead of an array of usage codes
    uint8_t size;        // bits per entry
    uint8_t count;       // number of entries
    uint16_t bit;        // offset of the first entry, after the report ID byte
    uint16_t usage_min;  // usage of the first bitmap bit, or of array value logical_min
    int32_t logical_min; // arrays only
} hid_key_field_t;

#define HID_KEY_FIELDS_MAX 8

typedef struct {
    uint8_t count;
    hid_key_field_t fields[HID_KEY_FIELDS_MAX];
} hid_key_layout_t;

// Find the Keyboard-page input fields in a report descriptor. Returns false if there are none.
bool hid_parse_key_layout(const uint8_t *desc, size_t len, hid_key_layout_t *layout);

// Write the Keyboard-page usages held in an input report to usages (at most max).
// Returns how many were written, or -1 when the report carries no keyboard fields
// or signals rollover; the caller should then keep its previous key state.
int hid_report_usages(const hid_key_layout_t *layout, const uint8_t *report, size_t len,
                      uint8_t *usages, int max);
