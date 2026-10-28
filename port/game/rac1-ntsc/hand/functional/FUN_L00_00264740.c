/* Carried by port/tools/versionmap/carry_functions.py from port/game/rac1-pal/hand/matched_moby_link_loop.c: func_L00_002657B8, the port's override of the matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */

extern char *D_L00_001ABD80_ov_c00_264740[] __asm__("D_L00_001ABA00");
extern char *D_L00_001600A4_ov_c00_264740 __asm__("D_L00_0015FFE4");
extern char *D_L00_00160098_ov_c00_264740 __asm__("D_L00_0015FFD8");
extern void func_L00_002514B8_ov_c00_264740(void *) __asm__("func_L00_00250480");
extern void func_L00_00251E30_ov_c00_264740(void *) __asm__("func_L00_00250DF8");
void FUN_L00_00264740(void) {
    char *q;
    char *prev;
    char *cur;
    void (*fn)(void *);
    D_L00_001ABD80_ov_c00_264740[0] = 0;
    D_L00_001600A4_ov_c00_264740 = 0;
    q = D_L00_00160098_ov_c00_264740;
    prev = 0;
    if ((unsigned char)q[0x20] != 0xFF) {
        do {
            if ((unsigned char)q[0x20] & 0x80) {
                q += 0x100;
            } else if (*(unsigned short *)(q + 0x34) & 0x2) {
                q += 0x100;
            } else {
                if (prev == 0) {
                    D_L00_001600A4_ov_c00_264740 = q;
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
    cur = D_L00_001600A4_ov_c00_264740;
    while (cur != 0) {
        if ((signed char)cur[0x20] < 0) {
            cur = *(char **)(cur + 0x28);
            continue;
        }
        if (!(*(unsigned short *)(cur + 0x34) & 0x40)) {
            func_L00_002514B8_ov_c00_264740(cur);
        }
        fn = *(void (**)(void *))(cur + 0x74);
        if (fn != 0) {
            fn(cur);
        }
        if (!(*(unsigned short *)(cur + 0x34) & 0x4)) {
            func_L00_00251E30_ov_c00_264740(cur);
        }
        cur = *(char **)(cur + 0x28);
    }
}
