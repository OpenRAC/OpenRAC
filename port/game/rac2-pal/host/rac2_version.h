/* Written by port/tools/hostgen/release_carry.py from the shared host C and this release's maps. Do not edit. */
/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * PAL (SCES-51607): its names and addresses for the shared host C (port/game/rac2/host/rac2.h), v1.01's
 * names and addresses as this release has them. Written by port/tools/hostgen/release_carry.py
 * from the shared C, the release table and versionmap's map; do not edit. */
#ifndef OPENRAC_RAC2_VERSION_H
#define OPENRAC_RAC2_VERSION_H

#define RAC2_ADDR(a) RAC2_ADDR_##a
#define RAC2_LADDR(n, a) RAC2_LADDR_##n##_##a
#define RAC2_K(name, value, where) RAC2_K_##name

#define RAC2_LEVEL_BASE 0x001A7D00u /* a level's program replaces the executable's memory from here */
#define RAC2_GP 0x001AF070u         /* the executable's $gp */
#define RAC2_EXE_SIZE 0x00255000u   /* the executable's loaded bytes from 0x00100000, to a page */

#include "rac2_addresses.h"


/* Addresses of the executable's memory. */
#define RAC2_ADDR_0x0010E9F0u 0x0010E9F0u /* the same microcode at the same place */
#define RAC2_ADDR_0x00138180u 0x00138200u /* the release's build */
#define RAC2_ADDR_0x00138280u 0x00138300u /* the release's build */
#define RAC2_ADDR_0x00146B40u 0x00146BC0u /* the same instruction of the paired function */
#define RAC2_ADDR_0x00146B44u 0x00146BC4u /* versionmap: between */
#define RAC2_ADDR_0x00152C26u 0x00152CA6u /* the release's build */
#define RAC2_ADDR_0x00152C64u 0x00152CE4u /* versionmap: between */
#define RAC2_ADDR_0x00152C68u 0x00152CE8u /* versionmap: between */
#define RAC2_ADDR_0x001A7350u 0x001A73D0u /* the release's build */
#define RAC2_ADDR_0x001A7354u 0x001A73D4u /* the release's build */
#define RAC2_ADDR_0x001A79F0u 0x001A7A78u /* the release's build */
#define RAC2_ADDR_0x001A7B98u 0x001A7C20u /* the release's build */
#define RAC2_ADDR_0x001A7BA4u 0x001A7C2Cu /* the release's build */
#define RAC2_ADDR_0x001A7BA8u 0x001A7C30u /* the release's build */
#define RAC2_ADDR_0x001A7BACu 0x001A7C34u /* the release's build */
#define RAC2_ADDR_0x001A7BB1u 0x001A7C39u /* the release's build */
#define RAC2_ADDR_0x001A7BB4u 0x001A7C3Cu /* the release's build */
#define RAC2_ADDR_0x001A8C60u 0x001A8D10u /* the release's build */
#define RAC2_ADDR_0x001A8C64u 0x001A8D14u /* the release's build */
#define RAC2_ADDR_0x001A8CFCu 0x001A8DACu /* the release's build */
#define RAC2_ADDR_0x001AA0F8u 0x001AA178u /* the release's build */
#define RAC2_ADDR_0x001ADE00u 0x001ADE78u /* the release's build */
#define RAC2_ADDR_0x001ADE20u 0x001ADE98u /* the release's build */
#define RAC2_ADDR_0x001ADE28u 0x001ADEA0u /* the release's build */
#define RAC2_ADDR_0x001AE0E0u 0x001AE158u /* the release's build */
#define RAC2_ADDR_0x001AE0F4u 0x001AE16Cu /* the release's build */
#define RAC2_ADDR_0x001AE120u 0x001AE198u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE124u 0x001AE19Cu /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE128u 0x001AE1A0u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE138u 0x001AE1B0u /* the release's build */
#define RAC2_ADDR_0x001B15A0u 0x001B15A0u /* versionmap: exact */
#define RAC2_ADDR_0x001B15FCu 0x001B15FCu /* versionmap: exact */
#define RAC2_ADDR_0x001B1608u 0x001B1608u /* versionmap: exact */
#define RAC2_ADDR_0x001B160Cu 0x001B160Cu /* versionmap: exact */
#define RAC2_ADDR_0x001B1610u 0x001B1610u /* versionmap: exact */
#define RAC2_ADDR_0x001B1614u 0x001B1614u /* versionmap: exact */
#define RAC2_ADDR_0x001B1618u 0x001B1618u /* versionmap: exact */
#define RAC2_ADDR_0x001B1628u 0x001B1628u /* versionmap: exact */
#define RAC2_ADDR_0x001B1638u 0x001B1638u /* versionmap: exact */
#define RAC2_ADDR_0x001B187Cu 0x001B187Cu /* versionmap: exact */
#define RAC2_ADDR_0x001B1890u 0x001B1890u /* versionmap: exact */
#define RAC2_ADDR_0x001B1894u 0x001B1894u /* versionmap: exact */
#define RAC2_ADDR_0x001B1B74u 0x001B1B74u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001B1D48u 0x001B1D48u /* versionmap: exact */
#define RAC2_ADDR_0x001B1D90u 0x001B1D90u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001B1D98u 0x001B1D98u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001B20F0u 0x001B20F0u /* versionmap: exact */
#define RAC2_ADDR_0x001B22A8u 0x001B22A8u /* versionmap: exact */
#define RAC2_ADDR_0x001B5200u 0x001B5200u /* versionmap: exact */
#define RAC2_ADDR_0x001B5570u 0x001B5570u /* versionmap: exact */
#define RAC2_ADDR_0x001B8840u 0x001B8840u /* versionmap: exact */
#define RAC2_ADDR_0x001B9040u 0x001B9040u /* versionmap: exact */
#define RAC2_ADDR_0x001B9340u 0x001B9340u /* versionmap: exact */
#define RAC2_ADDR_0x001B9E40u 0x001B9E40u /* versionmap: exact */
#define RAC2_ADDR_0x001B9F40u 0x001B9F40u /* versionmap: exact */
#define RAC2_ADDR_0x001BA040u 0x001BA040u /* versionmap: exact */
#define RAC2_ADDR_0x001BA140u 0x001BA140u /* versionmap: exact */
#define RAC2_ADDR_0x001BA240u 0x001BA240u /* versionmap: exact */
#define RAC2_ADDR_0x001BA340u 0x001BA340u /* versionmap: exact */
#define RAC2_ADDR_0x001BA440u 0x001BA440u /* versionmap: exact */
#define RAC2_ADDR_0x001BA540u 0x001BA540u /* versionmap: exact */
#define RAC2_ADDR_0x001BADC0u 0x001BADC0u /* versionmap: exact */
#define RAC2_ADDR_0x001C2740u 0x001C2740u /* versionmap: exact */
#define RAC2_ADDR_0x001F1F40u 0x001F1F40u /* versionmap: exact */
#define RAC2_ADDR_0x001F2840u 0x001F2840u /* versionmap: exact */
#define RAC2_ADDR_0x00255330u 0x00255330u /* versionmap: exact */
#define RAC2_ADDR_0x00261590u 0x00261330u /* the same instruction of the paired function */
#define RAC2_ADDR_0x00263B50u 0x002638D0u /* the release's build */
#define RAC2_ADDR_0x00263EF0u 0x00263C70u /* the release's build */
#define RAC2_ADDR_0x00264290u 0x00264010u /* the release's build */
#define RAC2_ADDR_0x0026EDE8u 0x0026EC50u /* code */
#define RAC2_ADDR_0x002DC908u 0x002DD5F8u /* code */

/* Addresses of a level's program. */
#define RAC2_LADDR_00_0x001B14ACu 0x001B14BCu /* the release's build */
#define RAC2_LADDR_00_0x001B14B0u 0x001B14C0u /* the release's build */
#define RAC2_LADDR_00_0x001B8E10u 0x001B8E50u /* the release's build */
#define RAC2_LADDR_00_0x001B8E20u 0x001B8E60u /* the release's build */
/* Constants of the game's code (RAC2_K): the same instruction's in this release. */
#define RAC2_K_CAMERA_CURSOR 0x2D8 /* v1.01 0x2A8; func_0033C738 as func_0033E608: 4 instructions */
#define RAC2_K_HELP_0 0xBB7 /* v1.01 0x2DA6; word 0 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_1 0xBB8 /* v1.01 0x2DA7; word 1 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_2 0xBB9 /* v1.01 0x2DA8; word 2 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_3 0xB7D /* v1.01 0x2D6C; word 3 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_4 0xBBB /* v1.01 0x2DAA; word 4 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_5 0xBBC /* v1.01 0x2DAB; word 5 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_6 0xBBF /* v1.01 0x2DAC; word 6 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_7 0x0 /* v1.01 0x2DAE; word 7 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_8 0xC2C /* v1.01 0x2E1C; word 8 of the table at 0x00265030 (v1.01's 0x002652B0) */
#define RAC2_K_LIST_CURSOR 0x130 /* v1.01 0x130; func_00342118 as func_00344180: 1 instructions */
#define RAC2_K_LIST_ICON 0x17C /* v1.01 0x17C; func_00341A88 as func_00343AF0: 1 instructions */
#define RAC2_K_LIST_ICON_SHEET 0x1C4 /* v1.01 0x1C4; func_00341A88 as func_00343AF0: 2 instructions */
#define RAC2_K_LIST_ON 0x230 /* v1.01 0x220; func_00341C20 as func_00343C88: 1 instructions */
#define RAC2_K_LIST_ORIGIN 0x228 /* v1.01 0x218; func_00341A88 as func_00343AF0: 2 instructions */
#define RAC2_K_LIST_SPRITE_1 0x4C /* v1.01 0x4C; func_00341C20 as func_00343C88: 1 instructions */
#define RAC2_K_LIST_SPRITE_2 0xE4 /* v1.01 0xE4; func_00341C20 as func_00343C88: 1 instructions */
#define RAC2_K_LIST_TITLE_BAR 0x98 /* v1.01 0x98; func_00341C20 as func_00343C88: 1 instructions */
#define RAC2_K_LIST_TITLE_TEXT 0x1D0 /* v1.01 0x1D0; func_00341C20 as func_00343C88: 1 instructions */
#define RAC2_K_OPTIONS_LIST_OFFSET 0x3E810 /* v1.01 0x3E1D8; func_002D58A0 as func_002D6478: 1 instructions */
#define RAC2_K_PAUSE_LIST_COUNT 0x1CC /* v1.01 0x1CC; by hand: func_00341610 as func_00343690: sw $a0, 0x1CC($s0) at +0x7c, as v1.01's at +0x74 (the list's count) */
#define RAC2_K_PAUSE_LIST_CURSORS 0x1B8 /* v1.01 0x1B8; func_00341610 as func_00343690: 1 instructions */
#define RAC2_K_PAUSE_LIST_ICONS 0x1C8 /* v1.01 0x1C8; func_00341610 as func_00343690: 1 instructions */
#define RAC2_K_PAUSE_LIST_OFFSET 0x3C210 /* v1.01 0x3BE08; func_0029C4C0 as func_0029CE58: 1 instructions */
#define RAC2_K_PAUSE_LIST_WHICH 0x31C /* v1.01 0x2FC; func_00341610 as func_00343690: 1 instructions */
#define RAC2_K_PAUSE_OPTIONS_ROWS 0x7 /* v1.01 0x8; by hand: func_00341610 as func_00343690: addiu $v1, $zero, 0x7 at +0x5c for v1.01's 0x8 (the Options list's rows, list 1) */
#define RAC2_K_TEXT_CAMERA_ROW 0xBA9 /* v1.01 0x2D98; func_0033C468 as func_0033E338: 1 instructions */
#define RAC2_K_TEXT_CAMERA_TITLE 0xBA9 /* v1.01 0x2D98; func_0033C468 as func_0033E338: 1 instructions */
#define RAC2_K_TEXT_LEFT_RIGHT 0xBF2 /* v1.01 0x2DE1; func_0033C738 as func_0033E608: 1 instructions */
#define RAC2_K_TEXT_PAGE_EXIT 0xB60 /* v1.01 0x2D4F; func_0033C468 as func_0033E338: 1 instructions */
#define RAC2_K_TEXT_PAGE_TOGGLE 0xB5F /* v1.01 0x2D4E; func_0033C468 as func_0033E338: 1 instructions */
#define RAC2_K_TEXT_SPEED 0xE3D /* v1.01 0x2F18; func_0033C738 as func_0033E608: 1 instructions */
#define RAC2_K_TEXT_UP_DOWN 0xBF3 /* v1.01 0x2DE2; func_0033C738 as func_0033E608: 1 instructions */
#define RAC2_K_TEXT_WIDESCREEN 0x10FC /* v1.01 0x31E4; func_0033C738 as func_0033E608: 1 instructions */
#define RAC2_K_TITLE_ICON_SHEET 0xEAA6 /* v1.01 0xEAA6; func_00342118 as func_00344180: 1 instructions */

#endif /* OPENRAC_RAC2_VERSION_H */
