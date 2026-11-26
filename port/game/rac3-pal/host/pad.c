/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's libpad API (libpad 4.0, as Deadlocked's: rac4-ntsc/host/pad.c), backed by the
 * host's controller ports. The game's pad state machine (func_003A3B40) and its reader
 * (func_003A31C8) drive it.
 * Configuration completes synchronously. Multitaps are absent. Motors: SetActAlign maps each
 * SetActDirect byte to motor 0 (small, on/off) or 1 (large, speed).
 */
#include <stdint.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"

/* mp_ui.c: the first pad button word (active low) with the presses the port takes left out. */
uint16_t rac3_mp_ui_pad_filter(uint16_t word);

static struct {
    unsigned char open, analog, pressure;
    unsigned char align[6]; /* each SetActDirect byte's motor, 0xFF none */
} pads[2];

enum {
    ACT_SMALL = 0,
    ACT_LARGE = 1,
    ACT_COUNT = 2,
    ACT_NONE = 0xFF
};

static int opened(int port, int slot) {
    return port >= 0 && port < 2 && slot == 0 && pads[port].open;
}

static int connected(int port, int slot) {
    uint16_t buttons;
    uint8_t analog[4];
    return opened(port, slot) && openrac_game_pad(port, &buttons, analog);
}

int func_0012EC90(int mode) { /* scePadInit */
    (void)mode;
    memset(pads, 0, sizeof pads);
    return 1;
}

int func_0012EF10(void) { /* scePadEnd */
    memset(pads, 0, sizeof pads);
    return 1;
}

int func_0012F0B8(int port, int slot, int work) { /* scePadPortOpen */
    (void)work; /* DMA work storage belongs to the console's driver. */
    if (port < 0 || port >= 2 || slot != 0) {
        return 0;
    }
    pads[port].open = 1;
    pads[port].analog = 0;
    pads[port].pressure = 0;
    memset(pads[port].align, ACT_NONE, sizeof pads[port].align);
    return 1;
}

int func_0012F3F8(int port, int slot, int data) { /* scePadRead */
    if (data == 0 || !connected(port, slot)) {
        return 0;
    }
    unsigned char* out = G((gaddr)data);
    memset(out, 0, 32);
    out[1] = pads[port].analog ? (pads[port].pressure ? 0x79 : 0x73) : 0x41;
    openrac_lib_scePad2Read(port, (gaddr)data + 2u);
    if (port == 0) {
        /* Up Your Arsenal's staging screen: a READY the port takes (mp_ui.c). The word is active low. */
        uint16_t word;
        memcpy(&word, out + 2, sizeof word);
        word = rac3_mp_ui_pad_filter(word);
        memcpy(out + 2, &word, sizeof word);
    }
    if (!pads[port].analog) {
        memset(out + 4, 0, 16);
    } else if (!pads[port].pressure) {
        memset(out + 8, 0, 12);
    }
    return 32;
}

int func_0012F470(int port, int slot) { /* scePadGetState */
    if (!opened(port, slot)) {
        return 99; /* same unopened-port result as the retail library */
    }
    return connected(port, slot) ? 6 : 0; /* stable / disconnected */
}

int func_0012F5A8(int port, int slot) {   /* scePadGetReqState */
    return connected(port, slot) ? 0 : 1; /* completed / failed */
}

int func_0012F608(int port, int slot, int actuator, int term) { /* scePadInfoAct */
    if (!connected(port, slot) || !pads[port].analog) {
        return 0;
    }
    if (actuator == -1) {
        return ACT_COUNT; /* how many motors */
    }
    /* Term details are not known; an existing motor answers 1. */
    (void)term;
    return actuator >= 0 && actuator < ACT_COUNT;
}

int func_0012F728(int port, int slot, int info, int index) { /* scePadInfoMode */
    if (!connected(port, slot)) {
        return 0;
    }
    switch (info) {
        case 1: /* current id */
        case 2: /* current extended id */
            return pads[port].analog ? 7 : 4;
        case 3: /* current mode-table index */
            return pads[port].analog ? 1 : 0;
        case 4: /* mode count, or entry */
            return index == -1 ? 2 : index == 0 ? 4 : index == 1 ? 7 : 0;
        default:
            return 0;
    }
}

int func_0012F860(int port, int slot, int mode, int lock) { /* scePadSetMainMode */
    (void)lock;
    if (!connected(port, slot) || (mode != 0 && mode != 1)) {
        return 0;
    }
    pads[port].analog = (unsigned char)mode;
    if (!mode) {
        pads[port].pressure = 0;
    }
    return 1;
}

int func_0012F918(int port, int slot, int values) { /* scePadSetActDirect */
    if (values == 0 || !connected(port, slot)) {
        return 0;
    }
    const unsigned char* data = G((gaddr)values);
    int small = 0, large = 0;
    for (int i = 0; i < 6; i++) {
        if (pads[port].align[i] == ACT_SMALL) {
            small = data[i] != 0;
        } else if (pads[port].align[i] == ACT_LARGE) {
            large = data[i];
        }
    }
    openrac_game_set_motors(port, small, large);
    return 1;
}

int func_0012F9E0(int port, int slot, int alignment) { /* scePadSetActAlign */
    if (alignment == 0 || !connected(port, slot)) {
        return 0;
    }
    memcpy(pads[port].align, G((gaddr)alignment), sizeof pads[port].align);
    return 1;
}

/* scePadSetButtonInfo: the game reaches it only through scePadEnterPressMode. */
static int set_button_info(int port, int slot, int mask) {
    if (!connected(port, slot) || !pads[port].analog) {
        return 0;
    }
    pads[port].pressure = mask != 0;
    return 1;
}

int func_0012FC18(int port, int slot) { /* scePadInfoPressMode */
    return connected(port, slot) && pads[port].analog;
}

int func_0012FC78(int port, int slot) { /* scePadEnterPressMode */
    return set_button_info(port, slot, 0xFFF);
}
