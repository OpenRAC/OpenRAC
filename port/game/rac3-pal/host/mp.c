/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's multiplayer executable (I5BOOTN.ELF): its start, and the host side of the
 * library rows of mp/libraries.tsv that bind to this game's own host C.
 *
 * I5BOOTN.ELF is the boot executable's loader again (rac3-decomp's src/i5bootn) carrying another
 * game: an engine core at 0x00100000-0x00241A00 whose libraries are the boot program's SDK build at
 * other addresses, the 2D layer and front end the boot program has at 0x0038xxxx at 0x0027B100,
 * and the multiplayer levels' programs loaded at 0x00241A00 (level 39, the multiplayer menus, and
 * the maps 40 to 55). hostgen translates it from the disc's own instructions (mp/hostgen.json,
 * every C name with mp_ before it), and exec.c starts it as the console's LoadExecPS2 does.
 *
 * Its crt0 (0x00192D18, hand-written) sets $gp to 0x00247FC0, the stack below 0x01FFC000 and the
 * heap end at 0x01F48000 by system call, then: _InitSys (0x0012CFD0), FlushCache(0) (0x001264E0),
 * _init (0x001330F0, empty), atexit (0x00118ED0) of the finaliser, main (0x001932A0) with the
 * argument block at 0x001EBE00, and exit. main reads its arguments (BOPT=, the boot program's
 * options; gooey) and runs the loaded program's entry (0x0027B1C0 first) and then the next
 * program's (0x00192F68 copies it over the last: a level's), for ever. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exec.h"
#include "game/common/lib/guest_format.h"
#include "game/common/lib/wad_routine.h"
#include "mp/game_protos.h"
#include "openrac/game_host.h"
#include "openrac/game_lib.h"
#include "openrac/guest.h"
#include "rac3_version.h"

void mp_openrac_game_register_functions(void);
void rac3_program_set_overlay(int overlay); /* program.c */
void rac3_program_keep_level(int level, gaddr buffer_word);
void rac3_program_forget_levels(void);
void rac3_program_set_range(gaddr gp, gaddr low, gaddr high);

#define MP_ARGUMENTS RAC3_MADDR(0x001EBE00u) /* crt0's argument block: argc, then argv */
#define MP_FRONT_END                                                                               \
    RAC3_MADDR(0x0027B1C0u) /* the executable's own program: its front end's entry */
#define MP_FRONT_END_LOW RAC3_MADDR(0x0027B100u)
#define MP_FRONT_END_HIGH RAC3_MADDR(0x002A0A38u)
#define MP_SND_REPLY RAC3_MADDR(0x00240B80u) /* the sound server's reply word ($gp - 0x7440) */
#define MP_MENU_ENTRY                                                                              \
    RAC3_LADDR(39, 0x00575CC8u) /* level 39's program's entry: the multiplayer menus */
/* The level whose program is loaded or being loaded: 39 the menus, 40 and up a map. */
#define MP_LEVEL RAC3_MADDR(0x002412DCu)
/* Where the next program was read (the boot program's 0x001D4B50): a header word, its records. */
#define MP_PROGRAM_BUFFER RAC3_MADDR(0x00240290u)
/* crt0's $gp, the same in every program the executable loads (hostgen.json "gp"). */
#ifdef RAC3_NTSC
#define MP_GP 0x00248140u
#else
#define MP_GP 0x00247FC0u
#endif
/* What a program replaces: level 39's records up. */
#define MP_PROGRAMS_LOW RAC3_MADDR(0x00241A00u)
#define MP_PROGRAMS_HIGH 0x00700000u
#define MP_MAPS_FIRST 40
#define MP_MAPS_LAST 55

/* LoadExecPS2("cdrom0:\I5BOOTN.ELF;1", ...) came back to func_0013ADA8: the multiplayer
 * executable read from the disc, unpacked over memory, its functions in place of the boot
 * program's, and its crt0 run. */
void rac3_mp_run(void) {
    uint32_t size = 0;
    uint8_t* file = rac3_exec_read_disc_file("I5BOOTN.ELF", &size);
    if (file == NULL) {
        fprintf(stderr, "rac3: I5BOOTN.ELF cannot be read from the disc image\n");
        exit(1);
    }
    memset(G(0x00100000u), 0, 0x02000000u - 0x00100000u);
    openrac_guest_unregister_all();
    openrac_game_kernel_reset();
    openrac_snd_ring_reset();
    openrac_guest_clear_level_programs();
    rac3_program_forget_levels();
    mp_openrac_game_register_functions();
    rac3_program_set_overlay(OPENRAC_OVERLAY_EXE);
    const uint32_t entry = rac3_exec_unpack(file, size);
    free(file);
    if (entry != RAC3_MADDR(0x00192D18u)) {
        fprintf(
            stderr, "rac3: I5BOOTN.ELF's game starts at 0x%08x, not this disc's 0x00192d18\n", entry
        );
        exit(1);
    }
    rac3_exec_write_arguments(MP_ARGUMENTS);
    mp_func_00192D18();
}

/* crt0's user-mode steps (the port owns the thread, the stack and the heap). */
int mp_func_00192D18(void) {
    uint32_t argc;
    mp_func_0012CFD0();  /* _InitSys */
    mp_func_001264E0(0); /* FlushCache(0) */
    memcpy(&argc, G(MP_ARGUMENTS), sizeof argc);
    mp_func_001932A0((int)argc, MP_ARGUMENTS + 4u);
    exit(0);
}

/* LoadExecPS2's wrapper: back to the boot program (leaving multiplayer), or another program. */
void mp_func_0012D318(unsigned int path, int argc, unsigned int argv) {
    rac3_load_exec(path, argc, argv);
}

/* The next program over the last (0x00192F68, the boot program's 0x0013AFF8): its entry. Which
 * program that is says which functions its code addresses are (the overlay hook, program.c): the
 * executable's own front end, level 39's (the multiplayer menus), or a map's (40 and up), whose
 * number the menus leave in 0x002412DC before they end. Level 39's and the map's programs are kept
 * as read, so that level 39's copies of the engine pair with the map's (OPENRAC_LDATA(39, ...):
 * mp_maps.c, render.c's level 39 copies). */
unsigned int mp_func_00192F68(void) {
    const int level = GREF(int32_t, MP_LEVEL);
    rac3_program_set_range(MP_GP, MP_PROGRAMS_LOW, MP_PROGRAMS_HIGH);
    if (level >= 39 && level <= MP_MAPS_LAST) {
        rac3_program_keep_level(level, MP_PROGRAM_BUFFER);
    }
    const unsigned int entry = mp_func_00192F68__game();
    int overlay = OPENRAC_OVERLAY_EXE;
    if (entry == MP_MENU_ENTRY) {
        overlay = 39;
    } else if (level >= MP_MAPS_FIRST && level <= MP_MAPS_LAST
               && (entry < MP_FRONT_END_LOW || entry >= MP_FRONT_END_HIGH)) {
        overlay = level;
    } else if (entry < MP_FRONT_END_LOW || entry >= MP_FRONT_END_HIGH) {
        fprintf(
            stderr,
            "[warning] rac3: multiplayer: a program at 0x%08x, not one the port has\n",
            entry
        );
    }
    rac3_program_set_overlay(overlay);
    fprintf(stderr, "[info] rac3: multiplayer: next program %d, entry 0x%08x\n", overlay, entry);
    return entry;
}

/* InitSystemCallTableAddress: no kernel to scan (entry.c's func_00124A80). */
void mp_func_0012CEC0(void) {}

/* sceDmaSend and KickVif0Chain: the boot program's (dma.c). */
void func_0012D938(gaddr channel, gaddr tag);

void mp_func_0013AE38(unsigned int channel, unsigned int tag) {
    func_0012D938(channel, tag);
}

void mp_func_00298D70(unsigned int chain) {
    func_0012D938(0x10008000u, (gaddr)chain & 0x0FFFFFFFu);
}

/* sceVu0UnitMatrix (vu0.c). */
void func_0012FF40(gaddr m);

void mp_func_0013D5C0(gaddr m) {
    func_0012FF40(m);
}

/* 989snd (sound.c): the reply word is this program's. */
int mp_func_00193788(void) {
    const uint32_t none = 0;
    memcpy(G(MP_SND_REPLY), &none, sizeof none);
    return 1;
}

int mp_func_00193590(void) {
    mp_func_00193788();
    return openrac_snd_ring_pump();
}

int func_0013C348(int command, int bytes, int data);

int mp_func_00193F90(int command, int bytes, int data) {
    return func_0013C348(command, bytes, data);
}

int func_0013C578(int command, int bytes, int data, int callback, long long argument);

int mp_func_001941C0(int command, int bytes, int data, int callback, long long argument) {
    return func_0013C578(command, bytes, data, callback, argument);
}

/* VU0's cosine and sine (trig.c). */
float mp_func_00280498(float angle) {
    return cosf(angle);
}

float mp_func_002804B0(float angle) {
    return sinf(angle);
}

/* libpad (pad.c). */
int func_0012EC90(int mode);
int func_0012EF10(void);
int func_0012F0B8(int port, int slot, int work);
int func_0012F3F8(int port, int slot, int data);
int func_0012F470(int port, int slot);
int func_0012F5A8(int port, int slot);
int func_0012F608(int port, int slot, int actno, int term);
int func_0012F728(int port, int slot, int term, int offs);
int func_0012F860(int port, int slot, int offs, int lock);
int func_0012F918(int port, int slot, int data);
int func_0012F9E0(int port, int slot, int data);
int func_0012FC18(int port, int slot);
int func_0012FC78(int port, int slot);

int mp_func_0013C268(int mode) {
    return func_0012EC90(mode);
}

int mp_func_0013C4E8(void) {
    return func_0012EF10();
}

int mp_func_0013C690(int port, int slot, int work) {
    return func_0012F0B8(port, slot, work);
}

int mp_func_0013CA88(int port, int slot, int data) {
    return func_0012F3F8(port, slot, data);
}

int mp_func_0013CB00(int port, int slot) {
    return func_0012F470(port, slot);
}

int mp_func_0013CC38(int port, int slot) {
    return func_0012F5A8(port, slot);
}

int mp_func_0013CC98(int port, int slot, int actno, int term) {
    return func_0012F608(port, slot, actno, term);
}

int mp_func_0013CDB8(int port, int slot, int term, int offs) {
    return func_0012F728(port, slot, term, offs);
}

int mp_func_0013CEF0(int port, int slot, int offs, int lock) {
    return func_0012F860(port, slot, offs, lock);
}

int mp_func_0013CFA8(int port, int slot, int data) {
    return func_0012F918(port, slot, data);
}

int mp_func_0013D070(int port, int slot, int data) {
    return func_0012F9E0(port, slot, data);
}

int mp_func_0013D2A8(int port, int slot) {
    return func_0012FC18(port, slot);
}

int mp_func_0013D308(int port, int slot) {
    return func_0012FC78(port, slot);
}

/* The WAD decompressor (wad.c). */
int mp_func_0028BBE0(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}

/* sprintf, formatted by the port (print.c). */
int mp_func_00120048(unsigned int buffer, unsigned int format, ...) {
    char out[1024];
    int n;
    va_list args;

    va_start(args, format);
    n = openrac_guest_vformat(out, sizeof(out), (const char*)G(format), args);
    va_end(args);
    memcpy(G(buffer), out, (size_t)n + 1);
    return n;
}

/* sceSifLoadModule (0x0012C528): a module of the disc loaded on the IOP (the network adapter's
 * driver, cdrom0:\DEV9.IRX, which the start-up retries until it loads). The port has no IOP: the
 * module is loaded, as kernel.c answers sceSifLoadModuleBuffer. */
int mp_func_0012C528(unsigned int path, int size, unsigned int args) {
    (void)path;
    (void)size;
    (void)args;
    return 1;
}

/* snd_BankLoadAsync (0x00193888): the bank at a place of the disc, its handle told to the callback
 * (sound.c). The loader of the menus waits for it (its state 3, 0x0028795C: the handle at
 * 0x001FE0B0). */
void func_0013BAB8(int sector, int offset, int callback, long long argument);

void mp_func_00193888(int sector, int offset, int callback, long long argument) {
    func_0013BAB8(sector, offset, callback, argument);
}

/* Level 39's (the multiplayer menus') copies of KickVif0Chain and the WAD decompressor (the
 * catalogue's frontbin rows func_003A8F90 and func_003A0760 at 39:005FCB98 and 39:005A9AA0), as
 * the boot program's levels' (dma.c, wad.c). */
void mp_func_L39_005FCB98(unsigned int chain) {
    func_0012D938(0x10008000u, (gaddr)chain & 0x0FFFFFFFu);
}

int mp_func_L39_005A9AA0(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}

/* Level 39's copies of the screen glow (the catalogue's func_L01_004DE670, func_L01_004DE6F0 and
 * func_L01_004DF310 at 39:0060DFC8, 39:0060E048 and 39:0060EC68): left out as render.c leaves the
 * boot program's levels' (their packets copy and blur the frame and add it back; replayed, they
 * covered the menus in one colour). The word each sets for the frame ($gp - 0x240C) is kept. */
#define MP_MENUS_GLOW_WORD RAC3_LADDR(39, 0x00245BB4u)

int mp_func_L39_0060DFC8(void) {
    GREF(int32_t, MP_MENUS_GLOW_WORD) = 0x6000;
    return 0;
}

int mp_func_L39_0060E048(void) {
    GREF(int32_t, MP_MENUS_GLOW_WORD) = 0;
    return 0;
}

int mp_func_L39_0060EC68(void) {
    return 0;
}
