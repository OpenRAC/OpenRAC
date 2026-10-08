/* NON_MATCHING func_L09_00306DD8 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: BYTES 7/488 (98.6% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1206: sweeps a hit test along a heading and logs hits (func_001E9730). p4.c/p5.c are 478/488 bytes 
 *   Swept hit test + log (488 bytes). Best now p9.c: 7/488, all in the log block: retail loads x+0xB2, x+0xA6 then
 */
typedef int u128_306DD8 __attribute__((mode(TI)));
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80_i(int, int, int) __asm__("func_0022ED80");
extern float func_001FA888(int);
extern void func_001F9EC0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_0025A890_c(void *, void *, int, float) __asm__("func_L00_0025A890");
extern int func_L00_001F2BE8_alt(float, void *, int, void *, void *) __asm__("func_L00_001F2BE8");
extern int func_00120778(float);
extern int func_001E9730();
extern int D_L09_0015F6B0 MACRO_ADDR;
extern struct { char pad[0x18]; char *p; } D_L09_00174040;
extern char D_L09_002093A0[];

// Builds a swept hit test along the moby's heading and logs the hits.
void func_L09_00306DD8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    float v10[4];
    float w[4];
    float buf[12];
    int i;
    float t;
    if (*(unsigned char *)(moby + 0x20) == 0) {
        *(unsigned char *)(moby + 0x20) = 1;
        *(int *)(data + 0x14) = -1;
    }
    if ((D_L09_0015F6B0 & 7) == (((int)moby >> 8) & 7)) {
        if (*(int *)(data + 0x14) == -1 ||
            (func_L00_0028EB98(moby, *(int *)(data + 0x14)) == 0 && *(int *)(data + 0x14) == -1)) {
            *(int *)(data + 0x14) = func_0022ED80_i(0, 4, (int)moby);
        }
    }
    t = (*(float *)(data + 4) - (*(float *)data + *(float *)data)) / func_001FA888(*(int *)(data + 8));
    *(u128_306DD8 *)v = 0;
    v[2] = t;
    qcopy(v10, moby + 0x10);
    func_001F9EC0(v, v, moby + 0xC0);
    func_L00_001FF4B0(w, v, *(float *)data);
    func_001F9BD8(v10, v10, w);
    func_L00_0025A890_c(buf, moby, *(int *)(data + 0x10), *(float *)(data + 0xC));
    for (i = 0; i < *(int *)(data + 8); i++) {
        if (func_L00_001F2BE8_alt(*(float *)data, v10, 1, moby, buf) != 0) {
            char *x = D_L09_00174040.p;
            int b = *(short *)(x + 0xA6);
            int a = *(short *)(x + 0xB2);
            int c = *(short *)(moby + 0xB2);
            func_001E9730(D_L09_002093A0, c, b, a, func_00120778(*(float *)(data + 0xC)));
        }
        func_001F9BD8(v10, v10, v);
    }
}
