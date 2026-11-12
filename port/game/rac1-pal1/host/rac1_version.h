/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank (PAL v1.00, SCES-50916, disc v1.00): the addresses of the shared host C (port/game/rac1/host,
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
        0x001EA280u, 0x0020BA00u, 0x001FAE80u, 0x001E3780u, 0x001DC800u, 0x00218C00u, 0x00201A80u,
        0x00211C00u, 0x001FC800u, 0x00209280u, 0x001DD500u, 0x0021B800u, 0x0020CA80u, 0x001F5780u,
        0x001FD200u, 0x001E3580u, 0x001E8800u, 0x001E6F80u, 0x001F2C00u,
    };
    return level >= 0 && (unsigned)level < sizeof(addresses) / sizeof(addresses[0])
               ? addresses[level]
               : 0;
}

/* gHeroPos (D_0013F4D0), for crash reports (boot.c); a version without it reports no position. */
#define RAC1_HERO_POS 0x0013F3D0u

/* The game's folder on the memory card (memcard_get_name's, "BA****-*****RATCHET" with the disc's
 * region and serial), where its save files are (play.c, the mods' saves). */
#define RAC1_SAVE_FOLDER "BESCES-50916RATCHET"

/* Where libmc (func_001236F0..func_00124528, the port's lib/mc.c) keeps the state of a command, for
 * the port's library to keep it there too (boot.c, openrac_game_mc_state): D_00132EA8 the command that
 * runs (0: none; sceMcSync func_00123F30 returns -1 then, else hands it out and clears it once the
 * IOP has answered), D_0015B0C0 the library's RPC receive buffer, whose first word is the command's
 * result that sceMcSync hands out. A version without them keeps lib/mc.c's own. */
#define RAC1_MC_COMMAND 0x00132DA8u
#define RAC1_MC_RESULT 0x0015AFC0u

/* The game's memory card record (memory_card_state) and its words play.c reads: which card is in
 * use, the card's header mismatches, whether saving is on, the operation running and the one
 * waiting. The card's save number is at +0x14 on every disc. */
#define RAC1_CARD_STATE 0x0013D290u
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

/* Aridia's counter anchors (hud.c): release_names.json pairs them, D_L02_0015F91C and D_L02_0015F920. */
#define RAC1_ADDR_L02_0x0015FA1Cu 0x0015F91Cu
#define RAC1_ADDR_L02_0x0015FA20u 0x0015F920u

/* ---- generated below ---- */

/* Addresses: games/rac1's, this version's (RAC1_LADDR: in level NN's program). */
#define RAC1_ADDR(a) RAC1_ADDR_##a
#define RAC1_LADDR(n, a) RAC1_ADDR_L##n##_##a
#define RAC1_ADDR_0x0010E4C0u 0x0010E4C0u
#define RAC1_ADDR_0x00137C80u 0x00137B80u
#define RAC1_ADDR_0x0013CA40u 0x0013C940u
#define RAC1_ADDR_0x0013D010u 0x0013CF10u
#define RAC1_ADDR_0x0013D490u 0x0013D388u
#define RAC1_ADDR_0x0013D510u 0x0013D408u
#define RAC1_ADDR_0x0013D5C8u 0x0013D4C0u
#define RAC1_ADDR_0x0013E130u 0x0013E030u
#define RAC1_ADDR_0x0013E150u 0x0013E050u
#define RAC1_ADDR_0x0013E15Au 0x0013E05Au
#define RAC1_ADDR_0x0013E604u 0x0013E504u
#define RAC1_ADDR_0x0013E610u 0x0013E510u
#define RAC1_ADDR_0x0013E614u 0x0013E514u
#define RAC1_ADDR_0x0013E620u 0x0013E520u
#define RAC1_ADDR_0x0013E650u 0x0013E550u
#define RAC1_ADDR_0x0013F450u 0x0013F350u
#define RAC1_ADDR_0x0014C150u 0x0014C050u
#define RAC1_ADDR_0x001517D0u 0x001516D0u
#define RAC1_ADDR_0x00151C60u 0x00151B60u
#define RAC1_ADDR_0x0015EDA0u 0x0015ECA0u
#define RAC1_ADDR_0x0015EDA8u 0x0015ECA8u
#define RAC1_ADDR_0x0015EDB0u 0x0015ECB0u
#define RAC1_ADDR_0x0015EDC0u 0x0015ECC0u
#define RAC1_ADDR_0x0015EE60u 0x0015ED60u
#define RAC1_ADDR_0x0015EE64u 0x0015ED64u
#define RAC1_ADDR_0x0015EE68u 0x0015ED68u
#define RAC1_ADDR_0x0015EE6Cu 0x0015ED6Cu
#define RAC1_ADDR_0x0015EE70u 0x0015ED70u
#define RAC1_ADDR_0x0015EE80u 0x0015ED80u
#define RAC1_ADDR_0x0015EE84u 0x0015ED84u
#define RAC1_ADDR_0x0015EE88u 0x0015ED88u
#define RAC1_ADDR_0x0015EE98u 0x0015ED98u
#define RAC1_ADDR_0x0015EEA0u 0x0015EDA0u
#define RAC1_ADDR_0x0015EEB4u 0x0015EDB4u
#define RAC1_ADDR_0x0015EEE0u 0x0015EDE0u
#define RAC1_ADDR_0x0015EEE8u 0x0015EDE8u
#define RAC1_ADDR_0x0015EF4Au 0x0015EE4Au
#define RAC1_ADDR_0x0015F000u 0x0015EF00u
#define RAC1_ADDR_0x0015F558u 0x0015F458u
#define RAC1_ADDR_0x0015F564u 0x0015F464u
#define RAC1_ADDR_0x0015F568u 0x0015F468u
#define RAC1_ADDR_0x0015F56Cu 0x0015F46Cu
#define RAC1_ADDR_0x0015F570u 0x0015F470u
#define RAC1_ADDR_0x0015F574u 0x0015F474u
#define RAC1_ADDR_0x0015F690u 0x0015F590u
#define RAC1_ADDR_0x0015F6B0u 0x0015F5B0u
#define RAC1_ADDR_0x0015F6B4u 0x0015F5B4u
#define RAC1_ADDR_0x0015F6E4u 0x0015F5E0u
#define RAC1_ADDR_0x0015F6E8u 0x0015F5E4u
#define RAC1_ADDR_0x0015F6FCu 0x0015F5F8u
#define RAC1_ADDR_0x00160000u 0x0015FEC0u
#define RAC1_ADDR_0x00160018u 0x0015FED8u
#define RAC1_ADDR_0x0016001Cu 0x0015FEDCu
#define RAC1_ADDR_0x00160020u 0x0015FEE0u
#define RAC1_ADDR_0x00160024u 0x0015FEE4u
#define RAC1_ADDR_0x00160030u 0x0015FEF0u
#define RAC1_ADDR_0x00160048u 0x0015FF08u
#define RAC1_ADDR_0x0016004Cu 0x0015FF0Cu
#define RAC1_ADDR_0x00160050u 0x0015FF10u
#define RAC1_ADDR_0x0016044Cu 0x0016030Cu
#define RAC1_ADDR_0x0016055Cu 0x0016041Cu
#define RAC1_ADDR_0x00160588u 0x00160448u
#define RAC1_ADDR_0x00160FE0u 0x00160EA0u
#define RAC1_ADDR_0x00161000u 0x00160EC0u
#define RAC1_ADDR_0x00161388u 0x00161208u
#define RAC1_ADDR_0x00166D00u 0x00166C00u
#define RAC1_ADDR_0x001864D0u 0x00186350u
#define RAC1_ADDR_0x00187040u 0x00186EC0u
#define RAC1_ADDR_0x00187180u 0x00187000u
#define RAC1_ADDR_0x00187390u 0x00187210u
#define RAC1_ADDR_0x00187394u 0x00187214u
#define RAC1_ADDR_0x0018A3D0u 0x0018A250u
#define RAC1_ADDR_0x0018CC20u 0x0018CAA0u
#define RAC1_ADDR_0x0018CE00u 0x0018CC80u
#define RAC1_ADDR_0x0018D140u 0x0018CFC0u
#define RAC1_ADDR_0x0018E040u 0x0018DEC0u
#define RAC1_ADDR_0x0018E140u 0x0018DFC0u
#define RAC1_ADDR_0x0018E240u 0x0018E0C0u
#define RAC1_ADDR_0x0018E340u 0x0018E1C0u
#define RAC1_ADDR_0x00193400u 0x00193280u
#define RAC1_ADDR_0x001940C0u 0x00193F40u
#define RAC1_ADDR_0x001941C0u 0x00194040u
#define RAC1_ADDR_0x00194200u 0x00194080u
#define RAC1_ADDR_0x001984C0u 0x00198340u
#define RAC1_ADDR_0x00199C60u 0x00199AE0u
#define RAC1_ADDR_0x0019A4E8u 0x0019A368u
#define RAC1_ADDR_0x0019BEC0u 0x0019BD40u
#define RAC1_ADDR_0x001A01F0u 0x001A0070u
#define RAC1_ADDR_0x001AAF40u 0x001AAB40u
#define RAC1_ADDR_0x001B2F40u 0x001B2B40u
#define RAC1_ADDR_0x001B3580u 0x001B3180u
#define RAC1_ADDR_0x001B3900u 0x001B3500u
#define RAC1_ADDR_0x001B3E40u 0x001B3A40u
#define RAC1_ADDR_0x001B7A60u 0x001B7660u
#define RAC1_ADDR_0x001C7AC0u 0x001C76C0u
#define RAC1_ADDR_0x001C7C80u 0x001C7880u
#define RAC1_ADDR_0x001CBE40u 0x001CBA40u
#define RAC1_ADDR_0x001CE720u 0x001CE320u
#define RAC1_ADDR_0x001CF538u 0x001CF138u
#define RAC1_ADDR_0x001CF798u 0x001CF398u
#define RAC1_ADDR_0x001D29D0u 0x001D25D0u
#define RAC1_ADDR_0x001D4948u 0x001D4548u
#define RAC1_ADDR_0x001D4B08u 0x001D4708u
#define RAC1_ADDR_0x001D4B90u 0x001D4790u
#define RAC1_ADDR_0x001D4B9Cu 0x001D479Cu
#define RAC1_ADDR_0x001D4BE8u 0x001D47E8u
#define RAC1_ADDR_0x001D4CC0u 0x001D48C0u
#define RAC1_ADDR_0x001D4D48u 0x001D4948u
#define RAC1_ADDR_0x001D4DE8u 0x001D49E8u
#define RAC1_ADDR_0x001D4E38u 0x001D4A38u
#define RAC1_ADDR_0x001D4F18u 0x001D4B18u
#define RAC1_ADDR_0x001D4F40u 0x001D4B40u
#define RAC1_ADDR_0x001D5F70u 0x001D5B70u
#define RAC1_ADDR_0x001D5F74u 0x001D5B74u
#define RAC1_ADDR_0x001D6120u 0x001D5D10u
#define RAC1_ADDR_0x001D6160u 0x001D5D50u
#define RAC1_ADDR_0x001D61A0u 0x001D5D90u
#define RAC1_ADDR_0x001D61E0u 0x001D5DD0u
#define RAC1_ADDR_0x001D9A70u 0x001D9660u
#define RAC1_ADDR_0x001DF3D0u 0x001DEFD0u
#define RAC1_ADDR_0x001DF770u 0x001DF370u
#define RAC1_ADDR_0x001DFB10u 0x001DF710u
#define RAC1_ADDR_0x001E8F00u 0x001E8A80u
#define RAC1_ADDR_0x001E8F80u 0x001E8B00u
#define RAC1_ADDR_0x00225E00u 0x00224670u
#define RAC1_ADDR_0x00225FB8u 0x00224828u
#define RAC1_ADDR_0x002260A8u 0x00224918u
#define RAC1_ADDR_L00_0x0015F044u 0x0015EF44u
#define RAC1_ADDR_L00_0x0015F650u 0x0015F550u
#define RAC1_ADDR_L00_0x0015F6A8u 0x0015F5A4u
#define RAC1_ADDR_L00_0x002618D8u 0x00260470u /* GiveItem */
#define RAC1_ADDR_L00_0x0015F6F8u 0x0015F5F8u
#define RAC1_ADDR_L00_0x0015F808u 0x0015F708u
#define RAC1_ADDR_L00_0x0015FB28u 0x0015FA28u
#define RAC1_ADDR_L00_0x0015FB2Cu 0x0015FA2Cu
#define RAC1_ADDR_L00_0x00160098u 0x0015FF98u
#define RAC1_ADDR_L00_0x0016022Cu 0x0016012Cu
#define RAC1_ADDR_L00_0x00160234u 0x00160134u
#define RAC1_ADDR_L00_0x00161D7Cu 0x00161C6Cu
#define RAC1_ADDR_L00_0x00161D80u 0x00161C70u
#define RAC1_ADDR_L00_0x00166F00u 0x00166E00u
#define RAC1_ADDR_L00_0x00166F10u 0x00166E10u
#define RAC1_ADDR_L00_0x0016C960u 0x0016C860u
#define RAC1_ADDR_L00_0x00173F00u 0x00173E00u
#define RAC1_ADDR_L00_0x00173F40u 0x00173E40u
#define RAC1_ADDR_L00_0x00178000u 0x00177F00u
#define RAC1_ADDR_L00_0x00178200u 0x00178100u
#define RAC1_ADDR_L00_0x00179BC0u 0x00179AC0u
#define RAC1_ADDR_L00_0x0017A6C0u 0x0017A5C0u
#define RAC1_ADDR_L00_0x0017A780u 0x0017A680u
#define RAC1_ADDR_L00_0x0019BB60u 0x0019B7E0u
#define RAC1_ADDR_L00_0x001ACB00u 0x001AC780u
#define RAC1_ADDR_L00_0x001B1C00u 0x001B1880u
#define RAC1_ADDR_L00_0x001BB610u 0x001BB280u
#define RAC1_ADDR_L00_0x001C2840u 0x001C24C0u
#define RAC1_ADDR_L00_0x001C2870u 0x001C24F0u
#define RAC1_ADDR_L00_0x001C3280u 0x001C2F00u
#define RAC1_ADDR_L00_0x001E7F90u 0x001E7B90u
#define RAC1_ADDR_L03_0x001DC460u 0x001DC0A0u
#define RAC1_ADDR_L05_0x00160098u 0x0015FF98u
#define RAC1_ADDR_L05_0x001672C0u 0x001671C0u
#define RAC1_ADDR_L05_0x001AC040u 0x001ABCC0u
#define RAC1_ADDR_L05_0x001B0CB0u 0x001B0930u
#define RAC1_ADDR_L07_0x001D36C0u 0x001D3300u
#define RAC1_ADDR_L07_0x00208820u 0x001D3300u
#define RAC1_ADDR_L08_0x001D7320u 0x001D6F60u
#define RAC1_ADDR_L09_0x001FCA00u 0x001FC640u
#define RAC1_ADDR_L09_0x002F0AA0u 0x002EF078u
#define RAC1_ADDR_L11_0x00160058u 0x0015FF58u
#define RAC1_ADDR_L11_0x001AC540u 0x001AC1C0u
#define RAC1_ADDR_L11_0x001DAB40u 0x001DA780u
#define RAC1_ADDR_L13_0x00161F18u 0x00161E08u
#define RAC1_ADDR_L13_0x00161F1Cu 0x00161E0Cu
#define RAC1_ADDR_L13_0x00161F20u 0x00161E10u
#define RAC1_ADDR_L13_0x00161F24u 0x00161E14u
#define RAC1_ADDR_L13_0x00161F28u 0x00161E18u
#define RAC1_ADDR_L13_0x00161F2Cu 0x00161E1Cu
#define RAC1_ADDR_L13_0x00161F30u 0x00161E20u
#define RAC1_ADDR_L13_0x00161F34u 0x00161E24u
#define RAC1_ADDR_L13_0x00161F38u 0x00161E28u
#define RAC1_ADDR_L13_0x00161F3Cu 0x00161E2Cu
#define RAC1_ADDR_L13_0x001D9E40u 0x001D9A70u
#define RAC1_ADDR_L14_0x001E9FA0u 0x001E9BE0u
#define RAC1_ADDR_L18_0x001DFED0u 0x001DFB10u

#endif /* OPENRAC_RAC1_VERSION_H */
