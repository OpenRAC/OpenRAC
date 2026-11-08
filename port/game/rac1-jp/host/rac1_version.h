/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank (NTSC-J v1.02, SCPS-15037): the addresses of the shared host C (port/game/rac1/host,
 * rac1.h) in this release, carried from rac1-pal's table by port/tools/hostgen/release_rac1.py
 * (release_carry.json says what rests on less than the release's check). The function names are
 * host/rac1_names.tsv's (release_host.py).
 */
#ifndef OPENRAC_RAC1_VERSION_H
#define OPENRAC_RAC1_VERSION_H

/* The capsule query's last argument, as this version's C declares it (host/collision.c). */
#define RAC1_COLL_CAPSULE_IGNORE gaddr

gaddr rac1_ship_destination_text(int id);
#define RAC1_DESTINATION_TEXT(id) rac1_ship_destination_text(id)

/* Level vtbl records carry moby update functions; the executable's vtbl is an empty sentinel. */
#define RAC1_MOBY_VTBL(level) rac1_moby_vtbl_address(level)

static inline unsigned rac1_moby_vtbl_address(int level) {
    static const unsigned addresses[] = {
        0x001EAA00u, 0x0020C180u, 0x001FB600u, 0x001E3F00u, 0x001DD000u, 0x00219380u, 0x00202200u,
        0x00212380u, 0x001FCF80u, 0x00209A00u, 0x001DDC80u, 0x0021BF80u, 0x0020D280u, 0x001F5F00u,
        0x001FD980u, 0x001E3D00u, 0x001E9000u, 0x001E7780u, 0x001F3380u,
    };
    return level >= 0 && (unsigned)level < sizeof(addresses) / sizeof(addresses[0])
               ? addresses[level]
               : 0;
}

/* gHeroPos (D_0013F4D0), for crash reports (boot.c); a version without it reports no position. */
#define RAC1_HERO_POS 0x0013F4D0u

/* The game's folder on the memory card (memcard_get_name's, "BA****-*****RATCHET" with the disc's
 * region and serial), where its save files are (play.c, the mods' saves). */
#define RAC1_SAVE_FOLDER "BISCPS-15037RATCHET"

/* Where libmc (func_001236F0..func_00124528, the port's lib/mc.c) keeps the state of a command, for
 * the port's library to keep it there too (boot.c, openrac_game_mc_state): D_00132EA8 the command that
 * runs (0: none; sceMcSync func_00123F30 returns -1 then, else hands it out and clears it once the
 * IOP has answered), D_0015B0C0 the library's RPC receive buffer, whose first word is the command's
 * result that sceMcSync hands out. A version without them keeps lib/mc.c's own. */
#define RAC1_MC_COMMAND 0x00132EA8u
#define RAC1_MC_RESULT 0x0015B0C0u

/* The game's memory card record (memory_card_state) and its words play.c reads: which card is in
 * use, the card's header mismatches, whether saving is on, the operation running and the one
 * waiting. The card's save number is at +0x14 on every disc. */
#define RAC1_CARD_STATE 0x0013D390u
#define RAC1_CARD_ERRORS 0xB4
#define RAC1_CARD_ACTIVE 0xC8
#define RAC1_CARD_OPERATION 0xDC
#define RAC1_CARD_PENDING 0xE4
#define RAC1_CARD_PENDING_CARD 0xE8
#define RAC1_CARD_SAVING 0xFC

/* What this version leaves out of the shared C (rac1.h, RAC1_OMIT), and why. */
/* The pause menu mobys' update at 0x0023B578: the decompilation has C for it (func_0023B578) since the
 * remnant in front of it was split off. */
#define RAC1_OMIT_game_D_0023B578 1
/* shrub_class_init: the decompilation's candidate (nonmatching/functional,
 * register_shrub_render_class) is the function, the GS texture registers of the
 * class's packets included. */
#define RAC1_OMIT_func_00204340 1
/* The executable's sky tile cull (sky_bsphere_check, SkyBsphereCheck's twin): its candidate is the function. */
#define RAC1_OMIT_func_0022D7E0 1

/* Where this version's decompilation declares a function otherwise: the type in its prototypes. */
/* ParseBin's result, the loaded level's entry. */
#define RAC1_TYPE_func_0012DA38_result gaddr
/* DrawTexturedQuad's colour and TEX0. */
#define RAC1_TYPE_func_001F5800_u64 unsigned long long
/* The text lookup's result, a string's address. */
#define RAC1_TYPE_func_001FE540_result gaddr
/* The sprite's TEX0 (func_00200CA0). */
#define RAC1_TYPE_func_00200CA0_tex0 long long
/* UpdateWorldObjectAnimation's parameter (scene_actors.c): the chunk's slot, which it does not read. */
#define RAC1_TYPE_func_00204FC0_params gaddr chunk __attribute__((unused))
/* memcard_RestoreGame's save, read through its address (options.c). */
#define RAC1_TYPE_func_00209CE8_save int

/* Where UpdateWorldObjectAnimation keeps the model record's word at +8 in the transfer state
 * D_0018CC20 (scene_actors.c). */
#define RAC1_TRANSFER_PAD08 0x46
/* draw_bootImage: NTSC-J's catalogue (config/versions/jp/functions.tsv) has no counterpart of it, so the shared C's wrapper of it is left out. */
#define RAC1_OMIT_func_00201AF0 1
/* FontPrintWindow in the small font: NTSC-J has no counterpart of it (its catalogue's func_J_001F8688 is a second copy of the large font's), so the shared C's replacement is left out. */
#define RAC1_OMIT_func_001F75D0 1
/* FontPrintWindow as NTSC-J's own C calls it: its font_print_window_regular (src/versions/jp/game/render/draw/func_001F7560.c) draws with func_J_001F81F8, its own routine, not PAL v2.00's func_001F7070. */
#define RAC1_FONT_PRINT_WINDOW func_001F81F8

/* ---- generated below ---- */

/* Addresses: games/rac1's, this version's (RAC1_LADDR: in level NN's program). */
#define RAC1_ADDR(a) RAC1_ADDR_##a
#define RAC1_LADDR(n, a) RAC1_ADDR_L##n##_##a
#define RAC1_ADDR_0x0010E4C0u 0x0010E4C0u
#define RAC1_ADDR_0x00137C80u 0x00137C80u
#define RAC1_ADDR_0x0013CA40u 0x0013CA40u
#define RAC1_ADDR_0x0013D010u 0x0013D010u
#define RAC1_ADDR_0x0013D490u 0x0013D490u
#define RAC1_ADDR_0x0013D510u 0x0013D510u
#define RAC1_ADDR_0x0013D5C8u 0x0013D5C8u
#define RAC1_ADDR_0x0013E130u 0x0013E130u
#define RAC1_ADDR_0x0013E150u 0x0013E150u
#define RAC1_ADDR_0x0013E15Au 0x0013E15Au
#define RAC1_ADDR_0x0013E604u 0x0013E604u
#define RAC1_ADDR_0x0013E610u 0x0013E610u
#define RAC1_ADDR_0x0013E614u 0x0013E614u
#define RAC1_ADDR_0x0013E620u 0x0013E620u
#define RAC1_ADDR_0x0013E650u 0x0013E650u
#define RAC1_ADDR_0x0013F450u 0x0013F450u
#define RAC1_ADDR_0x0014C150u 0x0014C150u
#define RAC1_ADDR_0x001517D0u 0x001517D0u
#define RAC1_ADDR_0x00151C60u 0x00151C60u
#define RAC1_ADDR_0x0015EDA0u 0x0015EDA0u
#define RAC1_ADDR_0x0015EDA8u 0x0015EDA8u
#define RAC1_ADDR_0x0015EDB0u 0x0015EDB0u
#define RAC1_ADDR_0x0015EDC0u 0x0015EDC0u
#define RAC1_ADDR_0x0015EE60u 0x0015EE60u
#define RAC1_ADDR_0x0015EE64u 0x0015EE64u
#define RAC1_ADDR_0x0015EE68u 0x0015EE68u
#define RAC1_ADDR_0x0015EE6Cu 0x0015EE6Cu
#define RAC1_ADDR_0x0015EE70u 0x0015EE70u
#define RAC1_ADDR_0x0015EE80u 0x0015EE80u
#define RAC1_ADDR_0x0015EE84u 0x0015EE84u
#define RAC1_ADDR_0x0015EE88u 0x0015EE88u
#define RAC1_ADDR_0x0015EE98u 0x0015EE98u
#define RAC1_ADDR_0x0015EEA0u 0x0015EEA0u
#define RAC1_ADDR_0x0015EEB4u 0x0015EEB4u
#define RAC1_ADDR_0x0015EEE0u 0x0015EEE0u
#define RAC1_ADDR_0x0015EEE8u 0x0015EEE8u
#define RAC1_ADDR_0x0015EF4Au 0x0015EF4Au
#define RAC1_ADDR_0x0015F000u 0x0015F000u
#define RAC1_ADDR_0x0015F558u 0x0015F558u
#define RAC1_ADDR_0x0015F564u 0x0015F564u
#define RAC1_ADDR_0x0015F568u 0x0015F568u
#define RAC1_ADDR_0x0015F56Cu 0x0015F56Cu
#define RAC1_ADDR_0x0015F570u 0x0015F570u
#define RAC1_ADDR_0x0015F574u 0x0015F574u
#define RAC1_ADDR_0x0015F690u 0x0015F690u
#define RAC1_ADDR_0x0015F6B0u 0x0015F6B0u
#define RAC1_ADDR_0x0015F6B4u 0x0015F6B4u
#define RAC1_ADDR_0x0015F6E4u 0x0015F6E4u
#define RAC1_ADDR_0x0015F6E8u 0x0015F6E8u
#define RAC1_ADDR_0x0015F6FCu 0x0015F6FCu
#define RAC1_ADDR_0x00160000u 0x00160000u
#define RAC1_ADDR_0x00160018u 0x00160018u
#define RAC1_ADDR_0x0016001Cu 0x0016001Cu
#define RAC1_ADDR_0x00160020u 0x00160020u
#define RAC1_ADDR_0x00160024u 0x00160024u
#define RAC1_ADDR_0x00160030u 0x00160030u
#define RAC1_ADDR_0x00160048u 0x00160048u
#define RAC1_ADDR_0x0016004Cu 0x0016004Cu
#define RAC1_ADDR_0x00160050u 0x00160050u
#define RAC1_ADDR_0x0016044Cu 0x0016044Cu
#define RAC1_ADDR_0x0016055Cu 0x0016055Cu
#define RAC1_ADDR_0x00160588u 0x00160588u
#define RAC1_ADDR_0x00160FE0u 0x00160FE0u
#define RAC1_ADDR_0x00161000u 0x00161000u
#define RAC1_ADDR_0x00161388u 0x00161388u
#define RAC1_ADDR_0x00166D00u 0x00166D00u
#define RAC1_ADDR_0x001864D0u 0x001864D0u
#define RAC1_ADDR_0x00187040u 0x00187040u
#define RAC1_ADDR_0x00187180u 0x00187180u
#define RAC1_ADDR_0x00187390u 0x00187390u
#define RAC1_ADDR_0x00187394u 0x00187394u
#define RAC1_ADDR_0x0018A3D0u 0x0018A3D0u
#define RAC1_ADDR_0x0018CC20u 0x0018CC20u
#define RAC1_ADDR_0x0018CE00u 0x0018CE00u
#define RAC1_ADDR_0x0018D140u 0x0018D140u
#define RAC1_ADDR_0x0018E040u 0x0018E040u
#define RAC1_ADDR_0x0018E140u 0x0018E140u
#define RAC1_ADDR_0x0018E240u 0x0018E240u
#define RAC1_ADDR_0x0018E340u 0x0018E340u
#define RAC1_ADDR_0x00193400u 0x00193400u
#define RAC1_ADDR_0x001940C0u 0x00193EC0u
#define RAC1_ADDR_0x001941C0u 0x00193FC0u
#define RAC1_ADDR_0x00194200u 0x00194000u
#define RAC1_ADDR_0x001984C0u 0x001982C0u
#define RAC1_ADDR_0x00199C60u 0x00199A60u
#define RAC1_ADDR_0x0019A4E8u 0x0019A2E8u
#define RAC1_ADDR_0x0019BEC0u 0x0019BCC0u
#define RAC1_ADDR_0x001A01F0u 0x0019FFF0u
#define RAC1_ADDR_0x001AAF40u 0x001AAD40u
#define RAC1_ADDR_0x001B2F40u 0x001B2D40u
#define RAC1_ADDR_0x001B3580u 0x001B3380u
#define RAC1_ADDR_0x001B3900u 0x001B3700u
#define RAC1_ADDR_0x001B3E40u 0x001B3C40u
#define RAC1_ADDR_0x001B7A60u 0x001B7860u
#define RAC1_ADDR_0x001C7AC0u 0x001C78C0u
#define RAC1_ADDR_0x001C7C80u 0x001C7A80u
#define RAC1_ADDR_0x001CBE40u 0x001CBC40u
#define RAC1_ADDR_0x001CE720u 0x001CE520u
#define RAC1_ADDR_0x001CF538u 0x001CF338u
#define RAC1_ADDR_0x001CF798u 0x001CF598u
#define RAC1_ADDR_0x001D29D0u 0x001D27D0u
#define RAC1_ADDR_0x001D4948u 0x001D4748u
#define RAC1_ADDR_0x001D4B08u 0x001D4908u
#define RAC1_ADDR_0x001D4B90u 0x001D4990u
#define RAC1_ADDR_0x001D4B9Cu 0x001D499Cu
#define RAC1_ADDR_0x001D4BE8u 0x001D49E8u
#define RAC1_ADDR_0x001D4CC0u 0x001D4AC0u
#define RAC1_ADDR_0x001D4D48u 0x001D4B48u
#define RAC1_ADDR_0x001D4DE8u 0x001D4BE8u
#define RAC1_ADDR_0x001D4E38u 0x001D4C38u
#define RAC1_ADDR_0x001D4F18u 0x001D4D18u
#define RAC1_ADDR_0x001D4F40u 0x001D4D40u
#define RAC1_ADDR_0x001D5F70u 0x001D5D70u
#define RAC1_ADDR_0x001D5F74u 0x001D5D74u
#define RAC1_ADDR_0x001D6120u 0x001D5F20u
#define RAC1_ADDR_0x001D6160u 0x001D5F60u
#define RAC1_ADDR_0x001D61A0u 0x001D5FA0u
#define RAC1_ADDR_0x001D61E0u 0x001D5FE0u
#define RAC1_ADDR_0x001D9A70u 0x001D9870u
#define RAC1_ADDR_0x001DF3D0u 0x001DFCB0u
#define RAC1_ADDR_0x001DF770u 0x001DFCB0u
#define RAC1_ADDR_0x001DFB10u 0x001DFCB0u
#define RAC1_ADDR_0x001E8F00u 0x001E9280u
#define RAC1_ADDR_0x001E8F80u 0x001E9300u
#define RAC1_ADDR_0x00225E00u 0x00226B90u
#define RAC1_ADDR_0x00225FB8u 0x00226D48u
#define RAC1_ADDR_0x002260A8u 0x00226E38u
#define RAC1_ADDR_L00_0x0015F044u 0x0015F044u
#define RAC1_ADDR_L00_0x0015F650u 0x0015F650u
#define RAC1_ADDR_L00_0x0015F6A8u 0x0015F6A8u
#define RAC1_ADDR_L00_0x002618D8u 0x00262838u /* GiveItem */
#define RAC1_ADDR_L00_0x0015F6F8u 0x0015F6F8u
#define RAC1_ADDR_L00_0x0015F808u 0x0015F808u
#define RAC1_ADDR_L00_0x0015FB28u 0x0015FB28u
#define RAC1_ADDR_L00_0x0015FB2Cu 0x0015FB2Cu
#define RAC1_ADDR_L00_0x00160098u 0x00160098u
#define RAC1_ADDR_L00_0x0016022Cu 0x0016022Cu
#define RAC1_ADDR_L00_0x00160234u 0x00160234u
#define RAC1_ADDR_L00_0x00161D7Cu 0x00161D6Cu
#define RAC1_ADDR_L00_0x00161D80u 0x00161D70u
#define RAC1_ADDR_L00_0x00166F00u 0x00166F00u
#define RAC1_ADDR_L00_0x00166F10u 0x00166F10u
#define RAC1_ADDR_L00_0x0016C960u 0x0016C960u
#define RAC1_ADDR_L00_0x00173F00u 0x00173D00u
#define RAC1_ADDR_L00_0x00173F40u 0x00173D40u
#define RAC1_ADDR_L00_0x00178000u 0x00177E00u
#define RAC1_ADDR_L00_0x00178200u 0x00178000u
#define RAC1_ADDR_L00_0x00179BC0u 0x001799C0u
#define RAC1_ADDR_L00_0x0017A6C0u 0x0017A4C0u
#define RAC1_ADDR_L00_0x0017A780u 0x0017A580u
#define RAC1_ADDR_L00_0x0019BB60u 0x0019B960u
#define RAC1_ADDR_L00_0x001ACB00u 0x001AC900u
#define RAC1_ADDR_L00_0x001B1C00u 0x001B1A00u
#define RAC1_ADDR_L00_0x001BB610u 0x001BB410u
#define RAC1_ADDR_L00_0x001C2840u 0x001C2640u
#define RAC1_ADDR_L00_0x001C2870u 0x001C2670u
#define RAC1_ADDR_L00_0x001C3280u 0x001C3080u
#define RAC1_ADDR_L00_0x001E7F90u 0x001E8390u
#define RAC1_ADDR_L03_0x001DC460u 0x001DC860u
#define RAC1_ADDR_L05_0x00160098u 0x00160098u
#define RAC1_ADDR_L05_0x001672C0u 0x001672C0u
#define RAC1_ADDR_L05_0x001AC040u 0x001ABE40u
#define RAC1_ADDR_L05_0x001B0CB0u 0x001B0AB0u
#define RAC1_ADDR_L07_0x001D36C0u 0x001D3AC0u
#define RAC1_ADDR_L07_0x00208820u 0x001D3AC0u
#define RAC1_ADDR_L08_0x001D7320u 0x001D7720u
#define RAC1_ADDR_L09_0x001FCA00u 0x001FCE00u
#define RAC1_ADDR_L09_0x002F0AA0u 0x002F16D8u
#define RAC1_ADDR_L11_0x00160058u 0x00160058u
#define RAC1_ADDR_L11_0x001AC540u 0x001AC340u
#define RAC1_ADDR_L11_0x001DAB40u 0x001DAF40u
#define RAC1_ADDR_L13_0x00161F18u 0x00161F08u
#define RAC1_ADDR_L13_0x00161F1Cu 0x00161F0Cu
#define RAC1_ADDR_L13_0x00161F20u 0x00161F10u
#define RAC1_ADDR_L13_0x00161F24u 0x00161F14u
#define RAC1_ADDR_L13_0x00161F28u 0x00161F18u
#define RAC1_ADDR_L13_0x00161F2Cu 0x00161F1Cu
#define RAC1_ADDR_L13_0x00161F30u 0x00161F20u
#define RAC1_ADDR_L13_0x00161F34u 0x00161F24u
#define RAC1_ADDR_L13_0x00161F38u 0x00161F28u
#define RAC1_ADDR_L13_0x00161F3Cu 0x00161F2Cu
#define RAC1_ADDR_L13_0x001D9E40u 0x001DA240u
#define RAC1_ADDR_L14_0x001E9FA0u 0x001EA3A0u
#define RAC1_ADDR_L18_0x001DFED0u 0x001E02D0u

#endif /* OPENRAC_RAC1_VERSION_H */
