/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Where an effect texture's pixels and CLUT are, for the TEX0 GetEffectTex gave it: its upload records
 * this frame (`count` of them at `records`; +0 the CLUT's address, +6 its block, +8 the pixels'
 * address, +0xE their block), which the game's texture paging turns into GS uploads; a TEX0 seen before
 * keeps the sources it had then (the game caches the word and records the upload only when it pages
 * the texture in). 1 when found in the records, 2 when remembered, 0 when neither.
 */
#pragma once

#include <stdint.h>

#include "openrac/guest.h"

static inline uint32_t openrac_effect_texture(uint64_t tex0, int count, gaddr records, uint32_t* pixels,
                                              uint32_t* clut) {
    static struct {
        uint64_t tex0;
        uint32_t pixels, clut;
    } known[256];
    static int next = 0;
    const uint32_t tbp = (uint32_t)(tex0 & 0x3FFF);
    const uint32_t cbp = (uint32_t)((tex0 >> 37) & 0x3FFF);
    for (int i = 0; i < count && i < 64; ++i) {
        const uint8_t* r = (const uint8_t*)G(records + (gaddr)i * 16);
        if (*(const uint16_t*)(r + 0xE) == tbp && *(const uint16_t*)(r + 6) == cbp) {
            *clut = *(const uint32_t*)(r + 0);
            *pixels = *(const uint32_t*)(r + 8);
            for (int k = 0; k < 256; ++k) {
                if (known[k].tex0 == tex0) {
                    known[k].pixels = *pixels;
                    known[k].clut = *clut;
                    return 1;
                }
            }
            known[next].tex0 = tex0;
            known[next].pixels = *pixels;
            known[next].clut = *clut;
            next = (next + 1) & 255;
            return 1;
        }
    }
    for (int k = 0; k < 256; ++k) {
        if (known[k].tex0 == tex0 && tex0 != 0) {
            *pixels = known[k].pixels;
            *clut = known[k].clut;
            return 2;
        }
    }
    return 0;
}
