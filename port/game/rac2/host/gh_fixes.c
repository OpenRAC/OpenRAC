/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Optional Greatest Hits corrections for the two earlier discs. The original
 * functions remain callable when a fix is disabled. See differences.json. */
#include "gh_fixes.h"

#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

static int greatest_hits(void) {
    return strcmp(openrac_game.id, "rac2-ntsc2") == 0;
}

static const unsigned char* reverse_fix;
static rac2_mining_thrust* mining_thrust;

void func_L08_0041EBF8(gaddr moby) {
    if (greatest_hits()) {
        func_L08_0041EBF8__game(moby);
        return;
    }
    if (!reverse_fix) {
        reverse_fix = openrac_game_fix("fix_mining_reverse");
    }
    rac2_mining_thrust phase = {moby + 0xC0u, 0, 0.0f};
    rac2_mining_thrust* previous = mining_thrust;
    mining_thrust = *reverse_fix ? &phase : NULL;
    func_L08_0041EBF8__game(moby);
    mining_thrust = previous;
}

void func_00282F20(gaddr destination, gaddr source, float scale) {
    func_00282F20__game(
        destination, source, rac2_mining_thrust_scale(mining_thrust, destination, source, scale)
    );
}

#define PLANET_INFO_COUNT_WIDGET RAC2_K(PLANET_INFO_COUNT_WIDGET, 0x410, func_0033ED88)
#define PLANET_INFO_COUNT_BUFFER RAC2_K(PLANET_INFO_COUNT_BUFFER, 0x600, func_0033ED88)
#define PLANET_INFO_COUNT_LABEL RAC2_K(PLANET_INFO_COUNT_LABEL, 0x2DBB, func_0033ED88)
#define PLANET_INFO_TEXT_POINTER RAC2_K(PLANET_INFO_TEXT_POINTER, 0x40, func_00336A38)
#define PLANET_INFO_COUNT_TITLE RAC2_K(PLANET_INFO_COUNT_TITLE, 0x4E8, func_0033ED88)

void func_0033ED88(gaddr object) {
    static const unsigned char* counter_fix;
    if (greatest_hits()) {
        func_0033ED88__game(object);
        return;
    }
    if (!counter_fix) {
        counter_fix = openrac_game_fix("fix_planet_platinum_count");
    }
    if (!*counter_fix) {
        func_0033ED88__game(object);
        return;
    }
    const int planet = GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001C4FA0u)) + 0x230u);
    /* GH shows this row for the selectable planets 1..20, not special maps. */
    if (planet < 1 || planet > 20) {
        return;
    }
    const gaddr widget = object + PLANET_INFO_COUNT_WIDGET;
    const gaddr buffer = object + PLANET_INFO_COUNT_BUFFER;
    const gaddr text = GREF(gaddr, widget + PLANET_INFO_TEXT_POINTER);
    const gaddr position = func_00335E10(widget);
    const float x = GREF(float, position), y = GREF(float, position + 4u);
    const int gap = GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001ADF90u)));
    const uint8_t (*flags)[4] = G(RAC2_ADDR(0x0019B278u));

    /* Use the disc's localized label directly instead of copying it into the
     * original 24-byte numeric buffer (some translations can be longer). */
    func_00336A38(widget, func_00288FB8(PLANET_INFO_COUNT_LABEL));
    func_00336A30(widget, 0);
    func_00336A78(widget);
    snprintf(
        G(buffer),
        24,
        "%u/%u",
        rac2_planet_bolt_count(flags, (unsigned)planet),
        rac2_planet_bolt_total((unsigned)planet)
    );
    func_00336A38(widget, buffer);
    func_00335E38(widget, x + (float)gap, y, 0.0f, 0.0f);
    func_00336A30(widget, 2);
    func_00336A78(widget);
    func_00335E38(widget, x, y, 0.0f, 0.0f);
    func_00336A38(widget, text);
    func_00336A30(widget, 0);
    /* The original heading is already the disc's localized "Platinum Bolt". */
    func_00336A78(object + PLANET_INFO_COUNT_TITLE);
}
