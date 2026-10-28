/* SPDX-License-Identifier: GPL-3.0-or-later */
/* func_L00_002657B8 (link_and_update_all_mobys), the level copy of the moby update loop. Its
 * matched C (src/overlays/shared/help_message_removal.c) calls each moby's update callback through
 * inline assembly (retail's beq/move/jal with both delay slots empty only comes out of GCC 2.95
 * next to an asm statement); hostgen skips inline assembly, and the port would never call a moby's
 * update. As the port runs it: the same loop with the callback called in C, from the former
 * candidate nonmatching/game/func_L00_002657B8.c. Not a PS2 match; the matched C stays as it
 * matches (hostgen.json "overrides": this C replaces it in the port only). It goes at the end of
 * help_message_removal.c.
 *
 * Walks the moby table (0x100-byte entries from D_L00_00160098) and rebuilds the list headed at
 * D_L00_001600A4 (linked at +0x28) from the entries that are live (flag +0x20 has no 0x80 bit, no
 * 0x2 in +0x34) until the 0xFF end marker; then for each listed moby: a negative +0x20 skips it,
 * else MobyAnimAdvance unless +0x34 & 0x40, the callback at +0x74 if set, and MobyBuildMatrix
 * unless +0x34 & 4. */
extern char *D_L00_001ABD80_ov[] __asm__("D_L00_001ABD80");
extern char *D_L00_001600A4_ov __asm__("D_L00_001600A4");
extern char *D_L00_00160098_ov __asm__("D_L00_00160098");
extern void func_L00_002514B8_ov(void *) __asm__("func_L00_002514B8");
extern void func_L00_00251E30_ov(void *) __asm__("func_L00_00251E30");

void func_L00_002657B8(void) {
    char *q;
    char *prev;
    char *cur;
    void (*fn)(void *);

    D_L00_001ABD80_ov[0] = 0;
    D_L00_001600A4_ov = 0;
    q = D_L00_00160098_ov;
    prev = 0;
    if ((unsigned char)q[0x20] != 0xFF) {
        do {
            if ((unsigned char)q[0x20] & 0x80) {
                q += 0x100;
            } else if (*(unsigned short *)(q + 0x34) & 0x2) {
                q += 0x100;
            } else {
                if (prev == 0) {
                    D_L00_001600A4_ov = q;
                } else {
                    *(char **)(prev + 0x28) = q;
                }
                prev = q;
                q += 0x100;
            }
        } while ((unsigned char)q[0x20] != 0xFF);
    }
    if (prev != 0) {
        *(char **)(prev + 0x28) = 0;
    }

    cur = D_L00_001600A4_ov;
    while (cur != 0) {
        if ((signed char)cur[0x20] < 0) {
            cur = *(char **)(cur + 0x28);
            continue;
        }
        if (!(*(unsigned short *)(cur + 0x34) & 0x40)) {
            func_L00_002514B8_ov(cur);
        }
        fn = *(void (**)(void *))(cur + 0x74);
        if (fn != 0) {
            fn(cur);
        }
        if (!(*(unsigned short *)(cur + 0x34) & 0x4)) {
            func_L00_00251E30_ov(cur);
        }
        cur = *(char **)(cur + 0x28);
    }
}
