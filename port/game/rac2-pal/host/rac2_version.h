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

#define RAC2_LEVEL_BASE 0x001A7D00u /* a level's program replaces the executable's memory from here */
#define RAC2_GP 0x001AF070u         /* the executable's $gp */
#define RAC2_EXE_SIZE 0x00255000u   /* the executable's loaded bytes from 0x00100000, to a page */

#include "rac2_addresses.h"

/* Functions: v1.01's name and this release's (release_carry.json says how each was paired). */
#define func_00131AE8 func_00131B48
#define func_00131CB8 func_00131D18
#define func_00131CB8__game func_00131D18__game
#define func_00131D98 func_00131DF8
#define func_00132028 func_00132088
#define func_00132498 func_001324F8
#define func_001326D8 func_00132738
#define func_00132C48 func_00132CA8
#define func_00132DF0 func_00132E50
#define func_00133350 func_001333B0
#define func_0027B858 func_0027BD40
#define func_0027B858__game func_0027BD40__game
#define func_0027C878 func_0027CD60
#define func_0027CE18 func_0027D300
#define func_0027CEE0 func_0027D3C8
#define func_0027CF58 func_0027D440
#define func_0027D020 func_0027D508
#define func_0027D0E8 func_0027D5D0
#define func_0027DE50 func_0027E338
#define func_0027DE50__game func_0027E338__game
#define func_0027E018 func_0027E500
#define func_0027E018__game func_0027E500__game
#define func_0027E1A0 func_0027E688
#define func_0027E1A0__game func_0027E688__game
#define func_0027E4A8 func_0027E990
#define func_0027E4A8__game func_0027E990__game
#define func_0027E920 func_0027EE08
#define func_0027E920__game func_0027EE08__game
#define func_0027EA28 func_0027EF10
#define func_0027EA28__game func_0027EF10__game
#define func_0027F178 func_0027F660
#define func_0027F198 func_0027F680
#define func_0027F1B8 func_0027F6A0
#define func_0027F2A8 func_0027F7B0
#define func_0027F2A8__game func_0027F7B0__game
#define func_0027F538 func_0027FA40
#define func_0027F5B8 func_0027FAC0
#define func_0027F640 func_0027FB48
#define func_0027F640__game func_0027FB48__game
#define func_0027F980 func_0027FF28
#define func_0027FA10 func_0027FFB8
#define func_0027FAA8 func_00280050
#define func_0027FB40 func_002800E8
#define func_0027FBD8 func_00280180
#define func_0027FC70 func_00280A30
#define func_00280190 func_00280A58
#define func_00280200 func_00280AC8
#define func_00280270 func_00280B38
#define func_00280B90 func_00281450
#define func_00283180 func_00283A40
#define func_00283198 func_00283A58
#define func_00283CF0 func_002845B0
#define func_00283DB8 func_00284678
#define func_00283DB8__game func_00284678__game
#define func_00288CB0 func_002895C8
#define func_00288FB8 func_002898E8
#define func_00288FB8__game func_002898E8__game
#define func_0028E458 func_0028ECD8
#define func_0028E458__game func_0028ECD8__game
#define func_0028E640 func_0028EEC0
#define func_0028E868 func_0028F0E8
#define func_0028E868__game func_0028F0E8__game
#define func_0028EA58 func_0028F2D8
#define func_0028EA58__game func_0028F2D8__game
#define func_0028ECD8 func_0028F558
#define func_0028ECD8__game func_0028F558__game
#define func_0028EE98 func_0028F718
#define func_0028EE98__game func_0028F718__game
#define func_0028F078 func_0028F8F8
#define func_0028F078__game func_0028F8F8__game
#define func_0028F278 func_0028FAF8
#define func_0028F278__game func_0028FAF8__game
#define func_0028F410 func_0028FC90
#define func_0028F410__game func_0028FC90__game
#define func_0028F788 func_00290008
#define func_0028F788__game func_00290008__game
#define func_00298E08 func_002997A0
#define func_0029B168 func_0029BB00
#define func_0029B168__game func_0029BB00__game
#define func_0029D4D0 func_0029DE68
#define func_0029FEE0 func_002A08A0
#define func_002A4078 func_002A4A70
#define func_002A4AD8 func_002A54D0
#define func_002A4D88 func_002A5780
#define func_002A4E68 func_002A5860
#define func_002B9420 func_002B9FD8
#define func_002D58A0 func_002D6478
#define func_002D58A0__game func_002D6478__game
#define func_002DC900 func_002DD5F8
#define func_002E10B8 func_002E1E88
#define func_002E11C0 func_002E1F90
#define func_002E2C38 func_002E3A08
#define func_002E3940 func_002E4710
#define func_002E48E8 func_002E56B8
#define func_002E48E8__game func_002E56B8__game
#define func_002E5E58 func_002E6C28
#define func_002EF310 func_002F0260
#define func_002EF3B0 func_002F0300
#define func_002F0468 func_002F13B8
#define func_002F0E38 func_002F1D88
#define func_002F0EF8 func_002F1E48
#define func_002F3E78 func_002F4DC8
#define func_002FC070 func_002FD0C8
#define func_002FC560 func_002FD5B8
#define func_002FC8D0 func_002FD928
#define func_002FC990 func_002FD9E8
#define func_002FCB00 func_002FDB58
#define func_003136D0 func_00314B60
#define func_003136D0__game func_00314B60__game
#define func_00335E20 func_00337B00
#define func_00335E38 func_00337B18
#define func_00336320 func_00338000
#define func_00336828 func_00338508
#define func_00336920 func_00338600
#define func_00336A40 func_00338738
#define func_00336A78 func_00338770
#define func_0033AF48 func_0033CE70
#define func_0033B050 func_0033CF78
#define func_0033C2D8 func_0033E160
#define func_0033C2D8__game func_0033E160__game
#define func_0033C378 func_0033E248
#define func_0033C590 func_0033E460
#define func_0033C590__game func_0033E460__game
#define func_0033C738 func_0033E608
#define func_00341540 func_003435C0
#define func_00341540__game func_003435C0__game
#define func_00341610 func_00343690
#define func_00341610__game func_00343690__game
#define func_00341A48 func_00343AB0
#define func_00341A68 func_00343AD0
#define func_00341C20 func_00343C88
#define func_00341C20__game func_00343C88__game
#define func_00342380 func_003443E8
#define func_0034AD10 func_0034CED8
#define func_0034AD10__game func_0034CED8__game
#define func_0034E4F8 func_003506E0
#define func_0034E4F8__game func_003506E0__game
#define func_0034EF58 func_00351140
#define func_L00_00424F98 func_L00_0042A488
#define func_L00_00424F98__game func_L00_0042A488__game
#define func_L05_0041DF58 func_L05_00422CA8
#define func_L08_002F2C48 func_L00_002E80C0
#define func_L10_00421790 func_L10_00426618
#define func_L15_0042E1B0 func_L15_004334B0
#define func_L16_0042FB10 func_L16_00435550
#define func_L22_003A5018 func_L22_003A83A8
#define func_L23_0041DA48 func_L23_00422D10

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
#define RAC2_ADDR_0x001A8CFCu 0x001A8DACu /* the release's build */
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

#endif /* OPENRAC_RAC2_VERSION_H */
