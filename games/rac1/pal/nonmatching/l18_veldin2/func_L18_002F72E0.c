/* NON_MATCHING func_L18_002F72E0 -- src/overlays/l18_veldin2/vendor_002F2AE0.c
 * Best so far: BYTES 135/2548 (94.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 2 (11 of 14 runs used)
 *   Retail asm: k=n+1 -> sll $4,k,2 ; sw k,0x34C ; addu $3,d,$4 ; lw 0x260($3) ; daddu $4,$3,$0 ; ... lw 0x204($4)
 *   Tried (all SIZE 2544, same 7-hunk diff as p10 unless noted):
 *   - p12: separate expressions d+k*4+0x260 and d+(n+1)*4+0x204: size 2548 but gcc folds second to n*4+0x208 off a
 *   - p13: both with k (same as p10). p14: e2=(char*)(long)e. p15: ((int*)d)[k+0x98]/[n+1+0x81] (folds like p12). 
 *   No form produced the register copy of e into a second pseudo ($a0); gcc CSE/copy-prop removes any plain-C alia
 *   ## Round 3 (5 of 16 runs used)
 *   All SIZE 2544 (no change vs p10): p19 distinct pointer q=d+(n+1)*4 for 0x204 (CSE'd into e); p20 same with the
 */
extern char D_0013E633[];
extern unsigned char D_001414F5[];
extern unsigned char *D_L18_00160058 MACRO_ADDR;
extern short D_L18_00160058_g __asm__("D_L18_00160058");
extern char *D_L18_0016016C MACRO_ADDR;
extern int *D_L18_001B11B0[];
extern short D_L18_001623C0;
extern short D_L18_00162414;
extern short D_L18_00162418;
extern short D_L18_0016241C;

extern void func_L18_00267000_v(void *, float) __asm__("func_L18_00267000");
extern char *func_L00_0025B478(void *, int, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, void *, void *, int, int);
extern void func_00213DE0(void *, int, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern void func_001FFDA0(int, int);
extern int func_001F9850(int);
extern int func_L00_0028EB98(void *, int);
extern void func_L00_0028EBF0(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_0025E590(void *, void *);
extern char *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_0020DAF8(void *, int, void *);
extern void func_001FA480(void *, void *);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_00251E30(void *);
extern int func_L00_00260FB0(float a, char *m, char *b, int c, int d, int *e, int f);
extern int func_L18_002D9440(char *);
extern char *func_L18_002DD7D0_v(void *, void *, float) __asm__("func_L18_002DD7D0");
extern void func_L00_00299B68(int);
extern void func_L18_002F30C8_v(void *) __asm__("func_L18_002F30C8");
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF500(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BC0(void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_00214358(void *, int, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L18_002E0E90_v(char *, void *, void *, void *, void *, void *, int, int, int) __asm__("func_L18_002E0E90");
extern int func_L18_002D7F48(void *, char *, int);

void func_L18_002F72E0(char *moby) {
    char *d = *(char **)(moby + 0x78);
    int res;
    float step;
    char *t;
    int crossed = 0;
    float f20;
    char *p;
    float V10[16];
    float V50[4];
    float V60[4];
    float V70[4];
    float V80[4];
    float V90[4];
    float VA0[4];

    *(float *)(moby + 0x2C) = *(float *)(*(char **)(moby + 0x24) + 0x24) * *(float *)&D_L18_001623C0;
    func_L18_00267000_v(moby, 0.5f);
    f20 = (7.0f - (float)*(int *)(d + 0x34C) - 2.0f) / 7.0f * 350.0f;
    step = 0.0f;
    t = func_L00_0025B478(moby, 0x330000, 0);
    if (t != 0) {
        char *o = *(char **)(t + 0x20);
        if (o != 0) {
            short h = *(short *)(o + 0xA6);
            if (h == 0x270 || h == 0x234 || (h == 0x238 && *(unsigned char *)(o + 0xBC) == 0)) {
                t = 0;
            }
        }
    }
    func_L00_0025B4D0(moby, t, d + 0x20, 0, &res, &step, 0, 4);
    if (res != 1) {
        int type = *(unsigned char *)(moby + 0x20);
        if ((type >= 0xC && type <= 0x13) || type == 2) {
            char *o = *(char **)(t + 0x20);
            if (o != 0) {
                short h = *(short *)(o + 0xA6);
                if (h != 0xB1) {
                    if (h == 0x131) {
                        step = step * 0.666f;
                    }
                } else {
                    step = step * 3.0f;
                }
            }
            if (*(int *)(D_0013E633 + 0x2EA9) == 0xF) {
                *(float *)(d + 0x390) = *(float *)(d + 0x390) + step;
            } else {
                *(float *)(d + 0x394) = *(float *)(d + 0x394) + step;
            }
            if (step != 0.0f) {
                float v;
                if (*(unsigned char *)(moby + 0x20) == 2) {
                    if (t != 0 && (*(int *)(t + 0x24) & 0x80000) && *(unsigned char *)(moby + 0x53) != 0xE) {
                        func_00213DE0(moby, 0xE, 5, 1);
                    }
                }
                if (f20 <= *(float *)(d + 0x38C) && *(float *)(d + 0x38C) - step < f20) {
                    crossed = 1;
                }
                v = *(float *)(d + 0x38C) - step;
                *(float *)(d + 0x38C) = v;
                if (v < f20) {
                    *(float *)(d + 0x38C) = f20;
                }
                d[0x67] = 0xF0;
                func_L00_0025E4B0(moby, (short *)(d + 0x60));
                if (*(float *)(d + 0x38C) == 0.0f) {
                    if (*(int *)(d + 0x368) != -1) {
                        func_001FFDA0(*(int *)(d + 0x368), 0);
                        *(int *)(d + 0x368) = -1;
                    }
                    moby[0x20] = 0x1A;
                    if (*(unsigned char *)(moby + 0x53) != 0) {
                        func_00213DE0(moby, 0, 0, func_001F9850(0x14));
                    }
                    if (func_L00_0028EB98(moby, *(int *)(d + 0x3B0)) != 0) {
                        int i = *(int *)(d + 0x3B0);
                        if (i != -1) {
                            char *e = D_0013E633 + 0x1D + i * 0x70;
                            if (*(char **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                                func_L00_0028EBF0(i);
                            }
                        }
                        *(int *)(d + 0x3B0) = -1;
                    }
                    *(int *)(d + 0x358) = 0;
                    crossed = 0;
                }
            }
        }
    }
    moby[0xA4] = 0xFF;
    *(int *)(d + 0x36C) = func_001FA898_r(*(float *)(d + 0x38C));
    func_L00_0025E590(moby, d + 0x60);
    if (*(int *)(d + 0x3A0) == 0) {
        p = func_0020D348_m(0x58F);
        *(char **)(d + 0x3A0) = p;
        p[0x30] = 0xFF;
        (*(char **)(d + 0x3A0))[0x31] = 1;
        func_00213DE0(*(char **)(d + 0x3A0), 1, 0, 0);
    }
    *(short *)(*(char **)(d + 0x3A0) + 0x32) = *(short *)(moby + 0x32);
    *(unsigned short *)(*(char **)(d + 0x3A0) + 0x34) = *(unsigned short *)(moby + 0x34) | 0x100;
    func_0020DAF8(moby, 4, V10);
    func_001FA480(*(char **)(d + 0x3A0) + 0xC0, V10);
    func_L00_00250800(moby, 4, *(char **)(d + 0x3A0) + 0x10);
    func_L00_00251E30(*(char **)(d + 0x3A0));
    if (*(int *)(d + 0x3A4) == 0) {
        p = func_0020D348_m(0x4E1);
        *(char **)(d + 0x3A4) = p;
        p[0x30] = 0xFF;
        (*(char **)(d + 0x3A4))[0x31] = 1;
        func_00213DE0(*(char **)(d + 0x3A4), 1, 0, 0);
    }
    *(short *)(*(char **)(d + 0x3A4) + 0x32) = *(short *)(moby + 0x32);
    *(unsigned short *)(*(char **)(d + 0x3A4) + 0x34) = *(unsigned short *)(moby + 0x34) | 0x100;
    func_0020DAF8(*(char **)(d + 0x3A0), 0, V10);
    func_001FA480(*(char **)(d + 0x3A4) + 0xC0, V10);
    func_L00_00250800(*(char **)(d + 0x3A0), 0, *(char **)(d + 0x3A4) + 0x10);
    func_L00_00251E30(*(char **)(d + 0x3A4));
    {
        int *e = D_L18_001B11B0[*(int *)(d + 0x218)];
        func_L00_00260FB0(512.0f, moby, d + 0x70, 0, 0, e + 4, e[0]);
    }
    if (*(int *)(d + 0xB0) == 0) {
        *(int *)(d + 0xB0) = *(int *)(D_0013E633 + 0x2E9D);
    }
    {
        int type = *(unsigned char *)(moby + 0x20);
        if (type >= 0xC && type <= 0x13) {
            int r = func_L18_002D9440((char *)*(int *)&D_L18_00160058 + (*(int *)(d + 0x21C) << 8));
            if (r == 1) {
                moby[0x20] = 0x13;
                if (*(unsigned char *)(moby + 0x53) != 1) {
                    func_00213DE0(moby, 1, 0, func_001F9850(0x14));
                }
                *(int *)(d + 0x358) = func_001F9850(0x258);
                *(int *)(d + 0x354) = func_001F9850(0x14);
                *(int *)(d + 0x364) = 0;
                *(char **)(d + 0x338) = func_L18_002DD7D0_v(moby, d + 0x3C0, 5.8f);
            } else if (r == -1) {
                *(unsigned short *)(moby + 0x34) |= 0x41;
                moby[0x20] = 7;
                func_L00_00299B68(7);
                *(int *)(d + 0x39C) = 0x19;
            }
        }
    }
    if (crossed != 0 || *(int *)&D_L18_0016241C != 0) {
        if (*(int *)(D_001414F5 + 0x203) != 0) {
            if (*(int *)&D_L18_0016241C != 0) {
                int i;
                *(int *)(d + 0x34C) = 0;
                for (i = 0; i < 4; i++) {
                    func_L18_002F30C8_v(D_L18_00160058 + (*(int *)(d + 0x230 + i * 4) << 8));
                }
                *(int *)&D_L18_0016241C = 0;
            }
            if (*(int *)(d + 0x34C) < 2) {
                int n;
                int k;
                int j;
                char *e;
                int *w;
                float ang;
                float *P;
                char *base;
                moby[0x20] = 3;
                P = (float *)(d + 0x300);
                n = *(int *)(d + 0x34C);
                k = n + 1;
                                *(int *)(d + 0x34C) = k;
                w = D_L18_001B11B0[*(int *)(d + k * 4 + 0x260)];
                *(int **)(d + 0x340) = w;
                *(int *)(d + 0x348) = 1;
                *(int *)(d + 0x398) = *(int *)(d + (n + 1) * 4 + 0x204);
                *(int *)(d + 0x374) = 0;
                qcopy(P, (char *)w + 0x10);
                base = d + 0x250;
                func_001F9BF0(P, P, D_L18_0016016C + (*(int *)(base + n * 4) << 7) + 0x30);
                func_L00_001FF500(P, P, 25.0f);
                *(int *)(d + 0x308) = 0;
                j = *(int *)(d + 0x34C) - 1;
                func_001F9BD8(P, P, D_L18_0016016C + (*(int *)(base + j * 4) << 7) + 0x30);
                func_001F9BC0(d + 0x310);
                {
                    char *r;
                    j = *(int *)(d + 0x34C) - 1;
                    r = D_L18_0016016C + (*(int *)(base + j * 4) << 7);
                    ang = func_L00_001FF860(*(float *)(r + 0x30) - P[0], *(float *)(r + 0x34) - P[1]);
                    *(float *)(d + 0x318) = ang;
                }
                qcopy(V50, P);
                f20 = 20.0f;
                V50[0] += func_001F9F90(ang) * f20;
                V50[1] += func_001F9FA8(*(float *)(d + 0x318)) * f20;
                V50[2] += 3.0f;
                j = *(int *)(d + 0x34C) - 1;
                qcopy(V90, D_L18_0016016C + (*(int *)(d + 0x228 + j * 4) << 7) + 0x30);
                V90[2] += f20;
                V90[2] = func_00214358(V90, 0, 0.5f);
                qcopy(V60, V90);
                j = *(int *)(d + 0x34C) - 1;
                func_001F9BF0(VA0, D_L18_0016016C + (*(int *)(base + j * 4) << 7) + 0x30, V60);
                VA0[2] = 0;
                func_L00_001FF4B0(VA0, VA0, -15.0f);
                func_001F9BD8(V60, V60, VA0);
                V60[2] += 15.0f;
                V90[3] = func_L00_001FF860(V90[0] - V60[0], V90[1] - V60[1]);
                func_001F9BC0(V70);
                V70[2] = func_L00_001FF860(*(float *)(d + 0x300) - V50[0], *(float *)(d + 0x304) - V50[1]);
                V70[1] = -0.17453292f;
                func_001F9BC0(V80);
                {
                    float *q = *(float **)(d + 0x340);
                    V80[2] = func_L00_001FF860(q[4] - V60[0], q[5] - V60[1]);
                }
                V80[1] = 0.17453292f;
                func_L18_002E0E90_v(*(char **)(d + 0x334), V50, V60, V70, V80, V90, *(int *)&D_L18_00162414, *(int *)&D_L18_00162418, 0);
            } else {
                moby[0x20] = 0x15;
                if (func_L00_0028EB98(moby, *(int *)(d + 0x3B0)) != 0) {
                    int i = *(int *)(d + 0x3B0);
                    if (i != -1) {
                        char *e = D_0013E633 + 0x1D + i * 0x70;
                        if (*(char **)(e + 0x88) == moby && *(unsigned char *)(e + 0x74) != 0) {
                            func_L00_0028EBF0(i);
                        }
                    }
                    *(int *)(d + 0x3B0) = -1;
                }
                if (*(unsigned char *)(moby + 0x53) != 0) {
                    func_00213DE0(moby, 0, 0, func_001F9850(0x14));
                }
                *(int *)(d + 0x330) = func_L18_002D7F48(d + 0x3C0, D_0013E633 + 0xE9D, *(int *)(d + 0x200));
                *(int *)(d + 0x394) = 0;
                *(int *)(d + 0x390) = 0;
            }
        }
    }
}
