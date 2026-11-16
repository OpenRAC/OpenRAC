/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef OPENRAC_VENDOR_TITLE_H
#define OPENRAC_VENDOR_TITLE_H
#include <stdint.h>

/* Match FontPrint's visible quads, including palette controls and accents. */
static inline float openrac_vendor_title_scale(
    const uint8_t* text, int limit, const int8_t* metrics
) {
    int pen = 0, right = 0, count = 0;
    while (*text && count != limit) {
        unsigned c = *text++;
        ++count;
        if (c >= 8 && c < 16) {
            continue;
        }
        int advance = metrics[c * 4 + 3];
        if (!advance) {
            continue;
        }
        if (c >= 0x80 && c < 0xA8) {
            int edge = pen + metrics[(c + 0x40) * 4 + 3] + 16;
            if (edge > right) {
                right = edge;
            }
        }
        int edge = pen + (c < 0x20 ? 24 : c > 0x20 ? 16 : 0);
        if (edge > right) {
            right = edge;
        }
        pen += advance;
    }
    return right > 116 ? 116.0f / right : 1.0f;
}
#endif
