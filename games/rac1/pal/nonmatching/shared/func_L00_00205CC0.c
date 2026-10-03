/* NON_MATCHING func_L00_00205CC0 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 808 / retail 816, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clamps two camera angle offsets in D_L00_0017A780 (+0x8A8, +0x8A4), steps the timer g[0xFF0] (wraps at 12), th
 *   Best p4.c (808 vs 816 bytes). Fresh `char *g3 = D_0013F450;` in the last loop helped. Left: second loop (spawn
 */
extern unsigned char D_0013F450[] NOT_SDA;
extern char D_L00_0017A780[];
extern int D_L00_0017C2C0[];
extern char D_L00_0017C140[];
extern char D_L00_0017C1B0[];
extern char D_L00_0017C220[];
extern float D_L00_0017C290[];
extern void func_L00_00205C18(void);
extern void func_0020D9D8(void *, void *);
extern void func_0020D960(char *, int, void *);

// Clamps the camera yaw/pitch offsets and steps the spawn timer, spawning or updating the 7 effect slots.
void func_L00_00205CC0(void) {
    char *p = D_L00_0017A780;
    char *g;
    char *g2;
    char *q;
    float a;
    int i;
    int n;
    a = *(float *)(p + 0x278) * 0.25f;
    *(float *)(p + 0x8A8) = a;
    if (a > 0.1483529806f) {
        *(float *)(p + 0x8A8) = 0.1483529806f;
    } else if (a < -0.1483529806f) {
        *(float *)(p + 0x8A8) = -0.1483529806f;
    }
    q = D_L00_0017A780;
    *(float *)(q + 0x8A4) = *(float *)(q + 0x274) / 2.8f;
    if (*(float *)(q + 0x8A4) > 0.2967059612f) *(float *)(q + 0x8A4) = 0.2967059612f;
    if (*(float *)(q + 0x8A4) < -0.1745329201f) *(float *)(q + 0x8A4) = -0.1745329201f;
    g = (char *)D_0013F450;
    if ((unsigned char)(*(unsigned char *)(*(char **)(g + 0x2080) + 0x53) - 1) < 2) {
        if (*(float *)(q + 0x8A8) > 0.0349065848f) *(float *)(q + 0x8A8) = 0.0349065848f;
        if (*(float *)(q + 0x8A8) < -0.0349065848f) *(float *)(q + 0x8A8) = -0.0349065848f;
        if (*(float *)(q + 0x8A4) > 0.1221730486f) *(float *)(q + 0x8A4) = 0.1221730486f;
        if (*(float *)(q + 0x8A4) < -0.1221730486f) *(float *)(q + 0x8A4) = -0.1221730486f;
    }
    func_L00_00205C18();
    g = (char *)D_0013F450;
    if (*(int *)(g + 0xFF0) == 0) return;
    n = *(int *)(g + 0xFF0) + 1;
    *(int *)(g + 0xFF0) = n;
    if (n >= 12) *(int *)(g + 0xFF0) = 0;
    g2 = g;
    if (*(int *)(g + 0xFF0) == 0) {
        for (i = 0; i < 7; i++) {
            char *e = g + i * 0x40;
            if (*(unsigned char *)(e + 0xD31) != 0) func_0020D9D8(*(char **)(g + 0x2080), e + 0xD30);
        }
        return;
    }
    for (i = 0; i < 7; i++) {
        char *e = g2 + i * 0x40;
        if (*(unsigned char *)(e + 0xD31) == 0) {
            func_0020D960(*(char **)(g2 + 0x2080), D_L00_0017C2C0[i], e + 0xD30);
            *(unsigned char *)(e + 0xD33) = 1;
            qcopy(e + 0xD40, D_L00_0017C140 + i * 16);
            qcopy(e + 0xD50, D_L00_0017C1B0 + i * 16);
            qcopy(e + 0xD60, D_L00_0017C220 + i * 16);
        }
    }
    {
        char *g3 = (char *)D_0013F450;
        for (i = 0; i < 7; i++) {
            *(float *)(g3 + 0xD3C + i * 0x40) = D_L00_0017C290[*(int *)(g3 + 0xFF0)];
        }
    }
}
