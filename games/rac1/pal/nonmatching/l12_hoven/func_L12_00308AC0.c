/* NON_MATCHING func_L12_00308AC0 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: BYTES 6/348 (98.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a debris moby in the slot table entry chosen by data[0xF0], copies the owner's position/model fields in
 *   Closest candidate p7.c (14 of 348 bytes wrong, same size): the prologue index add is `addu $v0,$v0,$a0` instea
 *   Store reorderings (p4-p8) only move the tie around; an unblock needs the wording that makes the 0x30 constant 
 *   Round fz5/y01: best is p11.c (BYTES 6/348): best.c with the 0x31 store written before the 0x36 store. Left: `a
 */
extern void func_L00_00251E30(void *);
extern float func_002140F8(float, float);
extern void func_L00_0026ED30(float, void *, int);
extern int func_L00_0025D390(char *);
extern int D_L12_00160058_m __asm__("D_L12_00160058") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

/* spawns a debris piece from the owner's slot at the owner's position, then bursts particles */
void func_L12_00308AC0(char *moby) {
    char *data = *(char **)(moby + 0x78);
    char *c = (char *)(D_L12_00160058_m + (((int *)(data + 0xC0))[*(int *)(data + 0xF0)] << 8));
    char *t;
    short *r;
    int i;
    qcopy(c + 0x10, moby + 0x10);
    *(float *)(c + 0x18) = *(float *)(c + 0x18) - 0.35f;
    c[0x20] = 0;
    c[0xBC] = 0;
    t = *(char **)(c + 0x24);
    *(unsigned short *)(c + 0x34) = *(unsigned short *)(t + 0x44);
    *(float *)(c + 0x2C) = *(float *)(t + 0x24);
    ((unsigned char *)c)[0x30] = 0xFF;
    c[0x31] = 1;
    *(short *)(c + 0x36) = 0x7F80;
    *(short *)(c + 0x32) = 0xFF;
    ((unsigned char *)c)[0xA4] = 0xFF;
    ((unsigned char *)c)[0x71] = 0xFF;
    ((unsigned char *)c)[0x72] = 0xFF;
    *(int *)(c + 0x94) = *(int *)(t + 0x10);
    *(long *)(c + 0x38) = *(long *)(moby + 0x38);
    func_L00_00251E30(c);
    *(char **)(c + 0xB8) = moby;
    for (i = 0; i < 14; i++) {
        func_L00_0026ED30(func_002140F8(D_0015EE6C * 3.0f, D_0015EE6C * 5.0f), moby + 0x10, 0);
    }
    r = (short *)func_L00_0025D390(c);
    if (r != 0) *(float *)r = (float)r[2];
}
