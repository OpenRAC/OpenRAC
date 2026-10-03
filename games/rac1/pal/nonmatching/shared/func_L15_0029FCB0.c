/* NON_MATCHING func_L15_0029FCB0 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: BYTES 5/284 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1427: scales moby+0x2C by class float, fades alpha (+0x23), spawns an effect via func_L00_0025A8E8,
 *   Best is p6.c/p10.c (5 instructions differ): only s0/s1 swapped between the two live-across-call locals (g = D_
 *   An allocator tie on two single-use pseudos; would need a different source shape for the pos/g pseudos.
 */
extern char D_0013E633[];
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_0025A8E8(float, float, float, char *, char *, int, int, int, int);
extern int func_001F9908(int *arg0);
extern void func_0020D678(void *);

/* update: scale and fade a moby, spawn an effect, delete it when done */
void func_L15_0029FCB0(char *moby) {
    char *cls = *(char **)(moby + 0x24);
    char *data = *(char **)(moby + 0x78);
    float t = *(float *)(moby + 0x2C) / *(float *)(cls + 0x24) * 0.5f;
    char *pos;
    char *g;
    t = t + *(float *)data;
    *(float *)(moby + 0x2C) = t + t;
    *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) * *(float *)(cls + 0x24);
    if (*(int *)(data + 8) < func_001F9850(0x28))
        moby[0x23] = *(int *)(data + 8) * 127 / func_001F9850(0x28);
    g = D_0013E633 + 0xE1D;
    pos = moby + 0x10;
    func_L00_0025A8E8(t, (float)func_001FA898_r(*(float *)(data + 0xC)), 1.0f,
                      *(char **)(g + 0x2080), pos, 0x30000, 0, 1, 0);
    if (func_001F9908((int *)(data + 8)))
        func_0020D678(moby);
}
