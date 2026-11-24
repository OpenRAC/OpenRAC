/* Written by port/tools/hostgen/release_carry_rac3.py; do not edit. */
/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * NTSC-U's address for each PAL address the shared host C writes as RAC3_ADDR(0x...u) (rac3_version.h):
 * the global or function of that address renamed (config/us/names.tsv, config/us/map.tsv), else
 * release/release.json's `addresses`. One not known is 0: what reads it reads nothing of the game's. */
#ifndef OPENRAC_RAC3_ADDRESSES_H
#define OPENRAC_RAC3_ADDRESSES_H

/* D_00141850 (names.tsv, the same address) */
#define RAC3_ADDR_0x00141850u 0x00141850u
/* D_00142120 (names.tsv, the same address) */
#define RAC3_ADDR_0x00142120u 0x00142120u
/* D_00142430 (names.tsv, the same address) */
#define RAC3_ADDR_0x00142430u 0x00142430u
/* D_001425C0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001425C0u 0x001425C0u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_00317C48 (NTSC-U func_L01_003177F8) +0x120, 34 of 34 places agree */
#define RAC3_ADDR_0x00142660u 0x00142660u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_00301798 (NTSC-U func_L01_003015F0) +0x94, 3 of 3 places agree */
#define RAC3_ADDR_0x00142668u 0x00142668u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_0030F0B0 (NTSC-U func_L01_0030EDC0) +0xC0, 18 of 18 places agree */
#define RAC3_ADDR_0x001427F0u 0x001427F0u
/* D_00142CA0 (names.tsv, the same address) */
#define RAC3_ADDR_0x00142CA0u 0x00142CA0u
/* D_00143950 (names.tsv, the same address) */
#define RAC3_ADDR_0x00143950u 0x00143950u
/* release.json: the same instruction of the same code in Veldin's program (rac3_level_pairs.py): func_L01_004D0E90 (NTSC-U func_L01_004CD1D0) +0x68, 2 of 2 places; the options block D_00143950 is at the same address on both discs */
#define RAC3_ADDR_0x001439F4u 0x001439F4u
/* D_001439FD (names.tsv, the same address) */
#define RAC3_ADDR_0x001439FDu 0x001439FDu
/* release.json: the same instruction of the same code in Veldin's program (rac3_level_pairs.py): func_L01_0049FBC8 (NTSC-U func_L01_0049C368) +0x1DC, 2 of 2 places; the options block D_00143950 is at the same address on both discs */
#define RAC3_ADDR_0x00143A70u 0x00143A70u
/* D_00160C40 (names.tsv, the same address) */
#define RAC3_ADDR_0x00160C40u 0x00160C40u
/* D_001A4BE0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001A4BE0u 0x001A4BE0u
/* D_001A4C60 (names.tsv, the same address) */
#define RAC3_ADDR_0x001A4C60u 0x001A4C60u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_004E51A0 (NTSC-U func_L01_004E13D8) +0x20, 3 of 3 places agree */
#define RAC3_ADDR_0x001A4C70u 0x001A4C70u
/* release.json: hero_carried, hero + 0x150: the hero D_001A4BE0 is at the same address on both discs (names.tsv), and names.tsv describes hero_carried at NTSC-U 0x001A4D30 */
#define RAC3_ADDR_0x001A4D30u 0x001A4D30u
/* release.json: hero_ledge_block, hero + 0x500 (rac3-decomp include/structs/hero.h); the hero D_001A4BE0 is at the same address on both discs (names.tsv), and names.tsv describes the block at NTSC-U 0x001A50E0 */
#define RAC3_ADDR_0x001A50E0u 0x001A50E0u
/* release.json: Veldin's program (rac3_level_pairs.py): between the same code's 0x001A5780 and 0x001A57F8, both moved by +0x0 (NTSC-U 0x001A5780, 0x001A57F8) */
#define RAC3_ADDR_0x001A57BCu 0x001A57BCu
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_0034DBE8 (NTSC-U func_L01_0034D168) +0x8, 3 of 3 places agree */
#define RAC3_ADDR_0x001A5E08u 0x001A5E08u
/* release.json: the same instruction of the same code in Veldin's program (rac3_level_pairs.py): func_L01_003090E0 (NTSC-U func_L01_00308E58) +0x80, 31 of 31 places agree */
#define RAC3_ADDR_0x001A71A0u 0x001A71A0u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_003261E8 (NTSC-U func_L01_00325CE8) +0x100, 8 of 8 places agree */
#define RAC3_ADDR_0x001A71A4u 0x001A71A4u
/* release.json: Veldin's program (rac3_level_pairs.py): between the same code's 0x001A71C5 and 0x001A71D4, both moved by +0x0 (NTSC-U 0x001A71C5, 0x001A71D4) */
#define RAC3_ADDR_0x001A71D2u 0x001A71D2u
/* D_001A7430 (names.tsv, the same address) */
#define RAC3_ADDR_0x001A7430u 0x001A7430u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_00317C48 (NTSC-U func_L01_003177F8) +0x78, 24 of 24 places agree */
#define RAC3_ADDR_0x001BBC40u 0x001BBC40u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_00325F60 (NTSC-U func_L01_00325A60) +0xA8, 2 of 2 places agree */
#define RAC3_ADDR_0x001C5290u 0x001C5290u
/* D_001CD0C0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001CD0C0u 0x001CD0C0u
/* D_001CFEC0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001CFEC0u 0x001CFEC0u
/* D_001D4B48 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4B48u 0x001D4B48u
/* D_001D4B50 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4B50u 0x001D4B50u
/* D_001D4BB0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BB0u 0x001D4BB0u
/* D_001D4BD0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BD0u 0x001D4BD0u
/* release.json: Veldin's program (rac3_level_pairs.py): the same instruction of the same code: func_L01_004CA270 (NTSC-U func_L01_004C6638) +0x138, 7 of 7 places agree */
#define RAC3_ADDR_0x001D4C40u 0x001D4C40u
/* release.json: Veldin's program (rac3_level_pairs.py): between the same code's 0x001D4C80 and 0x001D4CA8, both moved by +0x0 (NTSC-U 0x001D4C80, 0x001D4CA8) */
#define RAC3_ADDR_0x001D4CA0u 0x001D4CA0u
/* D_001D4CE8 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4CE8u 0x001D4CE8u
/* D_001D4CEC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4CECu 0x001D4CECu
/* D_001D4DA0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4DA0u 0x001D4DA0u
/* D_001D52FC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D52FCu 0x001D52FCu
/* D_001D5458 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5458u 0x001D5458u
/* D_001D545C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D545Cu 0x001D545Cu
/* release.json: the two font headers (FontUpload func_0038E2B0 reads them, FontSelect func_0038E530 copies the selected one's into 0x001D55C0): NTSC-U's boot program registers a font's header at the same address (f_0038E140: lui 0x1D, addiu 0x55D0, the font's index times 4, sw), and Veldin's FontSelect reads it there too (f_0040D548) */
#define RAC3_ADDR_0x001D55D0u 0x001D55D0u
/* D_001D55E8 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D55E8u 0x001D55E8u
/* D_001D5688 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5688u 0x001D5688u
/* D_001D568C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D568Cu 0x001D568Cu
/* D_001D5690 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5690u 0x001D5690u
/* D_001D5694 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5694u 0x001D5694u
/* D_001D5B38 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B38u 0x001D5B38u
/* D_001D5B3C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B3Cu 0x001D5B3Cu
/* D_001D5B74 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B74u 0x001D5B74u
/* D_001D5B90 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B90u 0x001D5B90u
/* D_001D5B94 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B94u 0x001D5B94u
/* D_001D5BDC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5BDCu 0x001D5BDCu
/* D_001D5C90 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5C90u 0x001D5C90u
/* D_001D89B8 (names.tsv) */
#define RAC3_ADDR_0x001D89B8u 0x001D89E8u
/* D_001D89BC (names.tsv) */
#define RAC3_ADDR_0x001D89BCu 0x001D89ECu
/* D_001D9BE0 (names.tsv) */
#define RAC3_ADDR_0x001D9BE0u 0x001D9C60u
/* D_001D9BFC (names.tsv) */
#define RAC3_ADDR_0x001D9BFCu 0x001D9C7Cu
/* D_001D9D04 (names.tsv) */
#define RAC3_ADDR_0x001D9D04u 0x001D9D84u
/* D_001D9E94 (names.tsv) */
#define RAC3_ADDR_0x001D9E94u 0x001D9F14u
/* D_001D9EA8 (names.tsv) */
#define RAC3_ADDR_0x001D9EA8u 0x001D9F28u
/* D_001D9EAC (names.tsv) */
#define RAC3_ADDR_0x001D9EACu 0x001D9F2Cu
/* D_001DA048 (names.tsv) */
#define RAC3_ADDR_0x001DA048u 0x001DA0C8u
/* D_001DA050 (names.tsv) */
#define RAC3_ADDR_0x001DA050u 0x001DA0D0u
/* D_001DA058 (names.tsv) */
#define RAC3_ADDR_0x001DA058u 0x001DA0D8u
/* D_001DA05C (names.tsv) */
#define RAC3_ADDR_0x001DA05Cu 0x001DA0DCu
/* D_001DA4D0 (names.tsv) */
#define RAC3_ADDR_0x001DA4D0u 0x001DA550u
/* D_001DA5F0 (names.tsv) */
#define RAC3_ADDR_0x001DA5F0u 0x001DA670u
/* D_001DA610 (names.tsv) */
#define RAC3_ADDR_0x001DA610u 0x001DA690u
/* D_001DA6B0 (names.tsv) */
#define RAC3_ADDR_0x001DA6B0u 0x001DA730u
/* D_001DA6C0 (names.tsv) */
#define RAC3_ADDR_0x001DA6C0u 0x001DA740u
/* D_001DA6D0 (names.tsv) */
#define RAC3_ADDR_0x001DA6D0u 0x001DA750u
/* D_001DA6F0 (names.tsv) */
#define RAC3_ADDR_0x001DA6F0u 0x001DA770u
/* D_00222420 (names.tsv) */
#define RAC3_ADDR_0x00222420u 0x002224A0u
/* D_00225700 (names.tsv) */
#define RAC3_ADDR_0x00225700u 0x00225780u
/* D_00225734 (names.tsv) */
#define RAC3_ADDR_0x00225734u 0x002257B4u
/* D_00225900 (names.tsv) */
#define RAC3_ADDR_0x00225900u 0x00225980u
/* D_00225C00 (names.tsv) */
#define RAC3_ADDR_0x00225C00u 0x00225C80u
/* D_00227400 (names.tsv) */
#define RAC3_ADDR_0x00227400u 0x00227480u
/* D_00227480 (names.tsv) */
#define RAC3_ADDR_0x00227480u 0x00227500u
/* D_0022758C (names.tsv) */
#define RAC3_ADDR_0x0022758Cu 0x0022760Cu
/* D_00280CF0 (names.tsv) */
#define RAC3_ADDR_0x00280CF0u 0x00280D70u
/* D_002D2008 (names.tsv) */
#define RAC3_ADDR_0x002D2008u 0x002D2088u
/* D_002DA100 (names.tsv) */
#define RAC3_ADDR_0x002DA100u 0x002DA180u
/* D_0037F830 (names.tsv) */
#define RAC3_ADDR_0x0037F830u 0x0037F8B0u
/* func_00381250 (map.tsv differs) */
#define RAC3_ADDR_0x00381250u 0x00381280u

/* The multiplayer executable's (release/mp_pairs.json). */
/* the function's pair: the same instructions (135 places), at the distance the nearest paired function 0x001266C0 moved by (+0x0) */
#define RAC3_MADDR_0x001260A0u 0x001260A0u
/* the function's pair: the same instructions, the only place they occur */
#define RAC3_MADDR_0x0014CE48u 0x0014CE48u
/* the function's pair: the same instructions, the only place they occur */
#define RAC3_MADDR_0x00152FB8u 0x00152FB8u
/* the function's pair: the same instructions, the only place they occur */
#define RAC3_MADDR_0x00183BDCu 0x00183CCCu
/* the function's pair: the executable's entry (its payload's records) */
#define RAC3_MADDR_0x00192D18u 0x00192E08u
/* not paired (rac3_mp_pairs.py) */
#define RAC3_MADDR_0x001959F8u 0u
/* the function's pair: the same instructions, the only place they occur */
#define RAC3_MADDR_0x001981D8u 0x00198318u
/* the same instruction of the same code: func 0x001619B8 (NTSC-U 0x001619B8) +0x8, 4 of 4 places agree */
#define RAC3_MADDR_0x001A2A58u 0x001A2BD8u
/* the same instruction of the same code: func 0x0017DFA8 (NTSC-U 0x0017DFA8) +0x10, 5 of 5 places agree */
#define RAC3_MADDR_0x001A30DCu 0x001A325Cu
/* the same instruction of the same code: func 0x0027EDA8 (NTSC-U 0x0027EDE8) +0x94, 1 of 1 places agree */
#define RAC3_MADDR_0x001A3800u 0x001A3980u
/* the same instruction of the same code: func 0x00192D18 (NTSC-U 0x00192E08) +0x1B0, 2 of 2 places agree */
#define RAC3_MADDR_0x001EBE00u 0x001EBF80u
/* the same instruction of the same code: func 0x00195010 (NTSC-U 0x00195100) +0x10, 1 of 1 places agree */
#define RAC3_MADDR_0x001FC6B0u 0x001FC830u
/* release.json: the screen whose size (+0x150, +0x152) SetScissor keeps its rectangle within: PAL mp_func_00299218 and NTSC-U mp_func_002991F8, the same instructions, both read 0x00230150 and 0x00230152 */
#define RAC3_MADDR_0x00230000u 0x00230000u
/* the same instruction of the same code: func 0x00192D18 (NTSC-U 0x00192E08) +0x254, 2 of 2 places agree */
#define RAC3_MADDR_0x00240290u 0x00240410u
/* the same instruction of the same code: func 0x0027EEB8 (NTSC-U 0x0027EEF8) +0xFC, 21 of 21 places agree */
#define RAC3_MADDR_0x00240310u 0x00240490u
/* the same instruction of the same code: func 0x00193590 (NTSC-U 0x00193680) +0x4, 12 of 12 places agree */
#define RAC3_MADDR_0x00240B80u 0x00240D00u
/* not paired (rac3_mp_pairs.py) */
#define RAC3_MADDR_0x00241128u 0u
/* release.json: the level whose program is loaded: read at $gp - 0x6CE4 in both cores (PAL 0x00199150, 0x002945BC, 0x00295534 with $gp 0x00247FC0; NTSC-U 0x00199290, 0x00295524 with $gp 0x00248140) */
#define RAC3_MADDR_0x002412DCu 0x0024145Cu
/* release.json: the two font headers: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) (NTSC-U adds 0x15D8 to 0x00240000) */
#define RAC3_MADDR_0x00241458u 0x002415D8u
/* release.json: the font FontSelect chose last: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) (NTSC-U's map copies store it at 0x002415F0) */
#define RAC3_MADDR_0x00241470u 0x002415F0u
/* the start of the payload's record at PAL 0x00241A00 (NTSC-U 0x00241B80) */
#define RAC3_MADDR_0x00241A00u 0x00241B80u
/* release.json: the core's pointer to the current font's page handles: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) (NTSC-U reads 0x00241E68 beside 0x00241E6C) */
#define RAC3_MADDR_0x00241CE8u 0x00241E68u
/* release.json: the core's pointer to the current font's palette handles, beside 0x00241CE8: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) */
#define RAC3_MADDR_0x00241CECu 0x00241E6Cu
/* release.json: level 39's (and the maps') pointer to the current font's page handles: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) (NTSC-U's map copies store 0x002429A0) */
#define RAC3_MADDR_0x00242820u 0x002429A0u
/* release.json: level 39's (and the maps') pointer to the palette handles, beside 0x00242820: the multiplayer core's FontUpload and FontSelect (PAL mp_func_00280B18 / mp_func_00280D98) and level 39's and the maps' FontSelect copies (PAL mp_func_L39_00591BA0, NTSC-U mp_func_L40_0045DC68 among others) read and write it; NTSC-U's translation has it 0x180 on, as the core's other globals there (0x002412DC, 0x00241A00) */
#define RAC3_MADDR_0x00242824u 0x002429A4u
/* the same instruction of the same code: func 0x00285318 (NTSC-U 0x00285358) +0x1D8, 4 of 4 places agree */
#define RAC3_MADDR_0x00244414u 0x00244614u
/* the same instruction of the same code: func 0x00284D30 (NTSC-U 0x00284D70) +0x4, 2 of 2 places agree */
#define RAC3_MADDR_0x00244428u 0x00244628u
/* the same instruction of the same code: func 0x00284D30 (NTSC-U 0x00284D70) +0x20, 2 of 2 places agree */
#define RAC3_MADDR_0x0024442Cu 0x0024462Cu
/* the same instruction of the same code: func 0x0027E360 (NTSC-U 0x0027E3A0) +0xE4, 141 of 141 places agree */
#define RAC3_MADDR_0x002446A8u 0x002448A8u
/* the start of the payload's record at PAL 0x0027B100 (NTSC-U 0x0027B200) */
#define RAC3_MADDR_0x0027B100u 0x0027B200u
/* the function's pair: the front end's entry: NTSC-U's record 0x0027B200 starts with the same function (its register saves in another order) */
#define RAC3_MADDR_0x0027B1C0u 0x0027B200u
/* the end of the payload's record at PAL 0x0027B100 (NTSC-U 0x0027B200, 0x257C8 bytes) */
#define RAC3_MADDR_0x002A0A38u 0x002A09C8u

/* Levels' own globals (release.json's level_addresses). */
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 46 of 46 places agree */
#define RAC3_LADDR_12_0x001D66E4u 0x001D66E4u
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 48 of 48 places agree */
#define RAC3_LADDR_19_0x001D66D4u 0x001D66D4u
/* between the same code's 0x001D64E8 and 0x001D64FA, both moved by +0x0 (NTSC-U 0x001D64E8, 0x001D64FA) */
#define RAC3_LADDR_1_0x001D64ECu 0x001D64ECu
/* between the same code's 0x001D64E8 and 0x001D64FA, both moved by +0x0 (NTSC-U 0x001D64E8, 0x001D64FA) */
#define RAC3_LADDR_1_0x001D64F0u 0x001D64F0u
/* between the same code's 0x001D64E8 and 0x001D64FA, both moved by +0x0 (NTSC-U 0x001D64E8, 0x001D64FA) */
#define RAC3_LADDR_1_0x001D64F4u 0x001D64F4u
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 46 of 46 places agree */
#define RAC3_LADDR_1_0x001D6694u 0x001D6694u
/* between the same code's 0x001DBCA0 and 0x001DBCBC, both moved by +0x40 (NTSC-U 0x001DBCE0, 0x001DBCFC) */
#define RAC3_LADDR_1_0x001DBCA4u 0x001DBCE4u
/* the same instruction of the same code: func_L01_00325F60 (NTSC-U func_L01_00325A60) +0x70, 3 of 3 places agree */
#define RAC3_LADDR_1_0x001DCA14u 0x001DCA54u
/* the same instruction of the same code: func_L01_003F1478 (NTSC-U func_L01_003EF538) +0xD0, 24 of 24 places agree */
#define RAC3_LADDR_1_0x001DF948u 0x001DF9C8u
/* the same instruction of the same code: func_L01_004B9810 (NTSC-U func_L01_004B5C78) +0x2C8, 1 of 1 places agree */
#define RAC3_LADDR_1_0x001DF960u 0x001DF9E0u
/* the same instruction of the same code: func_L01_0042D9C0 (NTSC-U func_L01_0042ABB0) +0x68, 2 of 2 places agree */
#define RAC3_LADDR_1_0x001DF97Cu 0x001DF9FCu
/* the same instruction of the same code: func_L01_004208E0 (NTSC-U func_L01_0041E1B0) +0x160, 6 of 6 places agree */
#define RAC3_LADDR_1_0x001DFD0Cu 0x001DFD8Cu
/* the same instruction of the same code: func_L01_004208E0 (NTSC-U func_L01_0041E1B0) +0x8, 1 of 1 places agree */
#define RAC3_LADDR_1_0x001DFD20u 0x001DFDA0u
/* the same instruction of the same code: func_L01_0041C4A0 (NTSC-U func_L01_00419D98) +0x34, 4 of 4 places agree */
#define RAC3_LADDR_1_0x001DFD24u 0x001DFDA4u
/* the same instruction of the same code: func_L01_002FBB10 (NTSC-U func_L01_002FBAE8) +0x58, 185 of 185 places agree */
#define RAC3_LADDR_1_0x001E001Cu 0x001E009Cu
/* the same instruction of the same code: func_L01_0042AF50 (NTSC-U func_L01_004281A8) +0xB8, 19 of 19 places agree */
#define RAC3_LADDR_1_0x001E0020u 0x001E00A0u
/* the same instruction of the same code: func_L01_00438EA0 (NTSC-U func_L01_00435DE0) +0x10, 39 of 39 places agree */
#define RAC3_LADDR_1_0x001E0024u 0x001E00A4u
/* between the same code's 0x001E002C and 0x001E0054, both moved by +0x80 (NTSC-U 0x001E00AC, 0x001E00D4) */
#define RAC3_LADDR_1_0x001E0050u 0x001E00D0u
/* the same instruction of the same code: func_L01_00325F60 (NTSC-U func_L01_00325A60) +0x23C, 21 of 21 places agree */
#define RAC3_LADDR_1_0x001E0100u 0x001E0180u
/* the same instruction of the same code: func_L01_004618E8 (NTSC-U func_L01_0045E440) +0x1C, 5 of 5 places agree */
#define RAC3_LADDR_1_0x001E0278u 0x001E02F8u
/* the same instruction of the same code: func_L01_004618E8 (NTSC-U func_L01_0045E440) +0xC, 8 of 8 places agree */
#define RAC3_LADDR_1_0x001E0280u 0x001E0300u
/* between the same code's 0x001E0D30 and 0x001E0DC0, both moved by +0x80 (NTSC-U 0x001E0DB0, 0x001E0E40) */
#define RAC3_LADDR_1_0x001E0D70u 0x001E0DF0u
/* between the same code's 0x001E0D30 and 0x001E0DC0, both moved by +0x80 (NTSC-U 0x001E0DB0, 0x001E0E40) */
#define RAC3_LADDR_1_0x001E0D80u 0x001E0E00u
/* between the same code's 0x001E0D30 and 0x001E0DC0, both moved by +0x80 (NTSC-U 0x001E0DB0, 0x001E0E40) */
#define RAC3_LADDR_1_0x001E0D90u 0x001E0E10u
/* between the same code's 0x001E0D30 and 0x001E0DC0, both moved by +0x80 (NTSC-U 0x001E0DB0, 0x001E0E40) */
#define RAC3_LADDR_1_0x001E0DB0u 0x001E0E30u
/* between the same code's 0x001E0DF0 and 0x001E0E90, both moved by +0x80 (NTSC-U 0x001E0E70, 0x001E0F10) */
#define RAC3_LADDR_1_0x001E0E10u 0x001E0E90u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x44, 8 of 8 places agree */
#define RAC3_LADDR_1_0x001E1060u 0x001E10E0u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x6C, 96 of 96 places agree */
#define RAC3_LADDR_1_0x001E1068u 0x001E10E8u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x3C, 8 of 8 places agree */
#define RAC3_LADDR_1_0x001E1074u 0x001E10F4u
/* the same instruction of the same code: func_L01_00305D30 (NTSC-U func_L01_00305AD0) +0x68, 18 of 18 places agree */
#define RAC3_LADDR_1_0x001E2880u 0x001E2900u
/* the same instruction of the same code: func_L01_003649E8 (NTSC-U func_L01_00363C60) +0x74, 13 of 13 places agree */
#define RAC3_LADDR_1_0x001E29E0u 0x001E2A60u
/* the same instruction of the same code: func_L01_00313708 (NTSC-U func_L01_00313400) +0x5C, 65 of 65 places agree */
#define RAC3_LADDR_1_0x001E2A40u 0x001E2AC0u
/* the same instruction of the same code: func_L01_003E4428 (NTSC-U func_L01_003E2638) +0x14, 8 of 8 places agree */
#define RAC3_LADDR_1_0x001E2AA0u 0x001E2B20u
/* the same instruction of the same code: func_L01_003E7000 (NTSC-U func_L01_003E5198) +0x44, 5 of 5 places agree */
#define RAC3_LADDR_1_0x001E2B90u 0x001E2C10u
/* the same instruction of the same code: func_L01_00423DF0 (NTSC-U func_L01_004210C0) +0x8, 2 of 2 places agree */
#define RAC3_LADDR_1_0x001E6DB4u 0x001E6E34u
/* the same instruction of the same code: func_L01_00403168 (NTSC-U func_L01_00400DF8) +0xB4, 9 of 9 places agree */
#define RAC3_LADDR_1_0x001E7580u 0x001E7600u
/* the same instruction of the same code: func_L01_0042D9C0 (NTSC-U func_L01_0042ABB0) +0x88, 1 of 1 places agree */
#define RAC3_LADDR_1_0x001E7880u 0x001E7900u
/* the same instruction of the same code: func_L01_0047E908 (NTSC-U func_L01_0047B0A8) +0x124, 13 of 13 places agree */
#define RAC3_LADDR_1_0x001EA7C0u 0x001EA840u
/* between the same code's 0x001EA7C0 and 0x001EA940, both moved by +0x80 (NTSC-U 0x001EA840, 0x001EA9C0) */
#define RAC3_LADDR_1_0x001EA840u 0x001EA8C0u
/* the same instruction of the same code: func_L01_00444F38 (NTSC-U func_L01_00441E00) +0xEC, 2 of 2 places agree */
#define RAC3_LADDR_1_0x001FCB40u 0x001FCBC0u
/* between the same code's 0x00220840 and 0x00220E40, both moved by +0x80 (NTSC-U 0x002208C0, 0x00220EC0) */
#define RAC3_LADDR_1_0x00220A40u 0x00220AC0u
/* the same instruction of the same code: func_L01_00305880 (NTSC-U func_L01_00305620) +0x24, 133 of 133 places agree */
#define RAC3_LADDR_1_0x002C5600u 0x002C5640u
/* the same instruction of the same code: func_L01_0041BFE8 (NTSC-U func_L01_004198E0) +0xC, 3 of 3 places agree */
#define RAC3_LADDR_1_0x002E60D0u 0x002E6110u
/* the entry of the draw callback list runner itself (host/wide_view.c reads its tables from its code): PAL func_L01_00404858's twin is NTSC-U's func_L01_004024E8, the pair rac3-ntsc's carried libraries.tsv binds */
#define RAC3_LADDR_1_0x00404858u 0x004024E8u
/* the entry of the draw callback list runner itself (host/wide_view.c reads its tables from its code): PAL func_L01_00404920's twin is NTSC-U's func_L01_004025B0, the pair rac3-ntsc's carried libraries.tsv binds */
#define RAC3_LADDR_1_0x00404920u 0x004025B0u
/* the entry of the draw callback list runner itself (host/wide_view.c reads its tables from its code): PAL func_L01_00404998's twin is NTSC-U's func_L01_00402628, the pair rac3-ntsc's carried libraries.tsv binds */
#define RAC3_LADDR_1_0x00404998u 0x00402628u
/* the entry of the draw callback list runner itself (host/wide_view.c reads its tables from its code): PAL func_L01_00404A60's twin is NTSC-U's func_L01_004026F0, the pair rac3-ntsc's carried libraries.tsv binds */
#define RAC3_LADDR_1_0x00404A60u 0x004026F0u
/* the entry of the draw callback list runner itself (host/wide_view.c reads its tables from its code): PAL func_L01_00404B28's twin is NTSC-U's func_L01_004027B8, the pair rac3-ntsc's carried libraries.tsv binds */
#define RAC3_LADDR_1_0x00404B28u 0x004027B8u
/* not paired (rac3_level_pairs.py) */
#define RAC3_LADDR_1_0x004D8220u 0u
/* not paired (rac3_level_pairs.py) */
#define RAC3_LADDR_1_0x004DB780u 0u
/* the same instruction of the same code: func_L01_003F1E98 (NTSC-U func_L01_003EFF58) +0x20, 4 of 4 places agree */
#define RAC3_LADDR_2_0x001D5477u 0x001D5477u
/* between the same code's 0x001DC518 and 0x001DC570, both moved by +0x40 (NTSC-U 0x001DC558, 0x001DC5B0) */
#define RAC3_LADDR_2_0x001DC568u 0x001DC5A8u
/* the same instruction of the same code: func_L01_0032FF68 (NTSC-U func_L01_0032FA40) +0x8, 47 of 47 places agree */
#define RAC3_LADDR_2_0x001DE4C0u 0x001DE540u
/* the same instruction of the same code: func_L02_003102A8 (NTSC-U func_L02_0030FF70) +0x10, 5 of 5 places agree */
#define RAC3_LADDR_2_0x001E00D4u 0x001E0154u
/* the same instruction of the same code: func_L02_0030FC68 (NTSC-U func_L02_0030F930) +0x280, 1 of 1 places agree */
#define RAC3_LADDR_2_0x001E00DCu 0x001E015Cu
/* the same instruction of the same code: func_L02_00310368 (NTSC-U func_L02_00310030) +0x84, 1 of 1 places agree */
#define RAC3_LADDR_2_0x001E00E0u 0x001E0160u
/* the same instruction of the same code: func_L02_00310368 (NTSC-U func_L02_00310030) +0x78, 1 of 1 places agree */
#define RAC3_LADDR_2_0x001E00E4u 0x001E0164u
/* the same instruction of the same code: func_L02_00310368 (NTSC-U func_L02_00310030) +0xB4, 1 of 1 places agree */
#define RAC3_LADDR_2_0x001E00E8u 0x001E0168u
/* the same instruction of the same code: func_L02_00310368 (NTSC-U func_L02_00310030) +0xAC, 1 of 1 places agree */
#define RAC3_LADDR_2_0x001E00ECu 0x001E016Cu
/* the same instruction of the same code: func_L01_00305D30 (NTSC-U func_L01_00305AD0) +0x68, 21 of 21 places agree */
#define RAC3_LADDR_2_0x001E1140u 0x001E11C0u
/* the same instruction of the same code: func_L01_00403168 (NTSC-U func_L01_00400DF8) +0xB4, 13 of 13 places agree */
#define RAC3_LADDR_2_0x001E5E40u 0x001E5EC0u
/* the same instruction of the same code: func_L02_0030FC68 (NTSC-U func_L02_0030F930) +0x1C, 3 of 3 places agree */
#define RAC3_LADDR_2_0x0026EB80u 0x0026EC00u
/* between the same code's 0x00245BB0 and 0x00245BCC, both moved by +0x180 (NTSC-U 0x00245D30, 0x00245D4C) */
#define RAC3_LADDR_39_0x00245BB4u 0x00245D34u
/* the same instruction of the same code: func_L39_00589538 (NTSC-U func_L39_005883C8) +0x40, 6 of 6 places agree */
#define RAC3_LADDR_39_0x0024B9FCu 0x0024BAFCu
/* between the same code's 0x0024BCE4 and 0x0024BCF8, both moved by +0x100 (NTSC-U 0x0024BDE4, 0x0024BDF8) */
#define RAC3_LADDR_39_0x0024BCECu 0x0024BDECu
/* the same instruction of the same code: func_L39_0059C910 (NTSC-U func_L39_0059B580) +0x4, 2 of 2 places agree */
#define RAC3_LADDR_39_0x0024BD00u 0x0024BE00u
/* the same instruction of the same code: func_L39_00589538 (NTSC-U func_L39_005883C8) +0x9C, 5 of 5 places agree */
#define RAC3_LADDR_39_0x0024BD04u 0x0024BE04u
/* the same instruction of the same code: func_L39_005B25C8 (NTSC-U func_L39_005B06C8) +0x1B8, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0024BF90u 0x0024C090u
/* the same instruction of the same code: func_L01_004C97C8 (NTSC-U func_L01_004C5C08) +0x28, 4 of 4 places agree */
#define RAC3_LADDR_39_0x0024C188u 0x0024C288u
/* the same instruction of the same code: func_L01_004C97C8 (NTSC-U func_L01_004C5C08) +0x8, 6 of 6 places agree */
#define RAC3_LADDR_39_0x0024C190u 0x0024C290u
/* between the same code's 0x0024C890 and 0x0024C8F0, both moved by +0x100 (NTSC-U 0x0024C990, 0x0024C9F0) */
#define RAC3_LADDR_39_0x0024C8A0u 0x0024C9A0u
/* between the same code's 0x0024C890 and 0x0024C8F0, both moved by +0x100 (NTSC-U 0x0024C990, 0x0024C9F0) */
#define RAC3_LADDR_39_0x0024C8B0u 0x0024C9B0u
/* between the same code's 0x0024C890 and 0x0024C8F0, both moved by +0x100 (NTSC-U 0x0024C990, 0x0024C9F0) */
#define RAC3_LADDR_39_0x0024C8C0u 0x0024C9C0u
/* between the same code's 0x0024C890 and 0x0024C8F0, both moved by +0x100 (NTSC-U 0x0024C990, 0x0024C9F0) */
#define RAC3_LADDR_39_0x0024C8E0u 0x0024C9E0u
/* between the same code's 0x0024C920 and 0x0024C9A0, both moved by +0x100 (NTSC-U 0x0024CA20, 0x0024CAA0) */
#define RAC3_LADDR_39_0x0024C940u 0x0024CA40u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x6C, 95 of 95 places agree */
#define RAC3_LADDR_39_0x0024CAD0u 0x0024CBD0u
/* the same instruction of the same code: func_L39_0067A4E0 (NTSC-U func_L39_00677AE8) +0x18, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0024DE20u 0x0024DF20u
/* the same instruction of the same code: func_L01_00403168 (NTSC-U func_L01_00400DF8) +0xB4, 10 of 10 places agree */
#define RAC3_LADDR_39_0x0025D6C0u 0x0025D7C0u
/* the same instruction of the same code: func_L39_0059C910 (NTSC-U func_L39_0059B580) +0x60, 3 of 3 places agree */
#define RAC3_LADDR_39_0x0025D9C0u 0x0025DAC0u
/* the same instruction of the same code: func_L39_005B25C8 (NTSC-U func_L39_005B06C8) +0xE8, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0025F580u 0x0025F680u
/* the same instruction of the same code: func_L01_00444F38 (NTSC-U func_L01_00441E00) +0xEC, 2 of 2 places agree */
#define RAC3_LADDR_39_0x00271500u 0x00271600u
/* between the same code's 0x00294FC0 and 0x002965C0, both moved by +0x100 (NTSC-U 0x002950C0, 0x002966C0) */
#define RAC3_LADDR_39_0x002961C0u 0x002962C0u
/* the level program's entry (its ELF header: PAL 0x00575CC8, NTSC-U 0x00574F88) */
#define RAC3_LADDR_39_0x00575CC8u 0x00574F88u

#endif /* OPENRAC_RAC3_ADDRESSES_H */
