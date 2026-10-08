/* NON_MATCHING func_L18_002D8F10 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 64/1092 (94.1% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern float func_00214D88(float *, float *, float, float, float, float);
extern int func_0022ED80__s(int, int, int) __asm__("func_0022ED80");
extern void func_L00_0028EBF0(int);
extern void func_001F49B0(void (*)(void), void *);
extern int func_001F9908(int *);
extern float func_001F9D48(void *, void *);
extern int func_001F9850(int);
extern int func_L00_00203F20(int a, int b);
extern float func_00214D28(float *, float, float);
extern void func_L18_002D9488(char *);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_0015EE84;
extern int D_0015EE84_far __asm__("D_0015EE84") NOT_SDA;
extern int D_0015EE84_far2 __asm__("D_0015EE84") NOT_SDA;
extern unsigned char D_0013D50F NOT_SDA;
extern short D_0015EFA4;
extern short D_L18_00161AA0;
extern short D_L18_00161AA4;
extern short D_L18_00161AA8;
extern short D_L18_00162420;
extern char D_0013E633[];
extern char D_0013E650[];
extern char D_0014171B[];

typedef struct {
    char pad0[0x3E];
    unsigned short flags;
    char pad40[0x20];
    int f60;
    float f64;
    float vec[1];
    char pad6C[4];
    int idx;
} Data;

typedef struct {
    char pad0[0x18];
    float x;
    char pad1C[8];
    unsigned char state;
} dummy_unused;

void func_L18_002D8F10(unsigned char *moby) {
    Data *data = *(Data **)(moby + 0x78);
    switch (moby[0x20]) {
    case 0: {
        data->f64 = *(float *)(moby + 0x18);
        data->flags |= 8;
        if (data->f60) {
            moby[0x20] = 1;
            *(float *)(moby + 0x2C) = *(float *)(*(char **)(moby + 0x24) + 0x24) * *(float *)&D_L18_00161AA0;
        } else {
            moby[0x20] = 4;
        }
        break;
    }
    case 1:
    case 3:
        func_00214D88((float *)(moby + 0x18), data->vec, data->f64 + *(float *)&D_L18_00161AA8,
                      D_0015EE70 * 20.0f, D_0015EE70 * 40.0f, D_0015EE6C * 20.0f);
        break;
    case 2: {
        char *t;
        func_00214D88((float *)(moby + 0x18), data->vec, data->f64 + *(float *)&D_L18_00161AA4,
                      D_0015EE70 * 5.0f, D_0015EE70 * 10.0f, D_0015EE6C * 20.0f);
        t = D_0013E633 + 0xE1D;
        if (*(short *)(t + 0x30E) == 0 && *(unsigned char **)(t + 0x2FC) == moby && *(int *)(t + 0x2084) == 0x22) {
            int idx;
            moby[0x20] = 3;
            func_0022ED80__s(0, 0, (int)moby);
            idx = data->idx;
            if (idx != -1) {
                unsigned char *e = (unsigned char *)D_0013E650 + idx * 0x70;
                if (*(unsigned char **)(e + 0x88) == moby && e[0x74]) {
                    func_L00_0028EBF0(idx);
                }
            }
            data->idx = -1;
        } else {
          char *t2 = D_0013E633 + 0xE1D;
          if (*(int *)(t2 + 0x2084) != 0x72) {
            func_001F49B0((void (*)(void))func_L18_002D9488, moby);
            if (func_001F9908(&data->pad6C[0] ? (int *)data->pad6C : 0)) {
                *(int *)&D_L18_00162420 = 1;
            }
          }
        }
        break;
    }
    case 4: {
        char *t = D_0013E633 + 0xE1D;
        if (*(short *)(t + 0x30E) == 0 && *(unsigned char **)(t + 0x2FC) == moby && *(int *)(t + 0x2084) == 0x22) {
            int d;
            moby[0x20] = 5;
            moby[0xBC] = 1;
            d = D_0015EE84_far;
            *(float *)(moby + 0x18) = *(float *)(moby + 0x18) - 1.5f;
            if (d == 0x12) {
                D_0013D50F = 1;
            }
        }
        if (func_001F9D48(moby + 0x10, D_0013E633 + 0xE9D) < 2.5f) {
            char *q = (char *)(data = (Data *)(D_0014171B + 0x34D));
                        int bit = 1 << D_0015EE84_far;
            if (!(*(int *)(q + 0x15814) & bit)) {
                int a = *(int *)&D_0015EFA4;
                if (*(unsigned short *)(q + 0x388) != 0) {
                    int y = func_001F9850(a);
                    int v = y - *(unsigned short *)(q + 0x38A) * 600;
                    if ((int)((float)func_001F9850(0x12) * 60.0f) < v || *(unsigned short *)(q + 0x38A) * 600 == 0) {
                        func_L00_00203F20(0x2B02, 0x71);
                    } else if (func_001F9850(*(int *)&D_0015EFA4) / 600 > *(unsigned short *)(q + 0x38A)) {
                        *(unsigned short *)(q + 0x38A) = func_001F9850(*(int *)&D_0015EFA4) / 600;
                    }
                } else {
                    (*(unsigned short *)(q + 0x388))++;
                    {
                        int b, r = func_001F9850(a) / 600;
                        b = *(int *)&D_0015EE84;
                        if (r > *(unsigned short *)(q + 0x38A)) {
                            *(unsigned short *)(q + 0x38A) = func_001F9850(*(int *)&D_0015EFA4) / 600;
                            b = D_0015EE84_far2;
                        }
                        *(unsigned int *)(q + 0x38C) = *(unsigned int *)(q + 0x38C) | 0x80000000 | (1 << b);
                    }
                }
            }
        }
        break;
    }
    case 5:
        if (func_00214D28((float *)(moby + 0x18), data->f64 - 1.5f, 0.15f) == 0.0f) {
            moby[0x20] = 6;
        }
        break;
    }
}
