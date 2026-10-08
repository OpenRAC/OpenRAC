/* NON_MATCHING func_L18_002D7310 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: SIZE ours 488 / retail 484, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby hit reaction: if state not 0/4/5, either latches state 5 (global==0x72) or calls func_L00_0025F4A8 with o
 *   p3.c is closest (lbu fixed, MACRO_ADDR D_L18_0015F660): remaining diff is that retail shares ONE jal between b
 *   Locals-per-arg single-call variant (p5.c) had a declaration-order compile error and the budget ran out.
 *   t11 round (p6-p10): hit-reaction for a moby (state 5/0/4 return; if g[0x2084]==0x72 just enter state 5 else ma
 */
extern char D_0013E633[];
extern float D_L18_0015F660[] MACRO_ADDR;
extern void *func_L00_0025B478(void *, int, int);
extern int func_001F9908(int *);
extern float func_001F9D48(void *, void *);
extern void func_L00_0025F4A8_x(void *, void *, void *, float, float, int, int, int, int, int, float, float, float, float, float, int, int, int) __asm__("func_L00_0025F4A8");

/* Reacts to the player's attack: plays one of two hit responses and enters state five. */
void func_L18_002D7310(void *mobyp) {
    unsigned char *moby = mobyp;
    char *d = *(char **)(moby + 0x78);
    unsigned short flags;
    if (moby[0x20] == 5 || moby[0x20] == 0 || moby[0x20] == 4) return;
    {
        char *g = D_0013E633 + 0xE1D;
        if (*(int *)(g + 0x2084) == 0x72) {
            flags = *(unsigned short *)(moby + 0x34);
            moby[0x20] = 5;
        } else {
            void *r = func_L00_0025B478(moby, 0x330000, 0);
            moby[0xA4] = 0xFF;
            if (*(char **)(g + 0x23C) == (char *)moby || *(char **)(g + 0x240) == (char *)moby || *(int *)(d + 0x19C) != 0) {
                func_L00_0025F4A8_x(moby, D_L18_0015F660, 0, 1.5f, 1.0f, 0x14, 6, 0x20, 1, 1, 3.0f, 1.5f, 9.0f, 1.5f, 0.0f, 0, -1, 0);
            } else if (func_001F9908((int *)(d + 0x194)) != 0 || r != 0 ||
                       (moby[0x20] == 1 && func_001F9D48(moby + 0x10, g + 0x80) < 2.0f)) {
                func_L00_0025F4A8_x(moby, D_L18_0015F660, 0, 0.0f, 0.0f, 5, 2, 8, 1, 0, 1.0f, 0.5f, 9.0f, 0.5f, 0.0f, 0, -1, 0);
            } else {
                return;
            }
            flags = *(unsigned short *)(moby + 0x34);
            moby[0x20] = 5;
        }
        flags |= 0x41;
        *(int *)(moby + 0x94) = 0;
        flags &= 0xEFFF;
        *(unsigned short *)(moby + 0x34) = flags;
    }
}
