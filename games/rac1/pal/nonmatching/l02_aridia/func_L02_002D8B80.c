/* NON_MATCHING func_L02_002D8B80 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: BYTES 8/3696 (99.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   run 12: p10 SIZE 3700 (nested ang arg in case 11 made it worse, reverted).
 *   run 13: p11 BYTES 74: 65050 called through alias with scale before v10 (retail arg order), case 12 stores 58,5
 *   run 14-15: p12 BYTES 97, p13 BYTES 83: separate a/b float locals in case 7 restore f20..f23 (merged f with lon
 *   run 16-17: p14 BYTES 59 (MACRO_ADDR on D_L02_0015F660 array makes case 17 exact), p15 BYTES 51 (case 12: (59,5
 *   run 18-19: p16 BYTES 24 (case 12 exact with (58,59,5A) order; case 7 tp local before mul), p17 same (function-
 *   run 20: p18 BYTES 8 (store b * r straight to memory in case 7 fixes lw/mul order). BUDGET SPENT.
 *   Left: case 11 (state 11): after func_L00_001FF860, retail emits `mov.s $f12,$f0` before `addiu $a1,$s5,0x214`;
 *   Idioms: `case 0: default:` merges to give retail's lopsided switch tree; D_L02_0015F660 array needs MACRO_ADDR
 */
extern void func_L00_00264B40(float x, int a, int b, unsigned char *m);
extern float func_001F9D10(void *, void *);
extern void func_L00_0025B178(void *);
extern void func_L02_002D99F0(void *);
extern int func_001160D8(void);
extern void func_L01_0026E8E0(char *p);
extern void func_001F9BC0(void *);
extern void func_L02_00265E58(char *moby);
extern void func_00213DE0(void *, int, int, int);
extern float func_L00_001FF860(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_002607A8(void *a, float x);
extern void func_001F9BD8(void *, void *, void *);
extern float func_00214358(void *, int, float);
extern float func_00214D28(float *p, float target, float maxstep);
extern int func_001F9850(int);
extern int func_L00_0025A060(char *m, char *p, float *pos, char *v);
extern float func_001F9B88(float);
extern float func_001FA850(float, float);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_002140F8(float, float);
extern float func_001F9FA8(float);
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern int func_00215570(void *arg0, int arg1);
extern void func_L02_00265E88(float, void *, void *, void *);
extern void func_L00_0025A8C0(char *arg, int a, int b, void *src, float scale);
extern void func_L02_002661E8(char **slots, int a, int b);
extern int func_002140B0(int arg0);
extern float func_001F9D48(void *, void *);
extern int func_L00_0025D6F0(void *, void *);
extern void func_L00_002584A8(void *, int, int);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern void *func_L00_00265050_alt(char *src, int cls, float *pos, void *mat, int a8, int a9, float scale, float *v10, float *v11, float *v12) __asm__("func_L00_00265050");
extern void func_0020D678(void *);
extern char D_L02_00167440[];
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L02_0016016C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L02_0015F6B0 MACRO_ADDR;
extern float D_L02_0015F660[] MACRO_ADDR;
extern short D_L02_00161B14;
extern short D_L02_00161B28;
extern short D_L02_00161B20;
extern short D_L02_00161B24;
extern short D_L02_00161B18;
extern short D_L02_00161B10;
extern char D_0013E633[];

// UpdateMoby_612: a state machine for an Aridia enemy that turns toward the player, walks to its spawn spot and attacks.
void func_L02_002D8B80(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    char sp[0x50];
    func_L00_00264B40(2.5f, (int)m, 1, (unsigned char *)(d + 0x1A0));
    if (m[0x31] != 0) {
        if (func_001F9D10(m + 0x10, D_L02_00167440) < 30.0f) {
            func_L00_0025B178(m);
            m[0x7F] = 0x18;
        }
    }
    func_L02_002D99F0(m);
    switch (m[0x20]) {
    case 0: {
        float x;
        if (func_001160D8() & 1) {
            *(unsigned short *)(m + 0x34) |= 0x8000;
        }
        *(unsigned short *)(m + 0x34) |= 0x1000;
        d[0x5C] = 1;
        *(float *)(d + 0x30) = 1.0f;
        d[0x58] = 0xF;
        d[0x5A] = 8;
        d[0x29] = 0;
        *(long *)(m + 0x38) = *(long *)(*(char **)(D_0013E633 + 0x2E9D) + 0x38);
        func_L01_0026E8E0(d + 0xD0);
        x = D_0015EE6C;
        d[0x28] = 2;
        *(float *)(d + 0x10C) = 0.03f;
        *(float *)(d + 0x110) = 0.3f;
        *(float *)(d + 0x114) = 0.25f;
        *(float *)(d + 0xF4) = 4.0f * x;
        *(float *)(d + 0xDC) = 2.0f;
        *(float *)(d + 0xD8) = 2.0f;
        *(float *)(d + 0x20) = 3.0f;
        func_001F9BC0(d + 0x40);
        func_L02_00265E58(d + 0x1F0);
        switch (*(int *)(d + 0x194)) {
        case 0:
        default:
            if (m[0x53] != 0) {
                func_00213DE0(m, 0, 0, 0);
            }
            m[0x20] = 7;
            break;
        case 1:
            if (m[0x53] != 0) {
                func_00213DE0(m, 0, 0, 0);
            }
            m[0x20] = 10;
            break;
        case 2:
            if (m[0x53] != 2) {
                func_00213DE0(m, 2, 0, 0);
            }
            m[0x20] = 8;
            break;
        case 3:
            if (m[0x53] != 0xF) {
                func_00213DE0(m, 0xF, 0, 0);
            }
            m[0x20] = 9;
            break;
        }
        break;
    }
    case 2:
        if (m[0x70] & 2) {
            m[0x20] = 3;
            if (m[0x53] != 4) {
                func_00213DE0(m, 4, 0, 0);
            }
        }
        break;
    case 3: {
        float ang;
        float f;
        ang = func_L00_001FF860(*(float *)(*(char **)(d + 0x220) + 0x10) - *(float *)(m + 0x10), *(float *)(*(char **)(d + 0x220) + 0x14) - *(float *)(m + 0x14));
        func_L00_002592B0((char *)m, (float *)(d + 0x214), ang, 0.02f, 0.3f, D_0015EE6C * 1.5707964f);
        func_001F9BF0(sp, D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30, m + 0x10);
        *(float *)(sp + 8) = 0.0f;
        func_L00_002607A8(sp, D_0015EE6C * 6.0f);
        func_001F9BD8(m + 0x10, m + 0x10, sp);
        f = func_00214358(m + 0x10, 0, 0.5f);
        func_00214D28((float *)(d + 0x48), D_0015EE6C * 40.0f, *(float *)&D_L02_00161B14 * D_0015EE70);
        func_00214D28((float *)(m + 0x18), f, *(float *)(d + 0x48));
        if (*(float *)(m + 0x18) == f) {
            m[0x20] = 4;
            if (m[0x53] != 5) {
                func_00213DE0(m, 5, 4, func_001F9850(10));
            }
        }
        break;
    }
    case 4:
        if (m[0x70] & 2) {
            m[0x20] = 5;
            if (m[0x53] != 6) {
                func_00213DE0(m, 6, 0, func_001F9850(6));
            }
        }
        break;
    case 5:
        if (func_L00_0025A060((char *)m, d + 0xD0, (float *)(D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30), d + 0x40) == 4) {
            if (m[0x53] != 0) {
                func_00213DE0(m, 0, 0, func_001F9850(30));
            }
            m[0x20] = 7;
        }
        break;
    case 6:
        if (func_L00_0025A060((char *)m, d + 0xD0, (float *)(d + 0x1E0), d + 0x40) & 6) {
            if (m[0x53] != 0) {
                func_00213DE0(m, 0, 0, func_001F9850(10));
            }
            m[0x20] = 7;
        } else if (func_001F9D10(*(char **)(d + 0x220) + 0x10, D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30) < *(float *)(d + 0x180)) {
            if (func_001F9B88(*(float *)(*(char **)(d + 0x220) + 0x18) - *(float *)(m + 0x18)) < 4.0f) {
                m[0x20] = 5;
                if (m[0x53] != 6) {
                    func_00213DE0(m, 6, 0, func_001F9850(10));
                }
            }
        }
        break;
    case 7: {
        char *tp;
        float r;
        float a;
        float b;
        float ang;
        float t;
        float f;
        if (func_001F9D10(*(char **)(d + 0x220) + 0x10, D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30) < *(float *)(d + 0x180) && func_001F9B88(*(float *)(*(char **)(d + 0x220) + 0x18) - *(float *)(m + 0x18)) < 4.0f) {
            if (func_001F9D10(m + 0x10, D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30) < 1.0f) {
                ang = func_L00_001FF860(*(float *)(*(char **)(d + 0x220) + 0x10) - *(float *)(m + 0x10), *(float *)(*(char **)(d + 0x220) + 0x14) - *(float *)(m + 0x14));
                func_L00_002592B0((char *)m, (float *)(d + 0x214), ang, 0.02f, 0.3f, 0.1f);
                if (func_001FA850(*(float *)(m + 0x48), ang) < 0.08726646f) {
                    m[0x20] = 0xB;
                    if (m[0x53] != 8) {
                        func_00213DE0(m, 8, 0, func_001F9850(6));
                    }
                }
            } else {
                m[0x20] = 5;
                if (m[0x53] != 6) {
                    func_00213DE0(m, 6, 0, func_001F9850(10));
                }
            }
        } else if (m[0x70] & 2) {
            t = func_00214158();
            a = func_001F9F90(t);
            a = a * func_002140F8(1.0f, 2.0f);
            *(float *)(d + 0x1E0) = a;
            b = func_001F9FA8(t);
            r = func_002140F8(1.0f, 2.0f);
            tp = D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30;
            *(float *)(d + 0x1E4) = b * r;
            *(int *)(d + 0x1E8) = 0;
            func_001F9BD8(d + 0x1E0, d + 0x1E0, tp);
            m[0x20] = 6;
            if (m[0x53] != 6) {
                func_00213DE0(m, 6, 0, func_001F9850(10));
            }
        }
        break;
    }
    case 8:
    case 9:
        if ((D_L02_0015F6B0 & 3) == 0) {
            if (m[0x31] != 0) {
                float k;
                float t;
                int r;
                float *dir;
                k = func_002140F8(0.0f, *(float *)&D_L02_00161B28) * D_0015EE6C;
                t = func_00214158();
                *(float *)(sp + 0x10) = func_001F9F90(t) * k;
                *(float *)(sp + 0x14) = func_001F9FA8(t) * k;
                *(float *)(sp + 0x18) = 0.0f;
                func_L00_00250800(m, 0, sp);
                dir = (float *)(sp + 0x10);
                r = func_001F9850(*(int *)&D_L02_00161B20);
                func_L00_0026DA50(sp, dir, *(int *)&D_L02_00161B18, *(int *)&D_L02_00161B18, func_L00_00258BC8(r, func_001F9850(*(int *)&D_L02_00161B24)), 1, 40000.0f);
            }
        }
    case 10: {
        int s;
        s = 0;
        if (*(int *)(d + 0x18C) >= 0) {
            s = func_00215570(D_0013E633 + 0x10BD, *(int *)(d + 0x18C)) != 0;
        } else if (func_001F9D10(D_0013E633 + 0xE9D, D_L02_0016016C + (*(int *)(d + 0x188) << 7) + 0x30) < *(float *)(d + 0x184)) {
            s = 1;
        }
        if (s != 0) {
            if (m[0x20] == 8) {
                if (m[0x53] != 4) {
                    func_00213DE0(m, 4, 0, func_001F9850(10));
                }
                m[0x20] = 2;
            } else if (m[0x20] == 9) {
                if (m[0x53] != 0x10) {
                    func_00213DE0(m, 0x10, 0, func_001F9850(10));
                }
                m[0x20] = 2;
            } else {
                m[0x20] = 5;
                if (m[0x53] != 6) {
                    func_00213DE0(m, 6, 0, func_001F9850(6));
                }
            }
        }
        break;
    }
    case 11: {
        float ang;
        *(float *)(m + 0x58) = 2.0f;
        ang = func_L00_001FF860(*(float *)(*(char **)(d + 0x220) + 0x10) - *(float *)(m + 0x10), *(float *)(*(char **)(d + 0x220) + 0x14) - *(float *)(m + 0x14));
        func_L00_002592B0((char *)m, (float *)(d + 0x214), ang, 0.02f, 0.3f, 0.1f);
        if (m[0x70] & 2) {
            *(float *)(m + 0x58) = 1.0f;
            m[0x20] = 0xC;
            if (m[0x53] != 9) {
                func_00213DE0(m, 9, 0, func_001F9850(6));
            }
        }
        break;
    }
    case 12:
        *(float *)(m + 0x58) = 0.5f;
        func_L00_00250800(m, 0, sp);
        *(float *)(sp + 0x10) = func_001F9F90(func_L00_001FF860(*(float *)sp - *(float *)(m + 0x10), *(float *)(sp + 4) - *(float *)(m + 0x14)));
        *(float *)(sp + 0x14) = func_001F9FA8(func_L00_001FF860(*(float *)sp - *(float *)(m + 0x10), *(float *)(sp + 4) - *(float *)(m + 0x14)));
        *(float *)(sp + 0x18) = -0.1f;
        func_L02_00265E88(*(float *)&D_L02_00161B10, d + 0x1F0, sp, sp + 0x10);
        *(float *)(sp + 0x1C) = 5627.925f;
        *(float *)(sp + 0x18) = 1.0f;
        func_L00_0025A8C0(sp + 0x20, (int)m, 0x10001, sp + 0x10, 1.0f);
        sp[0x38] = 5;
        sp[0x39] = 1;
        *(unsigned short *)(sp + 0x3A) = *(unsigned short *)(m + 0xA6);
        func_L02_002661E8((char **)(d + 0x1F0), (int)m, (int)(sp + 0x20));
        if (m[0x70] & 2) {
            *(float *)(m + 0x58) = 1.0f;
            m[0x20] = 0xD;
            if (func_002140B0(0xFF) & 1) {
                if (m[0x53] != 0xA) {
                    func_00213DE0(m, 0xA, 0, func_001F9850(6));
                }
            } else if (m[0x53] != 8) {
                func_00213DE0(m, 8, 0, func_001F9850(6));
            }
        }
        break;
    case 13: {
        float ang;
        ang = func_L00_001FF860(*(float *)(*(char **)(d + 0x220) + 0x10) - *(float *)(m + 0x10), *(float *)(*(char **)(d + 0x220) + 0x14) - *(float *)(m + 0x14));
        func_L00_002592B0((char *)m, (float *)(d + 0x214), ang, 0.02f, 0.3f, 0.1f);
        if (m[0x70] & 2) {
            if (func_001F9D48(m + 0x10, *(char **)(d + 0x220) + 0x10) < *(float *)(d + 0x180) + 1.0f
                && func_001F9B88(*(float *)(*(char **)(d + 0x220) + 0x18) - *(float *)(m + 0x18)) < 3.0f) {
                m[0x20] = 0xC;
                if (m[0x53] != 9) {
                    func_00213DE0(m, 9, 0, func_001F9850(6));
                }
            } else {
                m[0x20] = 7;
                if (m[0x53] != 0) {
                    func_00213DE0(m, 0, 0, func_001F9850(6));
                }
            }
        }
        break;
    }
    case 16:
        if (func_L00_0025D6F0(m, d + 0x70) & 0x40) {
            m[0x20] = 7;
            if (m[0x53] != 0) {
                func_00213DE0(m, 0, 0, func_001F9850(6));
            }
        }
        break;
    case 17:
        if (func_L00_0025D6F0(m, d + 0x70) & 0x140) {
            char *mat;
            float *v;
            qcopy(sp, m + 0x10);
            v = D_L02_0015F660;
            mat = (char *)(m + 0x40);
            *(float *)(sp + 8) += 1.0f;
            func_L00_002584A8(m, 0, -1);
            func_L00_0025F4A8(m, d + 0x40, sp, 0.0f, 0.0f, 5, 2, 4, 2.0f, 1.0f, 9.0f, -1, 1.0f, 15.0f, 1, 1, -1, 0);
            func_L00_00265050_alt((char *)m, 0x6D7, (float *)(m + 0x10), mat, 0, 0, 0.0f, v, v, v);
            func_L00_00265050_alt((char *)m, 0x6D8, (float *)(m + 0x10), mat, 0, 0, 0.0f, v, v, v);
            func_L00_00265050_alt((char *)m, 0x6D9, (float *)(m + 0x10), mat, 0, 0, 0.0f, v, v, v);
            func_L00_00265050_alt((char *)m, 0x6E9, (float *)(m + 0x10), mat, 0, 0, 0.0f, v, v, v);
            func_L00_00265050_alt((char *)m, 0x782, (float *)(m + 0x10), mat, 0, 0, 0.0f, v, v, v);
            func_0020D678(m);
            return;
        }
        break;
    }
    func_L00_0025B178(m);
}
