/* NON_MATCHING func_L01_002FB588 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 752 / retail 784, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef int Q_2fb588 __attribute__((mode(TI)));
typedef struct {
    char b[24];
} Blob24_2fb588;
extern Blob24_2fb588 D_L01_0020B7E0;
extern Blob24_2fb588 D_L01_0020B7F8;
extern char D_L01_001E3720[];
extern void func_001F9BC0(void *);
extern void func_L00_0026B890(void *, void *, int, int, int, int, int, int, float, float);
extern void func_L00_002ADBB0(void *, void *, void *, int, int, int, int, int, float);
extern char *func_L00_002D4CE8(char *a, char *b, int c, char *d);

/* Explosion at `at`: three fireball bursts in random colours from the two palettes, a flash and shock
 * ring on the moby, its explosion sound, and an optional light (radius `light`, or 13 if negative). */
void func_L01_002FB588(void *mv, void *at, int snd, float scale, float light) {
    char *m = mv;
    float pos[4];
    float vel[4];
    int ca[6];
    int cb[6];
    int i;
    *(Q_2fb588 *)pos = *(Q_2fb588 *)at;
    func_001F9BC0(vel);
    for (i = 0; i < 3; i++) {
        float sz = scale * 400000.0f;
        float g = func_002140F8(8.0f, 10.0f) * D_0015EE6C;
        int *pa, *pb;
        int l1, l2;
        *(Blob24_2fb588 *)ca = D_L01_0020B7E0;
        *(Blob24_2fb588 *)cb = D_L01_0020B7F8;
        pa = &ca[func_002140B0(6)];
        pb = &cb[func_002140B0(6)];
        l1 = func_L00_00258BC8(func_001F9850(0xF), func_001F9850(0x14));
        l2 = func_L00_00258BC8(func_001F9850(0x19), func_001F9850(0x1E));
        func_L00_0026B890(pos, vel, *pa, *pb, l1, l2, 0, 0, sz, g * scale);
    }
    if (m != 0) {
        func_L00_002ADBB0(m, pos, vel, func_001F9850(0x14), 0x7F, 0, 0x40, 0x30, scale + scale);
        func_L00_002ADBB0(m, pos, vel, func_001F9850(0x1D), 0x20, 0, 0x20, 0, scale * 1.5f);
        if (((unsigned char *)m)[0x20] != 0xFE && ((unsigned char *)m)[0x20] != 0xFD && snd != -1) {
            func_0022ED80(snd, 0, (int)m);
        }
    }
    if (light != 0.0f) {
        if (0.0f < light) {
            *(float *)(D_L01_001E3720 + 0x20) = light;
            *(float *)(D_L01_001E3720 + 0x24) = light;
            *(float *)(D_L01_001E3720 + 0x28) = light;
        } else {
            *(float *)(D_L01_001E3720 + 0x20) = 13.0f;
            *(float *)(D_L01_001E3720 + 0x24) = 13.0f;
            *(float *)(D_L01_001E3720 + 0x28) = 13.0f;
        }
        func_L00_002D4CE8(D_L01_001E3720, (char *)pos, 0, 0);
    }
}
