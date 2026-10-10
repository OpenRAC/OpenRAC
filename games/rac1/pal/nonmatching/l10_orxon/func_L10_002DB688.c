/* NON_MATCHING func_L10_002DB688 -- src/overlays/l10_orxon/vendor_00296BD8.c
 * Best so far: SIZE ours 956 / retail 964, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Update function (moby class 1067, level 10): switch on moby[0x20] with case 0 (init: shuffle a 10-byte copy, c
 *   Differences left: one saved register too many (moby lands in $20, retail $19: the constant -1 takes $19 in the
 *   Would unblock: the allocator tie for -1 vs moby, or retail's source form for the shuffle loop (retail counts k
 */
extern int func_001E9730();
extern int func_001E9730_2da0f0(void *, int) __asm__("func_001E9730");
extern void func_0020D678(void *);
extern int func_002140B0(int);
extern float func_001FA748(float, float);
extern s32 func_001FA898(f32);
extern int func_001F9850(int);
extern float func_001F9B50(float);
extern float func_001F9D10(void *, void *);
extern int func_L00_00200290(char *, float);
extern void func_001F49B0(void (*)(void), void *);
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80(int, int, int);
extern int func_001F9908(int *);
extern void func_L00_0028EBF0(int);
extern void func_L10_002DBA50(void);
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern char D_L10_001DD4D0[];
extern char D_L10_001DD510[];
extern char D_L10_00161D28[];
extern short D_L10_00161D18;
extern char D_0013E633[];
extern u128 D_L10_001672C0;
typedef struct { char b[10]; } Blob10;

/* Update function for moby class 1067 on level 10: first-frame setup, then per-state update. */
void func_L10_002DB688(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    int state;
    if (*(int *)(data + 0xC) == -1) {
        func_001E9730(D_L10_001DD4D0);
        func_001E9730_2da0f0(D_L10_001DD510, *(short *)(moby + 0xB2));
        func_0020D678(moby);
    }
    state = ((unsigned char *)moby)[0x20];
    if (state == 1) {
        float t, x, r, d;
        int n;
        *(u128 *)v = *(u128 *)(moby + 0x10);
        t = (float)*(int *)(data + 4) * *(float *)(data + 8) * 0.5f;
        x = *(float *)data * 0.5f;
        v[2] = v[2] + t;
        r = func_001F9B50(t * t + x * x);
        v[3] = r;
        d = func_001F9D10(moby + 0x10, &D_L10_001672C0);
        if (d < 80.0f) {
            n = func_L00_00200290((char *)v, 80.0f);
            if (0.0f <= (float)n) func_001F49B0(func_L10_002DBA50, moby);
        }
        if (func_L00_0028EB98(moby, *(int *)(data + 0x30))) return;
        *(int *)(data + 0x30) = func_0022ED80(0, 4, (int)moby);
        return;
    } else if (state == 2) {
        float t, x, r, d;
        int n, idx;
        idx = *(int *)(data + 0x30);
        func_001F9908((int *)(data + 0x34));
        if (idx != -1) {
            char *e = D_0013E633 + 0x1D + idx * 0x70;
            if (*(int *)(e + 0x88) == (int)moby && ((unsigned char *)e)[0x74]) func_L00_0028EBF0(idx);
        }
        *(int *)(data + 0x30) = -1;
        *(u128 *)v = *(u128 *)(moby + 0x10);
        t = (float)*(int *)(data + 4) * *(float *)(data + 8) * 0.5f;
        x = *(float *)data * 0.5f;
        v[2] = v[2] + t;
        r = func_001F9B50(t * t + x * x);
        v[3] = r;
        d = func_001F9D10(moby + 0x10, &D_L10_001672C0);
        if (d < 80.0f) {
            n = func_L00_00200290((char *)v, 80.0f);
            if (0.0f <= (float)n) func_001F49B0(func_L10_002DBA50, moby);
        }
        return;
    } else if (state == 0) {
        Blob10 buf;
        int m, i, j, r;
        if (D_0015EE84_m == 10 && D_0013E633[0x2EC1] == 1) {
            func_0020D678(moby);
            return;
        }
        *(Blob10 *)&buf = *(Blob10 *)D_L10_00161D28;
        for (m = 1; m <= 10; m++) {
            i = 0;
            r = func_002140B0(9);
            while (i < r) {
                unsigned char a, b;
                j = i + 1;
                a = ((unsigned char *)&buf)[i];
                b = ((unsigned char *)&buf)[j];
                ((unsigned char *)&buf)[j] = a;
                ((unsigned char *)&buf)[i] = b;
                i = j;
                r = func_002140B0(9);
            }
        }
        for (j = 0; j < *(int *)(data + 4); j++) (data + 0x10)[j] = ((unsigned char *)&buf)[j];
        *(float *)(moby + 0x48) = func_001FA748(*(float *)(moby + 0x48), 1.5707963f);
        moby[0x20] = 1;
        ((unsigned char *)moby)[0x30] = 0x60;
        *(int *)(data + 0x30) = -1;
        *(int *)(data + 0x34) = func_001F9850(func_001FA898(*(float *)&D_L10_00161D18 * 60.0f));
        *(unsigned short *)(moby + 0x34) |= 0x41;
        return;
    }
    return;
}
