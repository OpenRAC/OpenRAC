/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * PAL's (SCES-52456) addresses for the host C here, which is every Up Your Arsenal disc's: the
 * NTSC-U program (port/game/rac3-ntsc, hostgen.json "host_shared") compiles it too, with its own
 * rac3_version.h first on the include path and the functions renamed (rac3_names.tsv,
 * tools/hostgen/release_host.py). An address of the game's memory that NTSC-U may have elsewhere is
 * written RAC3_ADDR(0x...u), PAL's value: tools/hostgen/release_carry_rac3.py writes NTSC-U's
 * (rac3-ntsc/host/rac3_addresses.h). The resident core below 0x001D5680 is the same on both discs
 * for most of its data, but not all: when in doubt, wrap it. */
#ifndef OPENRAC_RAC3_VERSION_H
#define OPENRAC_RAC3_VERSION_H

/* The release the host C is written for. */
#define RAC3_PRIMARY 1
#define RAC3_PAL 1

#define RAC3_ADDR(a) (a)
/* An address of level `level`'s own memory (its .lit, .bss, .data), PAL's in that level: NTSC-U's
 * comes from the same instruction of the same code (tools/hostgen/rac3_level_pairs.py). Write the
 * level as a decimal number. */
#define RAC3_LADDR(level, a) (a)
/* An address of the multiplayer executable's (I5BOOTN.ELF) own memory, PAL's: NTSC-U's from the
 * same instructions (tools/hostgen/rac3_mp_pairs.py). */
#define RAC3_MADDR(a) (a)
/* The boot executable SYSTEM.CNF starts, which leaving multiplayer starts again. */
#define RAC3_BOOT_FILE "SCES_524.56"

#endif /* OPENRAC_RAC3_VERSION_H */
