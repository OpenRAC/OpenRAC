/* Carried by port/tools/versionmap/carry_functions.py from port/game/rac1-pal/hand/matched_moby_update_loop.c: func_00213C78, the port's override of the matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* carry, not from a `same` function or an `exact` data address (read them):
 *   D_00160024 -> 0015FF24: between
 */

extern void *func_0020E6B8_ov_c212e28(void) __asm__("func_0020D868");
extern void func_0020E3D0_ov_c212e28(void *) __asm__("func_0020D580");
extern void func_0020ED48_ov_c212e28(void *) __asm__("func_0020DEF8");
extern void *D_00160024_ov_c212e28 __asm__("D_0015FF24");
void FUN_00212e28(void) {
    char *m;
    m = (char *)func_0020E6B8_ov_c212e28();
    D_00160024_ov_c212e28 = m;
    if (m != 0) {
        do {
            if ((signed char)m[0x20] >= 0) {
                if (!(*(unsigned short *)(m + 0x34) & 0x40)) {
                    func_0020E3D0_ov_c212e28(m);
                }
                if (*(void (**)(void *))(m + 0x74) != 0) {
                    (*(void (**)(void *))(m + 0x74))(m);
                }
                if (!(*(unsigned short *)(m + 0x34) & 0x4)) {
                    func_0020ED48_ov_c212e28(m);
                }
            }
            m = *(char **)(m + 0x28);
        } while (m != 0);
    }
}
