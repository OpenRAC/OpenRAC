/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's pads, answered by the port above libpad (4.0) and libmtap.
 *
 * The game keeps eight pad records of 0x5C0 bytes (0x001CD0C0: ports 0 and 1, four multitap
 * slots each). On the console its pad start-up (the boot program's func_003A2FC0) started libmtap
 * and libpad and opened each record's port and slot, and each frame its pad update
 * (func_003A3B40, run on every record by func_003A3F70) stepped a state machine on the record's
 * +0x198 over several frames: find the controller (scePadGetState, scePadInfoMode), put it in
 * analog mode, align the motors, turn the pressures on, then (state 99) read a report into the
 * record's +0x574, hand it to the record's callback (+0x56C) and to the game's reader
 * (func_003A31C8), which turns it into the record's buttons, sticks and pressures, and send the
 * motors (+0x188 small, +0x189 large) with scePadSetActDirect. A record with no controller has
 * its callback and reader run on the game's empty report (0x001D6DF0) instead.
 *
 * frontbin (func_003A2BC0, func_003A3740), each level (Veldin's places func_L01_00460868,
 * func_L01_004613E8) and the multiplayer menus and maps (level 39's func_L39_005C32A0,
 * func_L39_005C3EA0; their update leaves out the active pad's flag) carry their own copies of the
 * start-up and the update, with their own readers.
 *
 * Here the start-up and the update are the port's, for every copy. The start-up sets each record
 * up as the game's does, with no library to start and no multitap. The update keeps the record's
 * state where the game's would have it, but a controller is ready as soon as it is there: state 0
 * for the frame it is found, then 99, the analog mode with pressures (id 0x79) and the motors
 * aligned as the game asks (byte 0 the small motor, byte 1 the large). Each frame a ready
 * record's report is the port's controller (openrac_pad_report, through the mods' pad callbacks)
 * in the place and form the game reads it (+0x574: a status byte, the id, then the report), and
 * the callback and the reader are the game's, so the dead zones, pressures, edges and everything
 * the game computes from the pad stay its own. The motors go to the window as they are.
 *
 * The ports' first slots are the port's controllers 0 and 1; the other slots have no controller
 * (no multitap), and answer as the console's library answered a slot it never opened (99).
 */
#include <stdint.h>
#include <string.h>

#include "game/common/lib/pad_report.h"
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* The boot program's (and frontbin's and the levels': they share its data). */
#define PADS RAC3_ADDR(0x001CD0C0u)       /* the eight pad records */
#define ACTIVE_PAD RAC3_ADDR(0x001D52FCu) /* the record the menus read */
#define PORT1_PAD RAC3_ADDR(0x001D5300u)  /* port 1's first record, twice */
#define PORT1_PAD_TOO RAC3_ADDR(0x001D5304u)
#define NOT_READY RAC3_ADDR(0x001D4CF4u) /* 1 while the active record finds its controller */
#define NO_PAD RAC3_ADDR(0x001D6DF0u)    /* the report read for a record with no controller */

#define PAD_SIZE 0x5C0u
#define PAD_COUNT 8

/* A pad record's fields (rac3-decomp src/boot_elf/text/3A2FC0.c; IDENTIFIED.tsv). */
enum {
    PAD_MOTOR_SMALL = 0x188, /* the game's motors, sent and cleared each frame */
    PAD_MOTOR_LARGE = 0x189,
    PAD_STATE = 0x198,     /* the update's state: 0 find the controller, 99 ready */
    PAD_LIB_STATE = 0x19C, /* scePadGetState's answer: 6 stable, 0 no controller */
    PAD_HELD = 0x1A0,
    PAD_PREV_HELD = 0x1AC,
    PAD_RAW_HELD = 0x1B0,
    PAD_PREV_RAW_HELD = 0x1BC,
    PAD_ID = 0x1DC, /* the last report's id */
    PAD_LATCHED_HELD = 0x348,
    PAD_PORT = 0x54C,
    PAD_MULTITAP = 0x550, /* libmtap's answer for the port: 0, none */
    PAD_ALIGN = 0x551,    /* six bytes: which motor each SetActDirect byte drives */
    PAD_SLOT = 0x557,
    PAD_SEEN = 0x558,    /* the ready state's first frame is past */
    PAD_WANT_ID = 0x55C, /* the id the record was set up for */
    PAD_MODE_ID = 0x560, /* scePadInfoMode's current id, and its extended id */
    PAD_MODE_EXT_ID = 0x564,
    PAD_CALLBACK = 0x56C, /* the game's (record, report, size, argument), or 0 */
    PAD_CALLBACK_ARG = 0x570,
    PAD_REPORT = 0x574, /* 32 bytes: status (0 read), id, then the report */
};

enum {
    STATE_FIND = 0,
    STATE_READY = 99,
    LIB_NONE = 0,
    LIB_STABLE = 6,
    LIB_NOT_OPENED = 99,
    ID_ANALOG = 7,       /* the DualShock 2 in analog mode */
    ID_PRESSURES = 0x79, /* its reports' id with pressures: 9 halfwords of payload */
    REPORT_BYTES = 32,
    REPORT_READ = 2 + 2 * (ID_PRESSURES & 0xF), /* what the reader is handed: 20 bytes */
};

/* One copy of the pad code: its program's records and globals, its reader and its clear. */
typedef struct {
    gaddr pads, active, port1, port1_too, none;
    int active_flag; /* whether its update keeps NOT_READY */
    void (*read)(gaddr pad, gaddr report, int size);
    void (*clear)(gaddr pad);
} pad_copy;

#define W(pad, field) GREF(int32_t, (pad) + (field))
#define B(pad, field) GREF(uint8_t, (pad) + (field))

static int pad_connected(gaddr pad) {
    uint8_t report[OPENRAC_PAD_REPORT];
    return B(pad, PAD_SLOT) == 0 && B(pad, PAD_PORT) < 2
           && openrac_pad_report(B(pad, PAD_PORT), report);
}

/* The record's callback, if it has one, then the reader, on `size` bytes at `report`. */
static void pad_read(const pad_copy* copy, gaddr pad, gaddr report, int size) {
    const gaddr callback = (gaddr)W(pad, PAD_CALLBACK);
    if (callback != 0) {
        const uint64_t ints[8] = {pad, report, (uint64_t)size, (uint32_t)W(pad, PAD_CALLBACK_ARG)};
        const float floats[8] = {0};
        float result;
        (void)openrac_guest_call_regs(callback, ints, floats, &result, 0);
    }
    copy->read(pad, report, size);
}

static void pads_start(const pad_copy* copy) {
    for (unsigned i = 0; i < PAD_COUNT; i++) {
        const gaddr pad = copy->pads + i * PAD_SIZE;
        copy->clear(pad);
        B(pad, PAD_SLOT) = (uint8_t)(i % 4);
        B(pad, PAD_PORT) = (uint8_t)(i >= 4);
        W(pad, PAD_STATE) = STATE_FIND;
        B(pad, PAD_MULTITAP) = 0;
        W(pad, PAD_CALLBACK) = 0;
        W(pad, PAD_CALLBACK_ARG) = 0;
    }
    GREF(gaddr, copy->active) = copy->pads;
    GREF(gaddr, copy->port1) = copy->pads + 4 * PAD_SIZE;
    GREF(gaddr, copy->port1_too) = copy->pads + 4 * PAD_SIZE;
}

static void pad_update(const pad_copy* copy, gaddr pad) {
    const int active = copy->active_flag && pad == GREF(gaddr, copy->active);

    W(pad, PAD_HELD) = 0;
    W(pad, PAD_PREV_HELD) = W(pad, PAD_LATCHED_HELD);
    W(pad, PAD_PREV_RAW_HELD) = W(pad, PAD_RAW_HELD);
    W(pad, PAD_RAW_HELD) = 0;
    if (B(pad, PAD_SLOT) != 0 || B(pad, PAD_PORT) >= 2) {
        W(pad, PAD_LIB_STATE) = LIB_NOT_OPENED;
    } else {
        W(pad, PAD_LIB_STATE) = pad_connected(pad) ? LIB_STABLE : LIB_NONE;
    }
    if (W(pad, PAD_LIB_STATE) == LIB_NONE) {
        W(pad, PAD_STATE) = STATE_FIND;
    }
    if (active) {
        GREF(uint8_t, NOT_READY) = 0;
    }

    if (W(pad, PAD_STATE) != STATE_READY) {
        if (active) {
            GREF(uint8_t, NOT_READY) = 1;
        }
        if (W(pad, PAD_LIB_STATE) != LIB_STABLE) {
            pad_read(copy, pad, copy->none, REPORT_BYTES);
            B(pad, PAD_SEEN) = 0;
            return;
        }
        /* What the game's steps leave once the controller is in analog mode with its pressures
         * on and the motors aligned. */
        W(pad, PAD_MODE_ID) = ID_ANALOG;
        W(pad, PAD_MODE_EXT_ID) = ID_ANALOG;
        W(pad, PAD_WANT_ID) = 0;
        B(pad, PAD_ALIGN + 0) = 0;
        B(pad, PAD_ALIGN + 1) = 1;
        for (unsigned n = 2; n < 6; n++) {
            B(pad, PAD_ALIGN + n) = 0xFF;
        }
        W(pad, PAD_STATE) = STATE_READY;
        return;
    }

    if (B(pad, PAD_SEEN) == 0) {
        B(pad, PAD_MULTITAP) = 0;
    }
    B(pad, PAD_SEEN) = 1;
    if (W(pad, PAD_LIB_STATE) != LIB_STABLE) {
        return;
    }
    uint8_t* report = (uint8_t*)G(pad + PAD_REPORT);
    memset(report, 0, REPORT_BYTES);
    report[1] = ID_PRESSURES;
    openrac_pad_report(B(pad, PAD_PORT), report + 2);
    pad_read(copy, pad, pad + PAD_REPORT, REPORT_READ);
    W(pad, PAD_ID) = ID_PRESSURES;
    if (W(pad, PAD_WANT_ID) != 0 && W(pad, PAD_ID) != W(pad, PAD_WANT_ID)) {
        W(pad, PAD_WANT_ID) = 0;
        W(pad, PAD_STATE) = STATE_FIND;
    } else {
        W(pad, PAD_WANT_ID) = W(pad, PAD_ID);
    }

    int small = 0, large = 0;
    for (unsigned n = 0; n < 6; n++) {
        const uint8_t value = n < 2 ? B(pad, PAD_MOTOR_SMALL + n) : 0;
        if (B(pad, PAD_ALIGN + n) == 0) {
            small = value != 0;
        } else if (B(pad, PAD_ALIGN + n) == 1) {
            large = value;
        }
    }
    openrac_game_set_motors(B(pad, PAD_PORT), small, large);
    B(pad, PAD_MOTOR_SMALL) = 0;
    B(pad, PAD_MOTOR_LARGE) = 0;
}

/* ---- The copies ---- */

void func_003A31C8(gaddr pad, gaddr report, int size);
void func_003A30D8(gaddr pad);
void func_003A2DC8__frontbin(gaddr pad, gaddr report, int size);
void func_003A2CD8__frontbin(gaddr pad);
void func_L01_00460A70(int pad, int report, int size);
void func_L01_00460980(int pad);

static void level_read(gaddr pad, gaddr report, int size) {
    func_L01_00460A70((int)pad, (int)report, size);
}

static void level_clear(gaddr pad) {
    func_L01_00460980((int)pad);
}

static const pad_copy boot = {
    PADS,
    ACTIVE_PAD,
    PORT1_PAD,
    PORT1_PAD_TOO,
    NO_PAD,
    1,
    func_003A31C8,
    func_003A30D8,
};
static const pad_copy frontbin = {
    PADS,
    ACTIVE_PAD,
    PORT1_PAD,
    PORT1_PAD_TOO,
    NO_PAD,
    1,
    func_003A2DC8__frontbin,
    func_003A2CD8__frontbin,
};
static const pad_copy level = {
    PADS,
    ACTIVE_PAD,
    PORT1_PAD,
    PORT1_PAD_TOO,
    NO_PAD,
    1,
    level_read,
    level_clear,
};

void func_003A2FC0(void) { /* the boot program's pad start-up */
    pads_start(&boot);
}

void func_003A3B40(gaddr pad) { /* its pad update */
    pad_update(&boot, pad);
}

void func_003A2BC0__frontbin(void) {
    pads_start(&frontbin);
}

void func_003A3740__frontbin(gaddr pad) {
    pad_update(&frontbin, pad);
}

int func_L01_00460868(void) {
    pads_start(&level);
    return 0;
}

int func_L01_004613E8(int pad) {
    pad_update(&level, (gaddr)pad);
    return 0;
}

/* scePadEnd, which the game calls on its way to another executable (func_003980A0, frontbin's
 * func_00397CC0): nothing is open; the motors stop. */
int func_0012EF10(void) {
    openrac_game_set_motors(0, 0, 0);
    openrac_game_set_motors(1, 0, 0);
    return 1;
}

/* ---- The multiplayer executable's libpad ----
 *
 * The multiplayer menus and maps (level 39's pad update, func_L39_005C3EA0, and each map's copy)
 * still run their libpad state machine; host/mp.c binds the executable's libpad entries to these,
 * which answer as libpad 4.0 does (as Deadlocked's, rac4-ntsc/host/pad.c): configuration completes
 * at once, no multitaps, and SetActAlign maps each SetActDirect byte to motor 0 (small, on/off) or
 * 1 (large, speed). */

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

int rac3_mp_pad_init(int mode) { /* scePadInit */
    (void)mode;
    memset(pads, 0, sizeof pads);
    return 1;
}

int rac3_mp_pad_port_open(int port, int slot, int work) { /* scePadPortOpen */
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

int rac3_mp_pad_read(int port, int slot, int data) { /* scePadRead */
    if (data == 0 || !connected(port, slot)) {
        return 0;
    }
    unsigned char* out = G((gaddr)data);
    memset(out, 0, 32);
    out[1] = pads[port].analog ? (pads[port].pressure ? 0x79 : 0x73) : 0x41;
    openrac_pad_report(port, out + 2);
    if (!pads[port].analog) {
        memset(out + 4, 0, 16);
    } else if (!pads[port].pressure) {
        memset(out + 8, 0, 12);
    }
    return 32;
}

int rac3_mp_pad_get_state(int port, int slot) { /* scePadGetState */
    if (!opened(port, slot)) {
        return 99; /* same unopened-port result as the retail library */
    }
    return connected(port, slot) ? 6 : 0; /* stable / disconnected */
}

int rac3_mp_pad_get_req_state(int port, int slot) { /* scePadGetReqState */
    return connected(port, slot) ? 0 : 1;           /* completed / failed */
}

int rac3_mp_pad_info_act(int port, int slot, int actuator, int term) { /* scePadInfoAct */
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

int rac3_mp_pad_info_mode(int port, int slot, int info, int index) { /* scePadInfoMode */
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

int rac3_mp_pad_set_main_mode(int port, int slot, int mode, int lock) { /* scePadSetMainMode */
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

int rac3_mp_pad_set_act_direct(int port, int slot, int values) { /* scePadSetActDirect */
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

int rac3_mp_pad_set_act_align(int port, int slot, int alignment) { /* scePadSetActAlign */
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

int rac3_mp_pad_info_press_mode(int port, int slot) { /* scePadInfoPressMode */
    return connected(port, slot) && pads[port].analog;
}

int rac3_mp_pad_enter_press_mode(int port, int slot) { /* scePadEnterPressMode */
    return set_button_info(port, slot, 0xFFF);
}
