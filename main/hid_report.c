// SPDX-FileCopyrightText: 2026 Danylo Shamaiev
// SPDX-License-Identifier: GPL-3.0-or-later

// Just enough of a HID report descriptor parser to find keyboard keys (HID 1.11, 6.2.2).
#include "hid_report.h"

enum { PAGE_KEYBOARD = 0x07, USAGE_ROLLOVER = 0x01, USAGE_FIRST_KEY = 0x04 };

bool hid_parse_key_layout(const uint8_t *desc, size_t len, hid_key_layout_t *layout) {
    *layout = (hid_key_layout_t){0};
    uint16_t offsets[256] = {0}; // input bits seen so far, per report ID
    uint16_t usage_page = 0;
    uint8_t report_id = 0;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    int32_t logical_min = 0;
    // Local items, cleared after every main item.
    uint16_t local_page = 0;
    uint16_t usage_min = 0;
    bool have_usage = false;

    for (size_t i = 0; i < len;) {
        const uint8_t prefix = desc[i++];
        if (prefix == 0xFE) { // long item: data size, tag, data
            if (i >= len)
                break;
            i += 2 + desc[i];
            continue;
        }
        const uint8_t size = (prefix & 3) == 3 ? 4 : prefix & 3;
        if (i + size > len)
            break;
        uint32_t value = 0;
        for (int b = 0; b < size; b++)
            value |= (uint32_t)desc[i + b] << (8 * b);
        i += size;
        const int32_t signed_value = size == 1   ? (int8_t)value
                                     : size == 2 ? (int16_t)value
                                                 : (int32_t)value;
        const uint8_t type = (prefix >> 2) & 3;
        const uint8_t tag = prefix >> 4;

        if (type == 1) { // global
            switch (tag) {
            case 0x0: usage_page = value; break;
            case 0x1: logical_min = signed_value; break;
            case 0x7: report_size = value; break;
            case 0x8: report_id = value; break;
            case 0x9: report_count = value; break;
            default: break;
            }
        } else if (type == 2) { // local
            // Usage (0) or Usage Minimum (1). A 4-byte value carries its own usage page.
            // The first Usage stands in for a minimum: listed usages are consecutive in practice.
            if (tag == 0x1 || (tag == 0x0 && !have_usage)) {
                usage_min = value & 0xFFFF;
                if (size == 4)
                    local_page = value >> 16;
                have_usage = true;
            }
        } else if (type == 0) { // main
            if (tag == 0x8) { // Input
                const uint16_t page = local_page ? local_page : usage_page;
                const bool constant = value & 1;
                if (!constant && page == PAGE_KEYBOARD && report_size >= 1 && report_size <= 16 &&
                    report_count >= 1 && report_count <= 255 && layout->count < HID_KEY_FIELDS_MAX) {
                    layout->fields[layout->count++] = (hid_key_field_t){
                        .report_id = report_id,
                        .bitmap = value & 2,
                        .size = report_size,
                        .count = report_count,
                        .bit = offsets[report_id],
                        .usage_min = usage_min,
                        .logical_min = logical_min,
                    };
                }
                offsets[report_id] += report_size * report_count;
            }
            local_page = 0;
            usage_min = 0;
            have_usage = false;
        }
    }
    return layout->count > 0;
}

static uint32_t read_bits(const uint8_t *data, uint32_t bit, uint8_t size) {
    uint32_t value = 0;
    for (uint8_t i = 0; i < size; i++, bit++)
        value |= (uint32_t)((data[bit / 8] >> (bit % 8)) & 1) << i;
    return value;
}

int hid_report_usages(const hid_key_layout_t *layout, const uint8_t *report, size_t len,
                      uint8_t *usages, int max) {
    uint8_t id = 0;
    if (layout->fields[0].report_id) { // IDs are nonzero, and either every report has one or none do
        if (len == 0)
            return -1;
        id = *report++;
        len--;
    }

    int count = 0;
    bool matched = false;
    for (int f = 0; f < layout->count; f++) {
        const hid_key_field_t *field = &layout->fields[f];
        if (field->report_id != id)
            continue;
        matched = true;
        for (uint32_t i = 0; i < field->count; i++) {
            const uint32_t bit = field->bit + i * field->size;
            if (bit + field->size > len * 8)
                break;
            const uint32_t value = read_bits(report, bit, field->size);
            uint32_t usage;
            if (field->bitmap) {
                if (value == 0)
                    continue;
                usage = field->usage_min + i;
            } else {
                if ((int32_t)value < field->logical_min)
                    continue;
                usage = field->usage_min + (uint32_t)((int32_t)value - field->logical_min);
                // Too many keys held: every slot says ErrorRollOver instead of a key.
                if (usage == USAGE_ROLLOVER)
                    return -1;
            }
            if (usage >= USAGE_FIRST_KEY && usage <= 0xFF && count < max)
                usages[count++] = usage;
        }
    }
    return matched ? count : -1;
}
