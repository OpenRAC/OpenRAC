/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* func_002E5398 as it is: its volume comes back in $v0, an integer (its assembly). */
extern int openrac_emitter_falloff(void *self, float d, float x, float y) __asm__("func_002E5398");

/* An emitter's volume from its distance to the listener (0x002E5490). The decompilation's C returns
 * a float, but the assembly hands back func_002E5398's $v0 untouched, and StartSoundEmitter
 * (func_002E68A0) reads $v0 as an int. With float, hostgen sees the result in $f0, a register the
 * callee never sets, and passes 0: every 3D sound started with a distance (the weapons' fire among
 * them) fell under StartSoundEmitter's cut of 32 and never played.
 * TODO: remove this file and its overrides.tsv row once rac2-decomp gives func_002E5490 an int
 * result that agrees with its callers' declarations (002E5AD0.c, 002E10B8-voices.c), at the next
 * decomp bump. */
int func_002E5490(char *self, char *p)
{
    float v = F391de845_FUN_00282E48(p, F391de845_D_001B5340);
    float *q = *(float **)(self + 8);

    return openrac_emitter_falloff(q, v, q[0], q[1]);
}
