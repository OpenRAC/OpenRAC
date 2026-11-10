/* Written by port/tools/hostgen/release_carry.py from the shared host C and this release's maps. Do not edit. */
/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * USA v2.00 (SCUS-97268, Greatest Hits): its names and addresses for the shared host C (port/game/rac2/host/rac2.h), v1.01's
 * names and addresses as this release has them. Written by port/tools/hostgen/release_carry.py
 * from the shared C, the release table and versionmap's map; do not edit. */
#ifndef OPENRAC_RAC2_VERSION_H
#define OPENRAC_RAC2_VERSION_H

#define RAC2_ADDR(a) RAC2_ADDR_##a
#define RAC2_LADDR(n, a) RAC2_LADDR_##n##_##a
#define RAC2_K(name, value, where) RAC2_K_##name

#define RAC2_LEVEL_BASE 0x001A7C80u /* a level's program replaces the executable's memory from here */
#define RAC2_GP 0x001AEFF0u         /* the executable's $gp */
#define RAC2_EXE_SIZE 0x00253000u   /* the executable's loaded bytes from 0x00100000, to a page */

#include "rac2_addresses.h"


/* Addresses of the executable's memory. */
#define RAC2_ADDR_0x0010E9F0u 0x0010E9F0u /* the same microcode at the same place */
#define RAC2_ADDR_0x00138180u 0x00138180u /* versionmap: exact */
#define RAC2_ADDR_0x00138280u 0x00138280u /* versionmap: between */
#define RAC2_ADDR_0x00146B40u 0x00146B40u /* the same instruction of the paired function */
#define RAC2_ADDR_0x00146B44u 0x00146B44u /* versionmap: between */
#define RAC2_ADDR_0x00152C26u 0x00152C26u /* versionmap: between */
#define RAC2_ADDR_0x00152C64u 0x00152C64u /* versionmap: between */
#define RAC2_ADDR_0x00152C68u 0x00152C68u /* versionmap: between */
#define RAC2_ADDR_0x001A7350u 0x001A7350u /* versionmap: exact */
#define RAC2_ADDR_0x001A7354u 0x001A7354u /* versionmap: exact */
#define RAC2_ADDR_0x001A79F0u 0x001A79F8u /* the release's build */
#define RAC2_ADDR_0x001A7B98u 0x001A7BA0u /* the release's build */
#define RAC2_ADDR_0x001A7BA4u 0x001A7BACu /* the release's build */
#define RAC2_ADDR_0x001A7BA8u 0x001A7BB0u /* the release's build */
#define RAC2_ADDR_0x001A7BACu 0x001A7BB4u /* the release's build */
#define RAC2_ADDR_0x001A7BB1u 0x001A7BB9u /* the release's build */
#define RAC2_ADDR_0x001A7BB4u 0x001A7BBCu /* the release's build */
#define RAC2_ADDR_0x001A8C60u 0x001A8C60u /* versionmap: exact */
#define RAC2_ADDR_0x001A8C64u 0x001A8C64u /* versionmap: exact */
#define RAC2_ADDR_0x001A8CFCu 0x001A8D04u /* the release's build */
#define RAC2_ADDR_0x001AA0F8u 0x001AA0F8u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001ADE00u 0x001ADDD8u /* the release's build */
#define RAC2_ADDR_0x001ADE20u 0x001ADDF8u /* the release's build */
#define RAC2_ADDR_0x001ADE28u 0x001ADE00u /* the release's build */
#define RAC2_ADDR_0x001AE0E0u 0x001AE0B8u /* the release's build */
#define RAC2_ADDR_0x001AE0F4u 0x001AE0CCu /* the release's build */
#define RAC2_ADDR_0x001AE120u 0x001AE0F8u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE124u 0x001AE0FCu /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE128u 0x001AE100u /* the same instruction of the paired function */
#define RAC2_ADDR_0x001AE138u 0x001AE110u /* the release's build */
#define RAC2_ADDR_0x001B15A0u 0x001B1520u /* the release's build */
#define RAC2_ADDR_0x001B15FCu 0x001B157Cu /* the release's build */
#define RAC2_ADDR_0x001B1608u 0x001B1588u /* the release's build */
#define RAC2_ADDR_0x001B160Cu 0x001B158Cu /* the release's build */
#define RAC2_ADDR_0x001B1610u 0x001B1590u /* the release's build */
#define RAC2_ADDR_0x001B1614u 0x001B1594u /* the release's build */
#define RAC2_ADDR_0x001B1618u 0x001B1598u /* the release's build */
#define RAC2_ADDR_0x001B1628u 0x001B15A8u /* the release's build */
#define RAC2_ADDR_0x001B1638u 0x001B15B8u /* the release's build */
#define RAC2_ADDR_0x001B187Cu 0x001B17FCu /* the release's build */
#define RAC2_ADDR_0x001B1890u 0x001B1810u /* the release's build */
#define RAC2_ADDR_0x001B1894u 0x001B1814u /* the release's build */
#define RAC2_ADDR_0x001B1B74u 0x001B1AF4u /* the release's build */
#define RAC2_ADDR_0x001B1D48u 0x001B1CC8u /* the release's build */
#define RAC2_ADDR_0x001B1D90u 0x001B1D10u /* the release's build */
#define RAC2_ADDR_0x001B1D98u 0x001B1D18u /* the release's build */
#define RAC2_ADDR_0x001B20F0u 0x001B2070u /* the release's build */
#define RAC2_ADDR_0x001B22A8u 0x001B2228u /* the release's build */
#define RAC2_ADDR_0x001B5200u 0x001B5180u /* the release's build */
#define RAC2_ADDR_0x001B5570u 0x001B54F0u /* the release's build */
#define RAC2_ADDR_0x001B8840u 0x001B87C0u /* the release's build */
#define RAC2_ADDR_0x001B9040u 0x001B8FC0u /* the release's build */
#define RAC2_ADDR_0x001B9340u 0x001B92C0u /* the release's build */
#define RAC2_ADDR_0x001B9E40u 0x001B9DC0u /* the release's build */
#define RAC2_ADDR_0x001B9F40u 0x001B9EC0u /* the release's build */
#define RAC2_ADDR_0x001BA040u 0x001B9FC0u /* the release's build */
#define RAC2_ADDR_0x001BA140u 0x001BA0C0u /* the release's build */
#define RAC2_ADDR_0x001BA240u 0x001BA1C0u /* the release's build */
#define RAC2_ADDR_0x001BA340u 0x001BA2C0u /* the release's build */
#define RAC2_ADDR_0x001BA440u 0x001BA3C0u /* the release's build */
#define RAC2_ADDR_0x001BA540u 0x001BA4C0u /* the release's build */
#define RAC2_ADDR_0x001BADC0u 0x001BAD40u /* the release's build */
#define RAC2_ADDR_0x001C2740u 0x001C26C0u /* the release's build */
#define RAC2_ADDR_0x001F1F40u 0x001F1EC0u /* the release's build */
#define RAC2_ADDR_0x001F2840u 0x001F27C0u /* the release's build */
#define RAC2_ADDR_0x00255330u 0x002552B0u /* the release's build */
#define RAC2_ADDR_0x00261540u 0x00261520u /* the release's build */
#define RAC2_ADDR_0x00261590u 0x00261570u /* the release's build */
#define RAC2_ADDR_0x00263B50u 0x00263B10u /* the release's build */
#define RAC2_ADDR_0x00263EF0u 0x00263EB0u /* the release's build */
#define RAC2_ADDR_0x00264290u 0x00264250u /* the release's build */
#define RAC2_ADDR_0x002652B0u 0x00265270u /* the help table of the RAC2_K HELP_ rows */
#define RAC2_ADDR_0x0026EDE8u 0x0026EDE8u /* code */
#define RAC2_ADDR_0x002DC908u 0x002DD630u /* code */

/* Addresses of a level's program. */
#define RAC2_LADDR_00_0x001B14ACu 0x001B144Cu /* the release's build */
#define RAC2_LADDR_00_0x001B14B0u 0x001B1450u /* the release's build */
#define RAC2_LADDR_00_0x001B8E10u 0x001B8D90u /* the release's build */
#define RAC2_LADDR_00_0x001B8E20u 0x001B8DA0u /* the release's build */
/* Constants of the game's code (RAC2_K): the same instruction's in this release. */
#define RAC2_K_CAMERA_CURSOR 0x2D8 /* v1.01 0x2A8; func_0033C738 as func_0033D780: 4 instructions */
#define RAC2_K_HELP_0 0x2C3C /* v1.01 0x2DA6; word 0 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_1 0x2C3D /* v1.01 0x2DA7; word 1 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_2 0x2C3E /* v1.01 0x2DA8; word 2 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_3 0x2C02 /* v1.01 0x2D6C; word 3 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_4 0x2C40 /* v1.01 0x2DAA; word 4 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_5 0x2C41 /* v1.01 0x2DAB; word 5 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_6 0x2C42 /* v1.01 0x2DAC; word 6 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_7 0x2C44 /* v1.01 0x2DAE; word 7 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_HELP_8 0x2CB2 /* v1.01 0x2E1C; word 8 of the table at 0x00265270 (v1.01's 0x002652B0) */
#define RAC2_K_LIST_CURSOR 0x130 /* v1.01 0x130; func_00342118 as func_00343028: 1 instructions */
#define RAC2_K_LIST_ICON 0x17C /* v1.01 0x17C; func_00341A88 as func_00342998: 1 instructions */
#define RAC2_K_LIST_ICON_SHEET 0x1C4 /* v1.01 0x1C4; func_00341A88 as func_00342998: 2 instructions */
#define RAC2_K_LIST_ON 0x230 /* v1.01 0x220; func_00341C20 as func_00342B30: 1 instructions */
#define RAC2_K_LIST_ORIGIN 0x228 /* v1.01 0x218; func_00341A88 as func_00342998: 2 instructions */
#define RAC2_K_LIST_SPRITE_1 0x4C /* v1.01 0x4C; func_00341C20 as func_00342B30: 1 instructions */
#define RAC2_K_LIST_SPRITE_2 0xE4 /* v1.01 0xE4; func_00341C20 as func_00342B30: 1 instructions */
#define RAC2_K_LIST_TITLE_BAR 0x98 /* v1.01 0x98; func_00341C20 as func_00342B30: 1 instructions */
#define RAC2_K_LIST_TITLE_TEXT 0x1D0 /* v1.01 0x1D0; func_00341C20 as func_00342B30: 1 instructions */
#define RAC2_K_MAIN_LIST_OFFSET 0x3E9C8 /* v1.01 0x3E430; func_002D5748 as func_002D63B0: 2 instructions */
#define RAC2_K_OPTIONS_LIST_OFFSET 0x3E760 /* v1.01 0x3E1D8; func_002D58A0 as func_002D6508: 1 instructions */
#define RAC2_K_PAUSE_LIST_COUNT 0x1CC /* v1.01 0x1CC; func_00341610 as func_00342520: 1 instructions */
#define RAC2_K_PAUSE_LIST_CURSORS 0x1B8 /* v1.01 0x1B8; func_00341610 as func_00342520: 1 instructions */
#define RAC2_K_PAUSE_LIST_ICONS 0x1C8 /* v1.01 0x1C8; func_00341610 as func_00342520: 1 instructions */
#define RAC2_K_PAUSE_LIST_OFFSET 0x3C160 /* v1.01 0x3BE08; func_0029C4C0 as func_0029D2F8: 1 instructions */
#define RAC2_K_PAUSE_LIST_WHICH 0x31C /* v1.01 0x2FC; func_00341610 as func_00342520: 1 instructions */
#define RAC2_K_PAUSE_OPTIONS_ROWS 0x8 /* v1.01 0x8; func_00341610 as func_00342520: 3 instructions */
#define RAC2_K_TEXT_CAMERA_ROW 0x2C2E /* v1.01 0x2D98; func_0033C468 as func_0033D4B0: 1 instructions */
#define RAC2_K_TEXT_CAMERA_TITLE 0x2C2E /* v1.01 0x2D98; func_0033C468 as func_0033D4B0: 1 instructions */
#define RAC2_K_TEXT_LEFT_RIGHT 0x2C77 /* v1.01 0x2DE1; func_0033C738 as func_0033D780: 1 instructions */
#define RAC2_K_TEXT_PAGE_EXIT 0x2BE5 /* v1.01 0x2D4F; func_0033C468 as func_0033D4B0: 1 instructions */
#define RAC2_K_TEXT_PAGE_TOGGLE 0x2BE4 /* v1.01 0x2D4E; func_0033C468 as func_0033D4B0: 1 instructions */
#define RAC2_K_TEXT_SPEED 0x2DAE /* v1.01 0x2F18; func_0033C738 as func_0033D780: 1 instructions */
#define RAC2_K_TEXT_UP_DOWN 0x2C78 /* v1.01 0x2DE2; func_0033C738 as func_0033D780: 1 instructions */
#define RAC2_K_TEXT_WIDESCREEN 0x307A /* v1.01 0x31E4; func_0033C738 as func_0033D780: 1 instructions */
#define RAC2_K_TITLE_ICON_SHEET 0xEAA6 /* v1.01 0xEAA6; func_00342118 as func_00343028: 1 instructions */

#endif /* OPENRAC_RAC2_VERSION_H */
