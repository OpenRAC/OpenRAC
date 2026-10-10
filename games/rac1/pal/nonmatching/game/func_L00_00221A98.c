extern unsigned char D_0013E633[] NOT_SDA;
extern char D_L00_00173F40[] NOT_SDA;
extern char D_L00_00173F60[] NOT_SDA;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern void func_L00_00213F38(void);
extern int func_L00_0020A8B8(int, float, float);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern int func_L00_0025F410(void *);
extern void func_L00_00233EE0(void *, float, float, float);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern float func_001F9D10(void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);

/* Hero move and collide step: while the hero's state allows it, probes the ground with two sphere
   queries (one either side of the hero) and records the height and the hit flags in the hero block. */
void func_L00_00221A98(void) {
    char *h = (char *)D_0013E633 + 0xE1D;
    char *p;
    float v1[4] __attribute__((aligned(16)));
    float v2[4] __attribute__((aligned(16)));
    float v3[4] __attribute__((aligned(16)));
    float ang;
    float len;
    float dist;
    float f12;
    float f20;
    float f21;
    float f22;
    float f23;
    float f24;
    float f25;
    int r18;
    int r2;

    if (*(unsigned char *)(h + 0x20AD) == 0) {
        func_L00_00213F38();
    }
    if (D_L00_0015F6B0 % 3 == 0) {
        *(float *)(h + 0x248) = 4.0f;
        *(unsigned char *)(h + 0x254) = 0;
        *(unsigned char *)(h + 0x255) = 0;
        if (func_L00_0020A8B8((int)(h + 0x248), 0.7f, 4.0f) != 0) {
            p = D_L00_00173F40;
            len = func_001F9CE8(p + 0x40);
            ang = func_L00_001FF860(*(float *)(p + 0x48), len);
            *(float *)(h + 0x250) = ang;
            if (*(int *)(p + 0x18) != 0) {
                *(unsigned char *)(h + 0x255) = 1;
                if (func_L00_0025F410((void *)*(int *)(p + 0x18)) != 0) {
                    *(unsigned char *)(h + 0x254) = 1;
                }
            }
        }
    }
    if (D_L00_0015F6B0 % 5 != 0) {
        return;
    }
    *(int *)(h + 0x260) = 0;
    *(int *)(h + 0x258) = 0;
    if (*(int *)(h + 0x300) == 0) {
        return;
    }
    f24 = *(float *)(h + 0x258);
    func_L00_00233EE0(v1, 1.1f, f24, 1.0f);
    func_L00_00233EE0(v2, 1.1f, f24, -20.0f);
    if (v2[2] < 0.5f) {
        v2[2] = 0.5f;
    }
    r18 = func_L00_001EFFF0(v1, v2, 2, *(int *)(h + 0x2080), 0);
    f22 = 20.0f;
    if (r18 != 0) {
        dist = func_001F9D10(v1, D_L00_00173F60);
        f22 = dist;
        if (!(3.0f < f22)) {
            return;
        }
    }
    f25 = 1.1f;
    f20 = -0.4f;
    f21 = f24;
    f24 = 0.1f;
    f23 = 0.5f;
    f12 = f20 + f25;
    for (;;) {
        func_L00_00233EE0(v3, f12, f21, f21);
        r2 = func_L00_001F10E0(*(float *)(h + 0x234), v3, 2, 0);
        if (r2 != 0) {
            f20 = f20 + f24;
            if (f20 < f23) {
                f12 = f20 + f25;
                continue;
            }
            return;
        }
        *(int *)(h + 0x260) = 1;
        if (r18 != 0) {
            *(float *)(h + 0x258) = f22;
        } else {
            *(float *)(h + 0x258) = 20.0f;
        }
        *(float *)(h + 0x25C) = *(float *)(h + 0x234) + f20;
        if (*(float *)(h + 0x25C) < f21) {
            *(int *)(h + 0x25C) = 0;
        }
        return;
    }
}
