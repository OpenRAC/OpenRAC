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

#define RAC2_LEVEL_BASE 0x001A7C80u /* a level's program replaces the executable's memory from here */
#define RAC2_GP 0x001AEFF0u         /* the executable's $gp */
#define RAC2_EXE_SIZE 0x00253000u   /* the executable's loaded bytes from 0x00100000, to a page */

#include "rac2_addresses.h"

/* Functions: v1.01's name and this release's (release_carry.json says how each was paired). */
#define func_0027B858 func_0027BEB8
#define func_0027B858__game func_0027BEB8__game
#define func_0027C878 func_0027CED8
#define func_0027CE18 func_0027D488
#define func_0027CEE0 func_0027D550
#define func_0027CF58 func_0027D5C8
#define func_0027D020 func_0027D690
#define func_0027D0E8 func_0027D758
#define func_0027DE50 func_0027E4D0
#define func_0027DE50__game func_0027E4D0__game
#define func_0027E018 func_0027E698
#define func_0027E018__game func_0027E698__game
#define func_0027E1A0 func_0027E820
#define func_0027E1A0__game func_0027E820__game
#define func_0027E4A8 func_0027EB28
#define func_0027E4A8__game func_0027EB28__game
#define func_0027E920 func_0027EFA0
#define func_0027E920__game func_0027EFA0__game
#define func_0027EA28 func_0027F0A8
#define func_0027EA28__game func_0027F0A8__game
#define func_0027F178 func_0027F7F8
#define func_0027F198 func_0027F818
#define func_0027F1B8 func_0027F838
#define func_0027F2A8 func_0027F920
#define func_0027F2A8__game func_0027F920__game
#define func_0027F538 func_0027FBA8
#define func_0027F5B8 func_0027FC28
#define func_0027F640 func_0027FCB0
#define func_0027F640__game func_0027FCB0__game
#define func_0027F980 func_00280090
#define func_0027FA10 func_00280120
#define func_0027FAA8 func_002801B8
#define func_0027FB40 func_00280250
#define func_0027FBD8 func_002802E8
#define func_0027FC70 func_00280B20
#define func_00280190 func_00280B48
#define func_00280200 func_00280BB8
#define func_00280270 func_00280C28
#define func_00280B90 func_00281540
#define func_00283180 func_00283B30
#define func_00283198 func_00283B48
#define func_00283CF0 func_002846A0
#define func_00283DB8 func_00284768
#define func_00283DB8__game func_00284768__game
#define func_00288CB0 func_002896D8
#define func_00288FB8 func_002899F8
#define func_00288FB8__game func_002899F8__game
#define func_0028E458 func_0028ECC0
#define func_0028E458__game func_0028ECC0__game
#define func_0028E640 func_0028EEA8
#define func_0028E868 func_0028F0D0
#define func_0028E868__game func_0028F0D0__game
#define func_0028EA58 func_0028F2C0
#define func_0028EA58__game func_0028F2C0__game
#define func_0028ECD8 func_0028F540
#define func_0028ECD8__game func_0028F540__game
#define func_0028EE98 func_0028F700
#define func_0028EE98__game func_0028F700__game
#define func_0028F078 func_0028F8E0
#define func_0028F078__game func_0028F8E0__game
#define func_0028F278 func_0028FAE0
#define func_0028F278__game func_0028FAE0__game
#define func_0028F410 func_0028FC78
#define func_0028F410__game func_0028FC78__game
#define func_0028F788 func_0028FFF0
#define func_0028F788__game func_0028FFF0__game
#define func_00298E08 func_00299BF8
#define func_0029B168 func_0029BF58
#define func_0029B168__game func_0029BF58__game
#define func_0029D4D0 func_0029E308
#define func_0029FEE0 func_002A0D18
#define func_002A4078 func_002A4EC8
#define func_002A4AD8 func_002A5928
#define func_002A4D88 func_002A5BD8
#define func_002A4E68 func_002A5CB8
#define func_002B9420 func_002BA340
#define func_002D58A0 func_002D6508
#define func_002D58A0__game func_002D6508__game
#define func_002DC900 func_002DD630
#define func_002E10B8 func_002E1ED0
#define func_002E11C0 func_002E1FD8
#define func_002E2C38 func_002E3A50
#define func_002E3940 func_002E4758
#define func_002E48E8 func_002E5720
#define func_002E48E8__game func_002E5720__game
#define func_002E5E58 func_002E6C90
#define func_002EF310 func_002F0168
#define func_002EF3B0 func_002F0208
#define func_002F0468 func_002F12C0
#define func_002F0E38 func_002F1C90
#define func_002F0EF8 func_002F1D50
#define func_002F3E78 func_002F4CD0
#define func_002FC070 func_002FCF00
#define func_002FC560 func_002FD3F0
#define func_002FC8D0 func_002FD760
#define func_002FC990 func_002FD820
#define func_002FCB00 func_002FD990
#define func_003136D0 func_003144C8
#define func_003136D0__game func_003144C8__game
#define func_00335E20 func_00336C28
#define func_00335E38 func_00336C40
#define func_00336320 func_00337128
#define func_00336828 func_00337630
#define func_00336920 func_00337728
#define func_00336A40 func_00337868
#define func_00336A78 func_003378A0
#define func_0033AF48 func_0033BF90
#define func_0033B050 func_0033C098
#define func_0033C2D8 func_0033D320
#define func_0033C2D8__game func_0033D320__game
#define func_0033C378 func_0033D3C0
#define func_0033C590 func_0033D5D8
#define func_0033C590__game func_0033D5D8__game
#define func_0033C738 func_0033D780
#define func_00341540 func_00342450
#define func_00341540__game func_00342450__game
#define func_00341610 func_00342520
#define func_00341610__game func_00342520__game
#define func_00341A48 func_00342958
#define func_00341A68 func_00342978
#define func_00341C20 func_00342B30
#define func_00341C20__game func_00342B30__game
#define func_00342380 func_00343290
#define func_0034AD10 func_0034BA50
#define func_0034AD10__game func_0034BA50__game
#define func_0034E4F8 func_0034F240
#define func_0034E4F8__game func_0034F240__game
#define func_0034EF58 func_0034FCA0
#define func_L00_00424F98 func_L00_004264A8
#define func_L00_00424F98__game func_L00_004264A8__game
#define func_L05_0041DF58 func_L05_0041F478
#define func_L08_002F2C48 func_L00_002E7920
#define func_L10_00421790 func_L10_00422CA8
#define func_L15_0042E1B0 func_L15_0042F6D0
#define func_L16_0042FB10 func_L16_00430F58
#define func_L22_003A5018 func_L22_003A6448
#define func_L23_0041DA48 func_L23_0041EF48

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
#define RAC2_ADDR_0x001A8CFCu 0x001A8D04u /* the release's build */
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
#define RAC2_ADDR_0x00261590u 0x00261570u /* the release's build */
#define RAC2_ADDR_0x00263B50u 0x00263B10u /* the release's build */
#define RAC2_ADDR_0x00263EF0u 0x00263EB0u /* the release's build */
#define RAC2_ADDR_0x00264290u 0x00264250u /* the release's build */
#define RAC2_ADDR_0x0026EDE8u 0x0026EDE8u /* code */
#define RAC2_ADDR_0x002DC908u 0x002DD630u /* code */

/* Addresses of a level's program. */
#define RAC2_LADDR_00_0x001B14ACu 0x001B144Cu /* the release's build */
#define RAC2_LADDR_00_0x001B14B0u 0x001B1450u /* the release's build */
#define RAC2_LADDR_00_0x001B8E10u 0x001B8D90u /* the release's build */
#define RAC2_LADDR_00_0x001B8E20u 0x001B8DA0u /* the release's build */

#endif /* OPENRAC_RAC2_VERSION_H */
