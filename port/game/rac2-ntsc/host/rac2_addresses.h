/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando's words the port's tools read, by the names of the decompilation's symbol table
 * (games/rac2/ntsc/symbol_addrs/usa/symbol_addrs.txt): each define is a name of that table with
 * RAC2_ and its address, which test_regress.py checks against the table, so nothing here is a
 * number of its own. Below 0x1A7C80 every program shares the word; from there on it is the boot
 * program's, and the loaded level has its copy elsewhere (OPENRAC_DATA). */
#ifndef OPENRAC_RAC2_ADDRESSES_H
#define OPENRAC_RAC2_ADDRESSES_H

/* Shared by every program. */
#define RAC2_g_heroPos 0x00189EA0u           /* the hero's position, 3 floats */
#define RAC2_g_heroOrientVec 0x00189EB0u     /* his rotation, 3 floats */
#define RAC2_g_pHeroMoby 0x0018C0B0u         /* his moby */
#define RAC2_g_heroState 0x0018C0B4u         /* his state */
#define RAC2_g_health 0x0018C2ECu            /* his health, an int: 0 is dead */
#define RAC2_g_playerProgress 0x001A79F0u    /* the save's block: +0 the level */
#define RAC2_g_boltCount 0x001A79F8u         /* the bolts */
#define RAC2_g_nanotech 0x001A7A0Cu          /* the nanotech level the armour's health follows */
#define RAC2_g_itemEquippedSlot 0x00139568u  /* item number to its record in g_weaponTable */
#define RAC2_g_weaponAmmo 0x00139688u        /* the ammunition by item number, s32 */
#define RAC2_g_inventoryOwned 0x001A7AF8u    /* whether he has the item, a byte by item number */
#define RAC2_g_inventoryNewFlag 0x001A7B30u  /* the same, shown as new */
#define RAC2_g_inventoryOrder 0x001A7B68u    /* the 32 slots of the weapon lists: item | flag, 0xFF */

/* The boot program's; a level's copy through OPENRAC_DATA. */
#define RAC2_g_nLevelExitRequested 0x001A8B84u  /* the loop ends and the next program loads */
#define RAC2_g_nGameState 0x001A8BB0u           /* 0 play, 2 a scene, 6 a program loading */
#define RAC2_g_nLevelExitDestination 0x001B1680u /* the level that loads then */
#define RAC2_g_cameraPos 0x001B5340u            /* the eye, 3 floats */
#define RAC2_g_cameraMatrix 0x001B5570u         /* rows of 4 floats: forward, left, up */
#define RAC2_g_frameCounter 0x001B1598u         /* the state loop's ticks: 0 before it runs */
#define RAC2_g_mobyTableBase 0x001B1B5Cu        /* the first moby */
#define RAC2_g_mobyTableEnd 0x001B1B64u         /* past the last */
#define RAC2_g_weaponTable 0x00239BA0u          /* the items' records, 0xE0 bytes each */

#endif /* OPENRAC_RAC2_ADDRESSES_H */
