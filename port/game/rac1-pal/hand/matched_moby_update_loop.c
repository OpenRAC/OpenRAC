/* SPDX-License-Identifier: GPL-3.0-or-later */
/* func_00213C78 (update_visible_resident_objects), the moby update loop. Its matched C
 * (src/game/mobyutil.c) calls each moby's update callback through inline assembly: retail's
 * beq/move/jal with both delay slots empty only comes out of GCC 2.95 next to an asm statement,
 * so the matching C writes the call as one; hostgen skips inline assembly, and the port would
 * never call a moby's update. As the port runs it: the same loop with the
 * callback called in C, from the former candidate nonmatching/game/func_00213C78.c (fcheck: SAME
 * on 3 calls, 2026-10-18). Not a PS2 match; the matched C stays as it matches (hostgen.json
 * "overrides": this C replaces it in the port only). It goes at the end of mobyutil.c. */

/* func_00213C78: fetches the head of the moby list from func_0020E6B8 and keeps it in
   D_00160024. For each moby whose state byte is non-negative (signed): calls func_0020E3D0
   unless flags 0x40 is set, calls its update callback at 0x74 if non-null, then calls
   func_0020ED48 unless flags 0x4 is set. Walks the list through next at 0x28. */
extern void *func_0020E6B8_ov(void) __asm__("func_0020E6B8");
extern void func_0020E3D0_ov(void *) __asm__("func_0020E3D0");
extern void func_0020ED48_ov(void *) __asm__("func_0020ED48");
extern void *D_00160024_ov __asm__("D_00160024");

void func_00213C78(void) {
    char *m;

    m = (char *)func_0020E6B8_ov();
    D_00160024_ov = m;
    if (m != 0) {
        do {
            if ((signed char)m[0x20] >= 0) {
                if (!(*(unsigned short *)(m + 0x34) & 0x40)) {
                    func_0020E3D0_ov(m);
                }
                if (*(void (**)(void *))(m + 0x74) != 0) {
                    (*(void (**)(void *))(m + 0x74))(m);
                }
                if (!(*(unsigned short *)(m + 0x34) & 0x4)) {
                    func_0020ED48_ov(m);
                }
            }
            m = *(char **)(m + 0x28);
        } while (m != 0);
    }
}
