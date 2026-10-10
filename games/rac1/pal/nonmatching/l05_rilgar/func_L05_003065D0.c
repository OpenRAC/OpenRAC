/* NON_MATCHING func_L05_003065D0 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: SIZE ours 1792 / retail 1824, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - Stopped (runs 1-6 of 12): p4.c is the best candidate, SIZE 1792 vs 1824 (32 bytes short). The function is a 
 *   - Main wall: moby and data are swapped in our allocation (ours moby=$23, data=$fp; retail moby=$30, data=$23).
 *   - Still differing: the first block's a/b variable is in $3 where retail keeps it in $16 (single saved variable
 */
extern char *func_L00_0025B478(void *, int, int);
extern char *func_L05_0028AA68(char *, char *, char *, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, int *, float *, int, int);
extern void func_0022ED80(int, int, int);
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001FA748(float, float);
extern int func_001F9850(int);
extern void func_001F9BC0(void *);
extern void *func_L00_00265050(char *, int, float *, void *, int, int, float *, float *, float, float *);
extern void func_L00_0025E4B0(void *, short *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern int func_001F9908_v(void *) __asm__("func_001F9908");
extern float func_001F9CB8(void *);
extern void func_L00_0025E590(void *, void *);
extern char D_L05_0015F660[] MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L05_00161C34;
extern short D_L05_00161BB0;
extern short D_L05_00161BB4;
extern char D_0013E633[];

/* Per-frame update of a rilgar moby: steers its path against the linked parts and emits the trail effects. */
void func_L05_003065D0(unsigned char *moby) {
    unsigned char *data;
    char *q;
    char *p;
    char *s;
    char *a;
    char *b;
    char *c;
    char *x;
    char *g;
    char *p184;
    char *p188;
    int r40;
    int t;
    float f44;
    float d;
    float v20[4];
    float v30[3];
    float k;
    float t3;
    float t4;
    float t5;
    float t7f;
    float *zv;

    f44 = 0.0f;
    p = func_L00_0025B478(moby, 0x210000, 0);
    data = *(unsigned char **)(moby + 0x78);
    if (*(char **)(data + 0x184) != 0) {
        q = func_L00_0025B478(*(char **)(data + 0x184), 0x210000, 0);
    } else {
        q = 0;
    }
    if (*(char **)(data + 0x188) != 0) {
        s = func_L00_0025B478(*(char **)(data + 0x188), 0x210000, 0);
    } else {
        s = 0;
    }
    a = func_L05_0028AA68(p, q, s, 0);
    x = 0;
    if (a != 0) {
        b = 0;
        if (*(char **)(a + 0x20) != (char *)moby) {
            b = a;
        }
        c = *(char **)(b + 0x20);
        x = b;
        if (c != 0) {
            t = *(short *)(c + 0xA6);
            if (t == 0x47 || t == 0) {
                x = 0;
            }
        }
    }
    func_L00_0025B4D0(moby, x, data + 0x20, 0, &r40, &f44, 0, 4);
    if (r40 != 1 && moby[0x20] != 3) {
        d = *(float *)(data + 0x20) - f44;
        *(float *)(data + 0x20) = d;
        if (0.0f < d && 0.0f < f44) {
            func_0022ED80(2, 0, (int)moby);
            data[0x67] = 0xC8;
        } else {
            func_0022ED80(3, 0, (int)moby);
            func_L00_002584A8(moby, 0, -1);
            qcopy(v20, moby + 0x10);
            v20[2] = v20[2] + 3.0f;
            func_L00_0025F4A8(moby, data + 0x40, v20, 0.0f, 0.0f, 20, 8, 20, 4.0f, 2.0f, 9.0f, 1.0f, -1, 15.0f, 1, 1, -1, 0);
            qcopy(v20, moby + 0x10);
            t3 = func_001F9F90(*(float *)(moby + 0x48));
            v20[0] = v20[0] + (t3 + t3);
            t4 = func_001F9FA8(*(float *)(moby + 0x48));
            v20[1] = v20[1] + (t4 + t4);
            func_L00_0025F4A8(moby, data + 0x40, v20, 0.0f, 0.0f, 20, 8, 20, 4.0f, 2.0f, 9.0f, 1.0f, -1, 15.0f, 1, 1, -1, 0);
            qcopy(v20, moby + 0x10);
            k = 3.1415927f;
            t3 = func_001FA748(*(float *)(moby + 0x48), k);
            t4 = func_001F9F90(t3);
            v20[0] = v20[0] + (t4 + t4);
            t5 = func_001FA748(*(float *)(moby + 0x48), k);
            t4 = func_001F9FA8(t5);
            v20[1] = v20[1] + (t4 + t4);
            func_L00_0025F4A8(moby, data + 0x40, v20, 0.0f, 0.0f, 20, 8, 20, 4.0f, 2.0f, 9.0f, 1.0f, -1, 15.0f, 1, 1, -1, 0);
            t = func_001F9850(*(int *)&D_L05_00161C34);
            *(short *)(data + 0x19E) = t;
            func_001F9BC0(v30);
            moby[0x20] = 3;
            v30[2] = *(float *)(data + 0x194);
            zv = (float *)D_L05_0015F660;
            func_L00_00265050((char *)moby, 0x6DD, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6DF, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6E0, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6FE, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6FE, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6FF, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6FF, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            func_L00_00265050((char *)moby, 0x6FF, (float *)(moby + 0x10), moby + 0x40, 0, 0, zv, zv, 0.0f, zv);
            return;
        }
        func_L00_0025E4B0(moby, (short *)(data + 0x60));
    }
    moby[0xA4] = 0xFF;
    p184 = *(char **)(data + 0x184);
    if (p184 != 0) {
        p184[0xA4] = 0xFF;
    }
    p188 = *(char **)(data + 0x188);
    if (p188 != 0) {
        p188[0xA4] = 0xFF;
    }
    if (*(int *)(data + 0x38) != 0 && *(int *)(data + 0x178) == 0) {
        *(int *)(data + 0x178) = func_001F9850(0xF0);
        *(int *)(data + 0x38) = 0;
        g = D_0013E633 + 0xE1D;
        t7f = func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(moby + 0x10), *(float *)(g + 0x84) - *(float *)(moby + 0x14));
        t7f = func_001FA850(*(float *)(moby + 0x48), t7f);
        if (1.5707964f < t7f) {
            *(signed char *)(data + 0xC4) = -*(signed char *)(data + 0xC4);
        }
    }
    t = func_001F9908_v(data + 0x178);
    if (t == 0) {
        *(float *)(data + 0x174) = 14.0f;
    } else {
        *(float *)(data + 0x174) = 20.0f;
    }
    p184 = *(char **)(data + 0x184);
    if (p184 != 0) {
        t7f = func_001F9CB8(data + 0x40);
        *(float *)(p184 + 0x58) = t7f / (*(float *)&D_L05_00161BB0 * D_0015EE6C);
        *(float *)(p184 + 0x58) = *(float *)(p184 + 0x58) + *(float *)(data + 0x84) * *(float *)&D_L05_00161BB4;
        qcopy(p184 + 0x40, moby + 0x40);
        qcopy(p184 + 0x10, moby + 0x10);
    }
    p188 = *(char **)(data + 0x188);
    if (p188 != 0) {
        t7f = func_001F9CB8(data + 0x40);
        *(float *)(p188 + 0x58) = t7f / (*(float *)&D_L05_00161BB0 * D_0015EE6C);
        *(float *)(p188 + 0x58) = *(float *)(p188 + 0x58) - *(float *)(data + 0x84) * *(float *)&D_L05_00161BB4;
        qcopy(p188 + 0x40, moby + 0x40);
        qcopy(p188 + 0x10, moby + 0x10);
    }
    func_L00_0025E590(moby, data + 0x60);
}
