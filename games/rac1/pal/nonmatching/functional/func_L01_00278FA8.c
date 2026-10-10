/* func_L01_00278FA8 -- src/overlays/shared/mobyutil_0026E8E0.c (functional C for the port, not a match)
 * Is Ratchet standing on this moby? In movement group 3, or in state 0x1C, the answer is his
 * riding moby (hero +0x4F8); otherwise his ground moby (+0x2FC) while on the ground (+0x30E == 0).
 * Retail shares its tail with func_L01_00278FE0 (a linker-joined remnant); this is the whole test.
 * From the staged near miss (nonmatching/shared/func_L01_00278FA8.c), made self-contained.
 * equiv: DIFFERENT by construction: retail's body ends in func_L01_00278FE0, so the tool sees only
 * the head; the head's loads and tests are all here (the ground-moby test is also included).
 */
typedef struct {
    unsigned char pad0[0x2FC];
    void *ground;
    unsigned char pad300[0x30E - 0x300];
    short air;
    unsigned char pad310[0x4F8 - 0x310];
    void *riding;
    unsigned char pad4FC[0x2084 - 0x4FC];
    int state;
    unsigned char pad2088[4];
    int group;
} Hero_278FA8;
extern Hero_278FA8 G_278FA8 __asm__("D_0013F450");

int func_L01_00278FA8(void *moby) {
    if (G_278FA8.group == 3 || G_278FA8.state == 0x1C) {
        if (G_278FA8.riding != moby) {
            return 0;
        }
        return 1;
    }
    if (G_278FA8.air != 0) {
        return 0;
    }
    if (G_278FA8.ground == moby) {
        return 1;
    }
    return 0;
}
