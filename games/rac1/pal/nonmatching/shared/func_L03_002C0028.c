/* NON_MATCHING func_L03_002C0028 -- src/overlays/shared/vendor_00292AC0.c
 * Best so far: BYTES 50/772 (93.5% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_2c0028 __attribute__((mode(TI)));
extern char *D_L03_00160058_p __asm__("D_L03_00160058") MACRO_ADDR;
extern char *D_L03_00160064_p __asm__("D_L03_00160064") MACRO_ADDR;
extern int D_L03_0015F6B0_p __asm__("D_L03_0015F6B0") MACRO_ADDR;
extern char D_L03_001E3290[];
extern int func_001E9730();
extern void func_00213D28(void *, int, int);
extern char *func_L00_002DCD40(char *);
extern void func_L00_0025E210(void *);
extern void func_L00_00251E30(void *);
extern float *func_L00_0025D390_p(void *) __asm__("func_L00_0025D390");

/* Spawns a child from m's pool of 18 linked mobys: revives the first dead one (of class cls, or any if
 * -1) at `at`, splitting m's health with it, and counts m's living children. Prints a warning if no
 * free child was found. */
char *func_L03_002C0028(char *m, float *at, int cls) {
    float pos[4];
    float *pp;
    char *d;
    char *found = 0;
    int got;
    int i;
    char *o;
    *(Q_2c0028 *)pos = *(Q_2c0028 *)at;
    pp = pos;
    if (m == 0 || (d = *(char **)(m + 0x78)) == 0) return 0;
    d[0x86] = 0;
    got = 0;
    for (i = 0; i < 18; i++) {
        int id = ((int *)(d + 0xE8))[i];
        if (id == -1) continue;
        o = D_L03_00160058_p + (id << 8);
        if (o != 0 && ((unsigned char *)o)[0x20] != 0xFE && ((unsigned char *)o)[0x20] != 0xFD) {
            d[0x86]++;
            continue;
        }
        if (cls != -1 && cls != *(short *)(o + 0xA6)) continue;
        if (got) continue;
        got = 1;
        d[0x86]++;
        qcopy(o + 0x10, pp);
        *(int *)(o + 0x1C) = 0;
        o[0x20] = 0;
        o[0xBC] = 0;
        {
            char *cl = *(char **)(o + 0x24);
            unsigned short fl = *(unsigned short *)(cl + 0x44);
            *(unsigned short *)(o + 0x34) = fl;
            *(float *)(o + 0x2C) = *(float *)(cl + 0x24);
            o[0x30] = m[0x30];
            *(short *)(o + 0x32) = *(short *)(m + 0x32);
            o[0x31] = got;
            if (*(int *)(cl + 0x40) != 0) *(unsigned short *)(o + 0x34) = fl | 0x10;
        }
        if (*(unsigned char *)(*(char **)(o + 0x24) + 0xF) != 0) *(unsigned short *)(o + 0x34) |= 0x400;
        ((unsigned char *)o)[0x71] = 0xFF;
        *(short *)(o + 0x36) = 0x7F80;
        ((unsigned char *)o)[0x72] = 0xFF;
        ((unsigned char *)o)[0xA4] = 0xFF;
        if (d[0x83] & 2) *(float *)(o + 0x2C) = *(float *)(*(char **)(o + 0x24) + 0x24) * 0.1f;
        func_00213D28(o, 0, 0);
        {
            char *t = func_L00_002DCD40(o);
            if (t != 0) {
                *(int *)(t + 0x94) = 0;
                *(short *)(t + 0x68) = 0;
                *(int *)(t + 0x6C) = 0;
            }
        }
        if (*(short *)(m + 0xB4) < *(short *)(o + 0xB4)) *(short *)(o + 0xB4) = *(short *)(m + 0xB4);
        if (*(short *)(o + 0xB4) <= 0) *(short *)(o + 0xB4) = got;
        *(short *)(m + 0xB4) -= *(short *)(o + 0xB4);
        if (*(short *)(m + 0xB4) < 0) *(short *)(m + 0xB4) = got;
        *(int *)(o + 0x94) = *(int *)(*(char **)(o + 0x24) + 0x10);
        func_L00_0025E210(o);
        func_L00_00251E30(o);
        found = o;
        {
            float *h = func_L00_0025D390_p(found);
            if (h != 0) {
                *(int *)(h + 13) = 0;
                *(int *)(h + 12) = 0;
                h[0] = (float)*(short *)(h + 1);
            }
        }
    }
    for (o = D_L03_00160064_p; o != 0; o = *(char **)(o + 0x28)) {
        if (*(short *)(o + 0xA6) == 0x221 && ((unsigned char *)o)[0x20] != 0xFE && ((unsigned char *)o)[0x20] != 0xFD) {
            d[0x86]++;
        }
    }
    if (found == 0) {
        func_001E9730(D_L03_001E3290, D_L03_0015F6B0_p, *(short *)(m + 0xB2), *(short *)(m + 0xA6));
    }
    return found;
}
