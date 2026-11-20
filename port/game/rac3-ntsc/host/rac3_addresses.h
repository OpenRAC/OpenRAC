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
/* D_001D4B50 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4B50u 0x001D4B50u
/* D_001D4BD0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BD0u 0x001D4BD0u
/* D_001D4BD4 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BD4u 0x001D4BD4u
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
/* D_001D5688 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5688u 0x001D5688u
/* D_001D568C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D568Cu 0x001D568Cu
/* D_001D5690 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5690u 0x001D5690u
/* D_001D5694 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5694u 0x001D5694u
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
/* D_001D9D04 (names.tsv) */
#define RAC3_ADDR_0x001D9D04u 0x001D9D84u
/* D_001D9EA8 (names.tsv) */
#define RAC3_ADDR_0x001D9EA8u 0x001D9F28u
/* D_001D9EAC (names.tsv) */
#define RAC3_ADDR_0x001D9EACu 0x001D9F2Cu
/* D_001DA050 (names.tsv) */
#define RAC3_ADDR_0x001DA050u 0x001DA0D0u
/* D_001DA058 (names.tsv) */
#define RAC3_ADDR_0x001DA058u 0x001DA0D8u
/* D_0022758C (names.tsv) */
#define RAC3_ADDR_0x0022758Cu 0x0022760Cu
/* D_00280CF0 (names.tsv) */
#define RAC3_ADDR_0x00280CF0u 0x00280D70u
/* D_0037F830 (names.tsv) */
#define RAC3_ADDR_0x0037F830u 0x0037F8B0u
/* func_00381250 (map.tsv differs) */
#define RAC3_ADDR_0x00381250u 0x00381280u

/* Levels' own globals (release.json's level_addresses). */
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 46 of 46 places agree */
#define RAC3_LADDR_12_0x001D66E4u 0x001D66E4u
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 48 of 48 places agree */
#define RAC3_LADDR_19_0x001D66D4u 0x001D66D4u
/* the same instruction of the same code: func_L01_003424B0 (NTSC-U func_L01_00341C10) +0x8, 46 of 46 places agree */
#define RAC3_LADDR_1_0x001D6694u 0x001D6694u
/* between the same code's 0x001DBCA0 and 0x001DBCBC, both moved by +0x40 (NTSC-U 0x001DBCE0, 0x001DBCFC) */
#define RAC3_LADDR_1_0x001DBCA4u 0x001DBCE4u
/* the same instruction of the same code: func_L01_00325F60 (NTSC-U func_L01_00325A60) +0x70, 3 of 3 places agree */
#define RAC3_LADDR_1_0x001DCA14u 0x001DCA54u
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
/* between the same code's 0x001E0DF0 and 0x001E0E90, both moved by +0x80 (NTSC-U 0x001E0E70, 0x001E0F10) */
#define RAC3_LADDR_1_0x001E0E10u 0x001E0E90u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x6C, 96 of 96 places agree */
#define RAC3_LADDR_1_0x001E1068u 0x001E10E8u
/* the same instruction of the same code: func_L01_003649E8 (NTSC-U func_L01_00363C60) +0x74, 13 of 13 places agree */
#define RAC3_LADDR_1_0x001E29E0u 0x001E2A60u
/* the same instruction of the same code: func_L01_003E4428 (NTSC-U func_L01_003E2638) +0x14, 8 of 8 places agree */
#define RAC3_LADDR_1_0x001E2AA0u 0x001E2B20u
/* the same instruction of the same code: func_L01_003E7000 (NTSC-U func_L01_003E5198) +0x44, 5 of 5 places agree */
#define RAC3_LADDR_1_0x001E2B90u 0x001E2C10u
/* the same instruction of the same code: func_L01_00403168 (NTSC-U func_L01_00400DF8) +0xB4, 10 of 10 places agree */
#define RAC3_LADDR_1_0x001E7580u 0x001E7600u
/* the same instruction of the same code: func_L01_0042D9C0 (NTSC-U func_L01_0042ABB0) +0x88, 1 of 1 places agree */
#define RAC3_LADDR_1_0x001E7880u 0x001E7900u
/* between the same code's 0x001EA7C0 and 0x001EA940, both moved by +0x80 (NTSC-U 0x001EA840, 0x001EA9C0) */
#define RAC3_LADDR_1_0x001EA840u 0x001EA8C0u
/* between the same code's 0x00220840 and 0x00220E40, both moved by +0x80 (NTSC-U 0x002208C0, 0x00220EC0) */
#define RAC3_LADDR_1_0x00220A40u 0x00220AC0u
/* the same instruction of the same code: func_L01_0041BFE8 (NTSC-U func_L01_004198E0) +0xC, 3 of 3 places agree */
#define RAC3_LADDR_1_0x002E60D0u 0x002E6110u
/* the same instruction of the same code: func_L39_005B25C8 (NTSC-U func_L39_005B06C8) +0x1B8, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0024BF90u 0x0024C090u
/* the same instruction of the same code: func_L01_003F8820 (NTSC-U func_L01_003F6678) +0x6C, 95 of 95 places agree */
#define RAC3_LADDR_39_0x0024CAD0u 0x0024CBD0u
/* the same instruction of the same code: func_L39_0067A4E0 (NTSC-U func_L39_00677AE8) +0x18, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0024DE20u 0x0024DF20u
/* the same instruction of the same code: func_L01_00403168 (NTSC-U func_L01_00400DF8) +0xB4, 10 of 10 places agree */
#define RAC3_LADDR_39_0x0025D6C0u 0x0025D7C0u
/* the same instruction of the same code: func_L39_005B25C8 (NTSC-U func_L39_005B06C8) +0xE8, 1 of 1 places agree */
#define RAC3_LADDR_39_0x0025F580u 0x0025F680u

#endif /* OPENRAC_RAC3_ADDRESSES_H */
