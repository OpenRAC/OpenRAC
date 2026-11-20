/* Written by port/tools/hostgen/release_carry_rac3.py; do not edit. */
/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * NTSC-U's address for each PAL address the shared host C writes as RAC3_ADDR(0x...u) (rac3_version.h):
 * the global or function of that address renamed (config/us/names.tsv, config/us/map.tsv), else
 * release/release.json's `addresses`. One not known is 0: what reads it reads nothing of the game's. */
#ifndef OPENRAC_RAC3_ADDRESSES_H
#define OPENRAC_RAC3_ADDRESSES_H

#define RAC3_ADDR_0x00141850u 0x00141850u /* D_00141850 (names.tsv, the same address) */
#define RAC3_ADDR_0x00142120u 0x00142120u /* D_00142120 (names.tsv, the same address) */
#define RAC3_ADDR_0x00142430u 0x00142430u /* D_00142430 (names.tsv, the same address) */
#define RAC3_ADDR_0x00143950u 0x00143950u /* D_00143950 (names.tsv, the same address) */
#define RAC3_ADDR_0x001439FDu 0x001439FDu /* D_001439FD (names.tsv, the same address) */
#define RAC3_ADDR_0x00160C40u 0x00160C40u /* D_00160C40 (names.tsv, the same address) */
#define RAC3_ADDR_0x001A4C60u 0x001A4C60u /* D_001A4C60 (names.tsv, the same address) */
#define RAC3_ADDR_0x001CD0C0u 0x001CD0C0u /* D_001CD0C0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4B50u 0x001D4B50u /* D_001D4B50 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BD0u 0x001D4BD0u /* D_001D4BD0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4BD4u 0x001D4BD4u /* D_001D4BD4 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4CE8u 0x001D4CE8u /* D_001D4CE8 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4CECu 0x001D4CECu /* D_001D4CEC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D4DA0u 0x001D4DA0u /* D_001D4DA0 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D52FCu 0x001D52FCu /* D_001D52FC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5458u 0x001D5458u /* D_001D5458 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D545Cu 0x001D545Cu /* D_001D545C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5688u 0x001D5688u /* D_001D5688 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D568Cu 0x001D568Cu /* D_001D568C (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5690u 0x001D5690u /* D_001D5690 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5694u 0x001D5694u /* D_001D5694 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B74u 0x001D5B74u /* D_001D5B74 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B90u 0x001D5B90u /* D_001D5B90 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5B94u 0x001D5B94u /* D_001D5B94 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5BDCu 0x001D5BDCu /* D_001D5BDC (names.tsv, the same address) */
#define RAC3_ADDR_0x001D5C90u 0x001D5C90u /* D_001D5C90 (names.tsv, the same address) */
#define RAC3_ADDR_0x001D89B8u 0x001D89E8u /* D_001D89B8 (names.tsv) */
#define RAC3_ADDR_0x001D89BCu 0x001D89ECu /* D_001D89BC (names.tsv) */
#define RAC3_ADDR_0x001D9D04u 0x001D9D84u /* D_001D9D04 (names.tsv) */
#define RAC3_ADDR_0x001D9EA8u 0x001D9F28u /* D_001D9EA8 (names.tsv) */
#define RAC3_ADDR_0x001D9EACu 0x001D9F2Cu /* D_001D9EAC (names.tsv) */
#define RAC3_ADDR_0x001DA050u 0x001DA0D0u /* D_001DA050 (names.tsv) */
#define RAC3_ADDR_0x001DA058u 0x001DA0D8u /* D_001DA058 (names.tsv) */
#define RAC3_ADDR_0x0022758Cu 0x0022760Cu /* D_0022758C (names.tsv) */
#define RAC3_ADDR_0x00280CF0u 0x00280D70u /* D_00280CF0 (names.tsv) */
#define RAC3_ADDR_0x0037F830u 0x0037F8B0u /* D_0037F830 (names.tsv) */
#define RAC3_ADDR_0x00381250u 0x00381280u /* func_00381250 (map.tsv differs) */

#endif /* OPENRAC_RAC3_ADDRESSES_H */
