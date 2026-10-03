/* NON_MATCHING func_L18_002F4050 -- src/overlays/l18_veldin2/vendor_002F2AE0.c
 * Best so far: BYTES 9006/12840 (29.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - Case 0x13 reads everything off the one VG base, as retail does: D_0013F9B0 is
 *   **VG + 0x560** and the player position **VG + 0x80** (c49, 105 hunks but 4
 *   bytes over; folded into c52).
 *   - Tried and a wash: flipping the two FP clamp comparisons to `120.0f < e` /
 *   `60.0f > e` to get retail's `c.lt.s`/`bc1f` instead of `c.le.s`/`bc1t` (c51,
 *   105 hunks) -- it fixes some sites and breaks others, so it has to be done per
 *   site, not globally.
 *   - Tried and worse: a local for the D_L18_0016D2E0 base in case 7 (c50, 108).
 */
/* func_L18_002F4050 -- UpdateMoby_1422 (level 18), 12840 bytes.
 * From build-sn/ghidra/l18/out/func_L18_002F4050.c, globals mapped back to
 * symbols ($gp = 0x166D00). WORK IN PROGRESS: see NOTES.md for what is done.
 */
/* The executable's globals in this range are one blob in the symbol list, so
   they are spelled as offsets from it, the way the rest of the level does
   (0x13F4D0, the player's position, is D_0013E633_u + 0xE9D). */
typedef struct { float v[4]; } __attribute__((aligned(16))) Q4050;
extern unsigned char D_0013E633_u[] __asm__("D_0013E633");
extern float D_L18_0015F4FC MACRO_ADDR;
extern int *D_L18_001B11B0[];
extern char *D_L18_0016016C MACRO_ADDR;
extern unsigned char *D_L18_00160058 MACRO_ADDR;
extern float D_L18_00167840[];
extern int D_L18_001623F0 MACRO_ADDR;
extern int D_L18_00162420 MACRO_ADDR;
extern int D_L18_00162424 MACRO_ADDR;
extern int D_L18_00162428 MACRO_ADDR;
extern int D_L18_00162430 MACRO_ADDR;
extern short D_L18_00162434;
extern int D_L18_0015F6A8 MACRO_ADDR;
/* Retail reads these two as fields of the state record at D_L18_0016D2E0
   (`lui $v0,0x17` / `addiu $s0,$v0,-0x2D20` then 0x30($s0) and 0x34($s0)),
   not as two globals of their own. */
extern int D_L18_0016D2E0_s[] __asm__("D_L18_0016D2E0");
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;

extern void func_L18_002F7278(char *moby);
extern void func_L18_002F72E0(char *moby);
/* The file declares this one `void`, but retail sets $a0 = moby at this call
   site, so it is reached through a one-argument alias. */
extern void f3f78_m(char *moby) __asm__("func_L18_002F3F78");
extern void func_L18_002F7F00(char *moby);
extern void func_L18_002F8408(int unused, int idx);
extern void func_L18_002F8488(int a0, int idx);
extern void func_L00_00264B40(float x, int a, int b, unsigned char *m);
extern void func_L00_0025B178(void *);
extern void func_L00_00299B68(int);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_L00_002512D8(int idx);
extern void func_L00_00286128(void *, void *);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_00211908(void);
extern void func_L15_002092E0(void);
extern void func_L18_002D8140(int idx);
extern void func_L18_002D81B0(void *moby);
extern void func_L18_002D9358(unsigned char *arg, int value);
extern void func_L18_002E0E90(char *moby, void *v0, void *v1, void *v2, void *v3,
                              void *v4, int a6, int a7, int a8);
extern void func_L00_0028EBF0(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F9BC0(float *);
extern void func_001F9BD8(void *, void *, void *);
extern char *func_L18_002D8070(void *pos, int idx);
extern char *func_L18_002DD7D0(void *owner, void *vector, float value);
extern char *func_L18_002EB4E0(void *owner, void *vector, int value);
extern int func_00215570(void *arg0, int arg1);
extern int func_00215B18(char *, float);
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_0025A778(void *, void *, int);
extern int func_L00_0028EB98(void *, int);
extern int func_L18_002E0F90(void *);
extern float func_L18_002E0F80(void *moby);
extern float func_001F9878(float);
extern float func_001F9B88(float);
extern float func_001F9D10(void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_002140F8(float, float);
extern float func_L00_00258C80(float lo, float hi);
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern char *func_L18_002D6738(float *pos, float *b, float *c, int d, int e,
                               float f1, float f2);
extern int func_L18_002F7CD8(char *moby, float arg, void *x);
extern unsigned char *func_L18_002F8270(char *self);
extern void func_L18_002F3038(unsigned char *moby);
extern void func_L18_002EB558(unsigned char *arg, void *src, void *position,
                              int active, float speed);
extern float func_00214358(void *arg0, int arg1, float arg2);
extern float func_00214D28(float *p, float target, float maxstep);
extern float func_001F9CB8(void *);
extern void func_001F9BF0(void *, void *, void *);
extern float D_L18_0015F660[];
extern void func_L00_001FF500(void *, void *, float);
/* The file's own prototype uses a typedef declared further down, so this one
   calls it through an alias with the same layout. */
extern void f8680_v(char *source, char *dest, void *from, void *to)
    __asm__("func_L18_002F8680");
extern float func_001FA888(int);
extern float func_001F9CE8(void *);
extern char *func_L18_002D70E8(float f, int a0, int idx, float *p6, float *p7,
                               int a8);
extern int func_L18_002FDB28(int idx, float *pos, float *dir, float speed);
extern int func_L18_002FDCA0(int idx);
extern void func_L18_002F7DC0(char *m);
extern void func_L18_002DD848(char *moby, void *pos, float f);
extern int func_0022ED80(int, int, int);
extern float D_L18_00162408_f __asm__("D_L18_00162408") MACRO_ADDR;
extern int func_L18_002F1420(char *moby, int idx);
extern void func_L18_002D93C0(unsigned char *arg);
extern void func_L18_002F86E8(char *moby);
extern void func_L00_002EBF50(void *, void *, int, int, int);
extern void func_L00_0029ADD8(void);
extern void func_L00_0029A8D0(int i);
extern void func_00219C70(int arg0);
extern void func_0020D678(void *);
extern void func_L00_00264BE8(void *, void *, void *, float, float);
extern void func_L00_00263950(char *, char *, int, float, float);
extern float func_00214D88(float, float, float, float, float *, float *);
extern int func_001FA8A8(int, int, float);
extern void func_001F49B0(void (*)(void), void *);
extern float D_0015EE64 MACRO_ADDR;
/* 18 arguments; the file's shared sibling spells it the same way. The two call
   sites' lists were read off the retail assembly, because Ghidra prints only
   the eight integer arguments that go in registers. */
extern void func_L00_0025F4A8_alt(void *, void *, void *, float, float, int, int,
                                  int, float, float, float, float, int, float,
                                  int, int, int, int) __asm__("func_L00_0025F4A8");
extern float D_L18_0016D2F0[];
extern float D_L18_0016D300[];
extern float func_00214158(void);
extern int D_L18_0016242C MACRO_ADDR;
extern void func_L18_002DCD28(char *pos, float *vec, int arg, float fa, float fb);
extern void func_L18_002F8B00(char *moby, void *p);
extern void *func_L18_002D7F48(float *a, float *b, int idx);
extern int func_L18_002F8518(char *moby);
extern int func_002140B0(int);

extern float func_001FA850(float, float);
extern char *func_L18_002DD7D0_v(void *owner, void *vector, float value) __asm__("func_L18_002DD7D0");

void func_L18_002F4050(unsigned char *moby) {
    char *d = *(char **)((char *)moby + 0x78);
    char *o;
    char *b;
    char *tgt = *(char **)(d + 0xB0);
    unsigned int st;
    int i;
    int *t;
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float v4[4];
    float slow;
    float ang;
    float *aim;
    float fast;

    func_L18_002F7278((char *)moby);
    func_L18_002F72E0((char *)moby);
    f3f78_m((char *)moby);
    o = *(char **)(d + 0x3A4);
    if (o != 0 && o[0x20] >= 0) {
        func_L00_00264B40(*(float *)&D_L18_00162434, (int)o, 0,
                          (unsigned char *)(d + 0x1C0));
    }
    if (moby[0x31] != 0 && func_001F9D10((char *)moby + 0x10, D_L18_00167840) < 32.0f) {
        func_L00_0025B178((char *)moby);
        moby[0x7F] = 0x1A;
    }
    st = moby[0x20];
    switch (st) {
    case 0:
        qcopy(d + 0x3C0, (char *)moby + 0x10);
        moby[0x20] = 1;
        moby[0x30] = 0xFF;
        *(short *)((char *)moby + 0x32) = 0x200;
        *(unsigned short *)((char *)moby + 0x34) |= 0x41;
        *(int *)((char *)moby + 0x94) = 0;
        t = (int *)(d + 0x204);
        d[0x28] = 3;
        *(float *)(d + 0x20) = 500.0f;
        *(float *)(d + 0x30) = 4.0f;
        i = 2;
        *(short *)(d + 0x24) = 500;
        o = (char *)D_L18_00160058 + *(int *)(d + 0x220) * 0x100;
        *(int **)(d + 0x340) = D_L18_001B11B0[*(int *)(d + 0x26C)];
        *(int *)(d + 0x398) = *(int *)(d + 0x204);
        *(int *)(d + 0x348) = 1;
        *(int *)(d + 0x368) = -1;
        *(float *)(d + 0x38C) = 350.0f;
        *(char **)(d + 0x334) = o;
        *(float *)(d + 0x3D0) = 0.75f;
        *(int *)(d + 0x374) = 0;
        for (; i >= 0; i--) {
            func_L18_002F8408((int)(char *)moby, *t++);
        }
        break;
    case 7:
        if (D_L18_0015F6A8 != 2) {
            *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
            {
                int owner = *(int *)(*(char **)((char *)moby + 0x24) + 0x10);
                moby[0x20] = *(unsigned char *)(d + 0x39C);
                *(int *)((char *)moby + 0x94) = owner;
            }
            if (moby[0x20] == 5) {
                D_L18_0015F4FC = 1.0f;
            }
            if (moby[0x20] == 0x19) {
                func_L00_00211908();
            }
            break;
        }
        if (D_L18_0016D2E0_s[0x30 / 4] == 2) {
            if (func_001F9850(0xA28) <= D_L18_0016D2E0_s[0x34 / 4] &&
                D_L18_0016D2E0_s[0x34 / 4] <= func_001F9850(0xB36)) {
                func_L18_002D81B0(*(void **)(d + 0x330));
            }
        } else if (D_L18_0016D2E0_s[0x30 / 4] == 3) {
            if (func_001F9850(0x3B6) < D_L18_0016D2E0_s[0x34 / 4] &&
                D_L18_0016D2E0_s[0x34 / 4] < func_001F9850(0x3C0)) {
                func_L18_002D8140(*(int *)(d + 0x200));
            }
            if (D_L18_00162430 == 0) {
                b = D_L18_0016016C + *(int *)(d + 0x294) * 0x80;
                func_L00_00286128(b + 0x30, b + 0x70);
                D_L18_00162430 = 1;
            }
        }
        break;
    case 1:
    {
        unsigned char *pp1 = D_0013E633_u + 0xE9D;
        if (func_00215570((float *)pp1, *(int *)(d + 0x274)) != 0 &&
            pp1[0x2024] == 2) {
            moby[0x20] = 7;
            *(unsigned short *)((char *)moby + 0x34) |= 0x41;
            func_L00_00299B68(0);
            *(int *)(d + 0x39C) = 2;
            *(int *)(d + 0x358) = func_001F9850(600);
            *(int *)(d + 0x344) = *(int *)(d + 0x25C);
            if (moby[0x53] != 10) {
                func_00213DE0((char *)moby, 10, 0, func_001F9850(0x14));
            }
            break;
        }
        t = D_L18_001B11B0[*(int *)(d + 0x270)];
        if (func_L00_0025A778((float *)pp1, t + 4, *t) != 0) {
            *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
            {
                int owner = *(int *)(*(char **)((char *)moby + 0x24) + 0x10);
                moby[0x20] = 10;
                *(int *)((char *)moby + 0x94) = owner;
            }
            b = (char *)D_L18_001B11B0[*(int *)(d + 0x260)];
            *(char **)(d + 0x340) = b;
            qcopy(d + 0x3C0, b + 0x10);
            *(float *)((char *)moby + 0x48) =
                func_L00_001FF860(((float *)pp1)[0] - *(float *)(d + 0x3C0),
                                  ((float *)pp1)[1] - *(float *)(d + 0x3C4));
            func_L18_002F8488((int)(char *)moby, *(int *)(d + 0x204));
            *(float *)(d + 0x38C) = *(float *)(d + 0x38C) * 0.85714287f;
            break;
        }
        if (func_001F9D10((float *)pp1,
                          D_L18_0016016C + (*(int *)(d + 0x250) << 7) + 0x30) <
            50.0f) {
            *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
            {
                int owner = *(int *)(*(char **)((char *)moby + 0x24) + 0x10);
                moby[0x20] = 0xC;
                *(int *)((char *)moby + 0x94) = owner;
            }
            t = D_L18_001B11B0[*(int *)(d + 0x260)];
            *(int **)(d + 0x340) = t;
            qcopy(d + 0x3C0, (char *)t + *t * 16);
            *(int *)(d + 0x344) = *(int *)(d + 0x250);
            *(float *)(d + 0x38C) = *(float *)(d + 0x38C) * 0.85714287f;
            break;
        }
        if (func_001F9D10((float *)pp1,
                          D_L18_0016016C + (*(int *)(d + 0x254) << 7) + 0x30) <
            50.0f) {
            *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
            *(int *)((char *)moby + 0x94) = *(int *)(*(char **)((char *)moby + 0x24) + 0x10);
            *(int *)(d + 0x34C) = 1;
            *(float *)(d + 0x38C) = *(float *)(d + 0x38C) * 0.71428573f;
            moby[0x20] = 0xC;
            t = D_L18_001B11B0[*(int *)(d + 0x264)];
            *(int **)(d + 0x340) = t;
            qcopy(d + 0x3C0, (char *)t + *t * 16);
            *(int *)(d + 0x344) = *(int *)(d + 0x254);
            break;
        }
        if (func_001F9D10((float *)pp1,
                          D_L18_0016016C + (*(int *)(d + 600) << 7) + 0x30) >=
            50.0f) {
            break;
        }
        if (D_L18_00162430 != 0) {
            func_L18_002D8140(*(int *)(d + 0x200));
            if (func_L00_0028EB98((char *)moby, *(int *)(d + 0x3B0)) != 0) {
                i = *(int *)(d + 0x3B0);
                if (i != -1 && *(char **)(D_0013E633_u + 0xA5 + i * 0x70) == (char *)moby &&
                    D_0013E633_u[0x91 + i * 0x70] != 0) {
                    func_L00_0028EBF0(i);
                }
                *(int *)(d + 0x3B0) = -1;
            }
            moby[0x20] = 0x1B;
            *(int *)((char *)moby + 0x94) = 0;
            *(unsigned short *)((char *)moby + 0x34) |= 0x41;
            *(int *)(d + 0x34C) = 7;
            break;
        }
        *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
        *(int *)((char *)moby + 0x94) = *(int *)(*(char **)((char *)moby + 0x24) + 0x10);
        *(int *)(d + 0x34C) = 2;
        *(float *)(d + 0x38C) = *(float *)(d + 0x38C) * 0.5714286f;
        moby[0x20] = 0xC;
        t = D_L18_001B11B0[*(int *)(d + 0x268)];
        *(int **)(d + 0x340) = t;
        qcopy(d + 0x3C0, (char *)t + *t * 16);
        *(int *)(d + 0x344) = *(int *)(d + 600);
        *(int *)(d + 0x358) = func_001F9850(0x168);
        break;
    }
    case 2:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if (*(float *)(d + 0x38C) <= 301.0f) {
            if (*(D_0013E633_u + 0x2EC1) != 0) {
                *(int *)(*(char **)(D_0013E633_u + 0x2E9D) + 0x94) = 0;
                *(unsigned short *)(*(char **)(D_0013E633_u + 0x2E9D) + 0x34) |= 0x41;
            }
            func_L15_002092E0();
            b = D_L18_0016016C + *(int *)(d + 0x2A0) * 0x80;
            func_L00_00217718(b + 0x30, b + 0x70, 0, 1);
            func_L00_002512D8(*(int *)(d + 0x2B0));
            func_L00_00286128((float *)(D_0013E633_u + 0xE9D), (float *)(D_0013E633_u + 0xEAD));
            func_L18_002F8488((int)(char *)moby, *(int *)(d + 0x204));
            func_L00_00299B68(1);
            moby[0x20] = 7;
            *(unsigned short *)((char *)moby + 0x34) |= 0x41;
            if (moby[0x53] != 10) {
                func_00213DE0((char *)moby, 10, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x39C) = 10;
            *(int *)(d + 0x348) = 0;
            *(int *)(d + 0x374) = 0;
            b = (char *)D_L18_001B11B0[*(int *)(d + 0x260)];
            *(char **)(d + 0x340) = b;
            qcopy(d + 0x3C0, b + 0x10);
            *(float *)((char *)moby + 0x48) =
                func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)(d + 0x3C0),
                                  ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)(d + 0x3C4));
            *(int *)(d + 0x368) = -1;
            break;
        }
        if (moby[0x53] == 10 &&
            (func_00215B18((char *)moby, 1.0f) != 0 || func_00215B18((char *)moby, 16.0f) != 0)) {
            qcopy((char *)v0, D_0013E633_u + 0xEDD);
            if (func_001F9B88(v0[2] - *(float *)(d + 0x3C8)) < 5.0f) {
                if ((float)moby[0x50] < 15.0f) {
                    func_L00_00250800((char *)moby, 2, v1);
                } else {
                    func_L00_00250800((char *)moby, 3, v1);
                }
                func_L00_001FF4B0(v2, (char *)moby + 0xC0, D_0015EE6C * 20.0f);
                {
                    float a = func_L00_00258C80(0.0872665f, 0.5236f);
                    float c = -func_002140F8(0.0872665f, 0.1745329f);
                    float e = func_001F9D48((char *)moby + 0x10, (float *)(D_0013E633_u + 0xE9D)) * 3.0f;
                    if (e > 120.0f) {
                        e = 120.0f;
                    } else if (e < 60.0f) {
                        e = 60.0f;
                    }
                    func_L18_002D6738(v1, v2, v0, func_001FA898_r(func_001F9878(e)),
                                      0, a, c);
                }
            }
        }
        if ((moby[0x70] & 2) != 0 && moby[0x53] != 10) {
            func_00213DE0((char *)moby, 10, 0, func_001F9850(0x14));
        }
        break;
    case 9: {
        float lvl;
        int hit;
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        qcopy((char *)v0, *(char **)(d + 0x340) + 0x10);
        hit = func_L18_002F7CD8((char *)moby, 20.0f, v0);
        lvl = func_L18_002E0F80(*(void **)(d + 0x334));
        if (moby[0x53] == 7) {
            if ((moby[0x70] & 2) != 0) {
                func_00213DE0((char *)moby, 8, 0, func_001F9850(10));
            }
        } else {
            func_001F9BF0(v1, *(char **)(d + 0x340) + 0x10, d + 0x3C0);
            func_L00_001FF4B0(v1, v1, D_0015EE6C * 5.0f);
            func_001F9BD8(d + 0x3C0, d + 0x3C0, v1);
        }
        if (lvl > 0.5f && lvl < 0.7f) {
            if (moby[0x53] != 9) {
                func_00213DE0((char *)moby, 9, 0, 0);
            }
            if (*(int *)(d + 0x33C) == 0) {
                break;
            }
            func_L00_00250800((char *)moby, 1, v1);
            b = D_L18_0016016C + *(int *)(d + 0x344) * 0x80;
            qcopy((char *)v2, b + 0x30);
            v2[2] = v2[2] + 10.0f;
            v2[2] = func_00214358(v2, 0, 0.5f);
            func_L18_002EB558(*(unsigned char **)(d + 0x33C), v1,
                              D_L18_0016016C + *(int *)(d + 0x344) * 0x80 + 0x30,
                              1, 1.0f);
            *(int *)(d + 0x33C) = 0;
        } else if (lvl >= 0.729f) {
            if (moby[0x53] != 0) {
                func_00213DE0((char *)moby, 0, 0, func_001F9850(0x14));
            }
            if (hit != 0) {
                moby[0x20] = 10;
            }
            t = (int *)(d + 0x240);
            for (i = 3; i >= 0; i--) {
                int k = *(int *)(d + 0x34C) == 1 ? t[-4] : *t;
                t++;
                func_L18_002F3038(D_L18_00160058 + k * 0x100);
            }
        }
        if (*(int *)(d + 0x33C) != 0) {
            float f = lvl + lvl;
            func_L00_00250800((char *)moby, 1, v1);
            if (f > 1.0f) {
                f = 1.0f;
            }
            func_L18_002EB558(*(unsigned char **)(d + 0x33C), v1, D_L18_0015F660,
                              0, f);
        }
        break;
    }
    case 10: {
        unsigned char *h;
        int *c;
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_00214D28((float *)(d + 0x374), D_0015EE6C * 12.0f,
                      D_0015EE70 * 20.0f);
        if (*(float *)(d + 0x374) != 0.0f) {
            func_001F9BF0(v0, *(char **)(d + 0x340) + *(int *)(d + 0x348) * 0x10 +
                                  0x10, d + 0x3C0);
            if (func_001F9CB8(v0) <= *(float *)(d + 0x374)) {
                *(int *)(d + 0x348) = *(int *)(d + 0x348) + 1;
            } else {
                func_L00_001FF4B0(v0, v0, *(float *)(d + 0x374));
            }
            func_001F9BD8(d + 0x3C0, d + 0x3C0, v0);
        }
        h = func_L18_002F8270((char *)moby);
        if (h != 0 &&
            (func_00215B18((char *)moby, 1.0f) != 0 || func_00215B18((char *)moby, 16.0f) != 0)) {
            qcopy((char *)v2, (char *)h + 0x10);
            qcopy((char *)v0, d + 0x3C0);
            func_001F9BD8(v0, v0, (char *)moby + 0xC0);
            v0[2] = v0[2] + 6.0f;
            func_L00_001FF4B0(v1, (char *)moby + 0xC0, D_0015EE6C * 10.0f);
            {
                float a = func_L00_00258C80(0.5236f, 0.7853982f);
                float c2 = -func_002140F8(0.3490659f, 0.5236f);
                func_L18_002D6738(v0, v1, v2, func_001F9850(0x3C), 5, a, c2);
            }
        }
        c = *(int **)(d + 0x340);
        if (*(int *)(d + 0x348) == *c - 2) {
            if (moby[0x53] != 0) {
                func_00213DE0((char *)moby, 0, 0, func_001F9850(0x14));
            }
            moby[0x20] = 0xB;
        }
        break;
    }
    case 0xB:
        if (*(int *)(d + 0x348) != **(int **)(d + 0x340)) {
            func_00214D28((float *)(d + 0x374), D_0015EE6C * 10.0f,
                          D_0015EE70 * 20.0f);
            func_001F9BF0(v0, *(char **)(d + 0x340) + *(int *)(d + 0x348) * 0x10 +
                                  0x10, d + 0x3C0);
            if (func_001F9CB8(v0) <= *(float *)(d + 0x374)) {
                *(int *)(d + 0x348) = *(int *)(d + 0x348) + 1;
            } else {
                func_L00_001FF4B0(v0, v0, *(float *)(d + 0x374));
            }
            func_001F9BD8(d + 0x3C0, d + 0x3C0, v0);
            break;
        }
        if (func_001F9D48((char *)moby + 0x10, (float *)(D_0013E633_u + 0xE9D)) >= 50.0f || *(short *)(D_0013E633_u + 0x112B) != 0) {
            break;
        }
        if (*(char **)(D_0013E633_u + 0x1119) != 0 && *(short *)(*(char **)(D_0013E633_u + 0x1119) + 0xA6) == 0x24B) {
            break;
        }
        t = *(int **)(d + 0x340);
        *(int *)(d + 0x374) = 0;
        qcopy(d + 0x3C0, (char *)t + *t * 16);
        *(int *)(d + 0x344) = *(int *)(d + *(int *)(d + 0x34C) * 4 + 0x250);
        if (*(int *)(d + 0x34C) == 2) {
            if (D_L18_001623F0 == 0) {
                moby[0x20] = 4;
            } else {
                moby[0x20] = 0x15;
                *(void **)(d + 0x330) = func_L18_002D7F48((float *)(d + 0x3C0),
                                                          (float *)(D_0013E633_u + 0xE9D),
                                                          *(int *)(d + 0x200));
            }
            break;
        }
        moby[0x20] = 0xC;
        break;
    case 4:
        if (*(short *)(D_0013E633_u + 0x112B) == 0 &&
            (*(char **)(D_0013E633_u + 0x1119) == 0 || *(short *)(*(char **)(D_0013E633_u + 0x1119) + 0xA6) != 0x24B)) {
            D_L18_001623F0 = 1;
            moby[0x20] = 7;
            *(int *)(d + 0x39C) = 5;
            t = (int *)(d + 0x204);
            *(char **)(d + 0x330) = func_L18_002D8070((float *)(D_0013E633_u + 0xE9D), *(int *)(d + 0x200));
            *(unsigned short *)((char *)moby + 0x34) |= 0x41;
            func_L00_00299B68(2);
            for (i = 2; i >= 0; i--) {
                func_L18_002F8408((int)(char *)moby, *t++);
            }
        }
        break;
    case 5:
        b = (char *)((*(int *)(d + 0x21C) << 8) + (int)D_L18_00160058);
        b += 0x10;
        D_L18_00162420 = 0;
        v0[0] = func_001F9F90(0.0f) * 6.0f;
        v0[1] = func_001F9FA8(0.0f) * 6.0f;
        v0[2] = 0.0f;
        v1[0] = func_001F9F90(0.7853982f) * 6.0f;
        v1[1] = func_001F9FA8(0.7853982f) * 6.0f;
        v1[2] = 0.0f;
        func_001F9BD8(v0, v0, b);
        func_001F9BD8(v1, v1, b);
        v0[2] = v0[2] + 5.0f;
        v1[2] = v1[2] + 7.0f;
        func_001F9BC0(v2);
        v2[2] = 3.1415927f;
        func_001F9BC0(v3);
        v2[1] = 0.17453292f;
        v3[1] = 0.5236f;
        v3[2] = 3.9269907f;
        b = (char *)((*(int *)(d + 0x298) << 7) + (int)D_L18_0016016C);
        qcopy((char *)v4, b + 0x30);
        v4[2] = *(float *)(b + 0x38);
        v4[3] = *(float *)(b + 0x78);
        func_L18_002E0E90(*(char **)(d + 0x334), v0, v1, v2, v3, v4,
                          D_L18_00162424, D_L18_00162428, 1);
        *(int *)(D_0013E633_u + 0x2EE1) = 3;
        moby[0x20] = 8;
        func_L18_002D9358((unsigned char *)D_L18_00160058 +
                              *(int *)(d + 0x21C) * 0x100,
                          func_001F9850(0x708));
        break;
    case 3:
        if (func_L18_002E0F90(*(void **)(d + 0x334)) == 2) {
            qcopy(d + 0x3C0, d + 0x300);
            qcopy((char *)moby + 0x40, d + 0x310);
            moby[0x20] = 9;
            if (moby[0x53] != 7) {
                func_00213DE0((char *)moby, 7, 0, func_001F9850(0x14));
            }
            func_L18_002F8488((int)(char *)moby, *(int *)(d + 0x398));
            func_L18_002F8408((int)(char *)moby,
                              *(int *)(d + (*(int *)(d + 0x34C) - 1) * 4 + 0x204));
            func_L00_00250800((char *)moby, 1, v0);
            *(char **)(d + 0x33C) = func_L18_002EB4E0((char *)moby, v0, 300);
        }
        break;
    case 8:
        *(unsigned short *)((char *)moby + 0x34) |= 0x41;
        if (func_L18_002E0F80(*(void **)(d + 0x334)) < 1.0f) {
            break;
        }
        moby[0x20] = 0xC;
        *(unsigned short *)((char *)moby + 0x34) &= 0xFFBE;
        if (moby[0x53] != 0) {
            func_00213DE0((char *)moby, 0, 0, func_001F9850(10));
        }
        break;
    case 0x14:
        if (func_001F9908((int *)(d + 0x354)) == 0) {
            break;
        }
        *(int *)(d + 0x354) = func_001F9850(0x14);
        qcopy((char *)v0, d + 0x3C0);
        v0[2] = v0[2] + 4.0f;
        if (func_L18_002F1420((char *)moby, *(int *)(d + 0x214)) == 0) {
            break;
        }
        moby[0x20] = 0xC;
        break;
    case 0xC:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(tgt + 0x10) - *(float *)((char *)moby + 0x10),
                                            *(float *)(tgt + 0x14) - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if ((moby[0x70] & 2) == 0) {
            break;
        }
        switch (func_L18_002F8518((char *)moby)) {
        case 0xF:
            moby[0x20] = 0xF;
            if (moby[0x53] != 10) {
                func_00213DE0((char *)moby, 10, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x354) = 0;
            *(int *)(d + 0x358) = func_001F9850(600);
            *(int *)(d + 0x35C) = func_001F9850(0x168);
            break;
        case 0xD:
            moby[0x20] = 0xD;
            if (moby[0x53] != 2) {
                func_00213DE0((char *)moby, 2, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x354) = 0;
            *(int *)(d + 0x358) = func_001F9850(func_002140B0(3) * 0x5A + 0x14A);
            *(int *)(d + 0x35C) =
                func_001F9850((6 - *(int *)(d + 0x34C)) * 0x1E);
            *(float *)(d + 0x3BC) = func_002140F8(-0.33333334f, 0.33333334f);
            break;
        case 0x11:
            moby[0x20] = 0x11;
            if (moby[0x53] != 11) {
                func_00213DE0((char *)moby, 0xB, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x354) = func_001F9850(10);
            *(int *)(d + 0x358) = func_001F9850(600);
            break;
        default:
            moby[0x20] = 0x10;
            if (moby[0x53] != 6) {
                func_00213DE0((char *)moby, 6, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x354) = func_001F9850(0xF);
            *(int *)(d + 0x358) = func_001F9850(0x168);
            break;
        case 0xE:
            moby[0x20] = 0x12;
            if (moby[0x53] != 0) {
                func_00213DE0((char *)moby, 0, 0, func_001F9850(0x14));
            }
            {
                float best = 0.0f;
                char *g = (char *)D_0013E633_u + 0xE1D;
                t = (int *)(d + 0x280);
                for (i = 2; i >= 0; i--) {
                    float w;
                    char *bb = (char *)((*t << 7) + (int)D_L18_0016016C);
                    qcopy((char *)v0, bb + 0x30);
                    w = func_001FA850(
                        func_L00_001FF860(*(float *)(d + 0x3C0) - *(float *)(g + 0x80),
                                          *(float *)(d + 0x3C4) - *(float *)(g + 0x84)),
                        func_L00_001FF860(*(float *)(d + 0x3C0) - v0[0],
                                          *(float *)(d + 0x3C4) - v0[1]));
                    if (best < w) {
                        *(int *)(d + 0x3A8) = *t;
                        best = w;
                    }
                    t++;
                }
            }
            *(int *)(d + 0x358) = func_001F9850(900);
            *(int *)(d + 0x354) = func_001F9850(0xB4);
            break;
        case 0x13:
            moby[0x20] = 0x13;
            if (moby[0x53] != 1) {
                func_00213DE0((char *)moby, 1, 0, func_001F9850(0x14));
            }
            if (func_001F9D48((char *)moby + 0x10, (float *)(D_0013E633_u + 0xE9D)) < 16.0f) {
                *(int *)(d + 0x354) = func_001F9850(0x5A);
            } else {
                *(int *)(d + 0x354) = 0;
            }
            *(int *)(d + 0x358) = func_001F9850(600);
            *(int *)(d + 0x35C) = func_001F9850(600);
            *(char **)(d + 0x338) =
                func_L18_002DD7D0((char *)moby, d + 0x3C0, 5.8f);
            break;
        }
        break;
    case 0xE:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(d + 0x320) - *(float *)((char *)moby + 0x10),
                                            *(float *)(d + 0x324) - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L00_00250800((char *)moby, 0x13, v0);
        *(Q4050 *)v1 = *(Q4050 *)(d + 0x320);
        f8680_v((char *)moby, d + 0x140, v0, v1);
        *(float *)(d + 0x3D8) = -3.0f;
        if ((moby[0x70] & 2) != 0 && moby[0x53] != 8) {
            func_00213DE0((char *)moby, 8, 0, func_001F9850(0x14));
        }
        if (*(int *)(d + 0x354) != 0) {
            if (*(int *)(d + 0x33C) == 0) {
                func_L00_00250800((char *)moby, 1, v1);
                *(char **)(d + 0x33C) =
                    func_L18_002EB4E0((char *)moby, v1, func_001F9850(D_L18_0016242C));
            } else if (func_001F9908((int *)(d + 0x354)) != 0) {
                if (moby[0x53] != 9) {
                    func_00213DE0((char *)moby, 9, 0, 0);
                }
                *(int *)(d + 0x354) = func_001F9850(0x78);
                func_L00_00250800((char *)moby, 1, v1);
                func_001F9BF0(v2, d + 0x320, v1);
                v2[2] = 2.0f;
                func_L00_001FF500(v2, v2, 180.0f);
                func_001F9BD8(v2, v2, v1);
                func_L18_002EB558(*(unsigned char **)(d + 0x33C), v1, v2, 1, 1.0f);
                *(int *)(d + 0x33C) = 0;
                if (*(int *)(d + 0x358) == 0) {
                    *(int *)(d + 0x350) = 0xE;
                    moby[0x20] = 0xC;
                }
            } else {
                float f = 1.0f - func_001FA888(*(int *)(d + 0x354)) /
                                     func_001F9878(90.0f);
                if (f > 1.0f) {
                    f = 1.0f;
                } else if (f < 0.0f) {
                    f = 0.0f;
                }
                func_L00_00250800((char *)moby, 1, v1);
                func_L18_002EB558(*(unsigned char **)(d + 0x33C), v1, d + 0x320,
                                  0, f);
                if (func_001F9850(0xF) < *(int *)(d + 0x354)) {
                    qcopy(d + 0x320, tgt + 0x10);
                }
            }
        }
        func_001F9908((int *)(d + 0x358));
        break;
    case 0xF:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(tgt + 0x10) - *(float *)((char *)moby + 0x10),
                                            *(float *)(tgt + 0x14) - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if (func_00215B18((char *)moby, 1.0f) != 0 || func_00215B18((char *)moby, 16.0f) != 0) {
            if (func_002140B0(5) != 0 || *(int *)(d + 0xB4) == 2) {
                float a;
                float r;
                {
                    char *src = *(int *)(d + 0xB4) == 2
                                    ? (char *)(D_0013E633_u + 0xE9D)
                                    : d + 0x70;
                    qcopy((char *)v1, src);
                }
                a = func_00214158();
                r = func_002140F8(1.0f, 10.0f);
                v0[0] = func_001F9F90(a) * r;
                v0[1] = func_001F9FA8(a) * r;
                v0[2] = 0.0f;
                func_001F9BD8(v0, v0, v1);
            } else {
                qcopy((char *)v0, d + 0x70);
            }
            if (*(int *)(D_0013E633_u + 0x2EA9) == 0xF) {
                qcopy((char *)v1,
                      D_L18_0016016C + *(int *)(d + 0x344) * 0x80 + 0x30);
                func_001F9BF0(v0, v0, v1);
                if (func_001F9CE8(v0) > 29.0f) {
                    func_L00_001FF500(v0, v0, 29.0f);
                }
                func_001F9BD8(v0, v0, v1);
            }
            v0[2] = func_00214358(v0, 0, 0.5f);
            if (func_001F9B88(v0[2] - *(float *)(d + 0x3C8)) < 5.0f) {
                float a;
                float c2;
                float e;
                if (func_00215B18((char *)moby, 1.0f) == 0) {
                    func_L00_00250800((char *)moby, 3, v1);
                } else {
                    func_L00_00250800((char *)moby, 2, v1);
                }
                func_L00_001FF4B0(v2, (char *)moby + 0xC0, D_0015EE6C * 20.0f);
                a = func_L00_00258C80(0.0872665f, 0.5236f);
                c2 = func_002140F8(0.0872665f, 0.1745329f);
                e = func_001F9D48((char *)moby + 0x10, (float *)(D_0013E633_u + 0xE9D)) * 3.0f;
                if (e > 120.0f) {
                    e = 120.0f;
                } else if (e < 60.0f) {
                    e = 60.0f;
                }
                func_L18_002D6738(v1, v2, v0, func_001FA898_r(func_001F9878(e)),
                                  2, a, -c2);
                func_L18_002F8B00((char *)moby, v1);
            }
        }
        if (func_001F9908((int *)(d + 0x358)) == 0 &&
            (*(float *)(d + 0x390) <= 25.0f ||
             (float)*(int *)(d + 0x34C) >= 4.0f)) {
            break;
        }
        *(int *)(d + 0x350) = 0xF;
        moby[0x20] = 0xC;
        break;
    case 0xD:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(tgt + 0x10) - *(float *)((char *)moby + 0x10),
                                            *(float *)(tgt + 0x14) - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if (*(int *)(d + 0x354) < *(int *)(d + 0x358) + *(int *)(d + 0x35C)) {
            *(float *)(d + 0x128) =
                func_L00_001FF860(1.0f, *(float *)(d + 0x3BC));
        }
        if (moby[0x53] == 2) {
            if ((moby[0x70] & 2) != 0) {
                func_00213DE0((char *)moby, 4, 0, func_001F9850(0x14));
            }
        } else if (func_001F9908((int *)(d + 0x354)) != 0 &&
                   *(int *)(d + 0x358) != 0 && moby[0x53] != 4) {
            func_00213DE0((char *)moby, 4, 0, func_001F9850(0x14));
        }
        if (moby[0x53] == 4) {
            if (func_00215B18((char *)moby, 1.0f) != 0) {
                float f;
                *(int *)(d + 0x354) = func_001F9850(0x5A);
                *(int *)(d + 0x360) = func_001F9850(300);
                func_L00_00250800((char *)moby, 0, v0);
                qcopy((char *)v1, (char *)moby + 0xC0);
                func_L00_001FF4B0(v2, (char *)moby + 0xD0, *(float *)(d + 0x3BC));
                func_001F9BD8(v1, v1, v2);
                f = func_001F9D48(v0, tgt + 0x10) * 0.5f;
                if (f > 30.0f) {
                    f = 30.0f;
                } else if (f < 0.0f) {
                    f = 0.0f;
                }
                func_L00_001FF4B0(v1, v1, f * D_0015EE6C);
                func_L18_002DCD28((char *)v0, v1, (int)(char *)moby, 64.0f,
                                  D_0015EE6C * 18.0f);
                func_L18_002F8B00((char *)moby, v0);
                *(float *)(d + 0x3BC) = func_L00_00258C80(0.128f, 0.33333334f);
            } else if ((moby[0x70] & 2) != 0 && moby[0x53] != 3) {
                func_00213DE0((char *)moby, 3, 0, func_001F9850(0x14));
            }
        }
        if (func_001F9908((int *)(d + 0x358)) != 0 &&
            func_001F9908((int *)(d + 0x35C)) != 0) {
            *(int *)(d + 0x350) = 0xD;
            moby[0x20] = 0xC;
            if (moby[0x53] != 5) {
                func_00213DE0((char *)moby, 5, 0, func_001F9850(0x14));
            }
        }
        break;
    case 0x10:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if (func_001F9908((int *)(d + 0x354)) != 0) {
            float a;
            float r;
            qcopy((char *)v0, D_L18_0016016C + *(int *)(d + 0x344) * 0x80 + 0x30);
            func_L00_001FF4B0(v1, (char *)moby + 0xC0, 3.0f);
            func_001F9BD8(v1, v1, (char *)moby + 0x10);
            v1[2] = v1[2] + 3.0f;
            a = func_00214158();
            r = func_002140F8(3.0f, 29.0f);
            v2[0] = func_001F9F90(a) * r;
            v2[1] = func_001F9FA8(a) * r;
            v2[2] = 0.0f;
            func_001F9BD8(v2, v2, v0);
            v2[2] = v2[2] + 5.0f;
            v2[2] = func_00214358(v2, 0, 0.5f);
            if (func_001F9B88(v2[2] - *(float *)(d + 0x3C8)) < 5.0f) {
                float speed = func_001F9D48(v2, v1) / (float)func_001F9850(0x3C);
                float w = func_002140F8(18.0f, 20.0f);
                func_L18_002D70E8(speed, (int)(char *)moby, *(int *)(d + 0x29C), v1, v2,
                                  func_001FA898_r(func_001F9878(w * 60.0f)));
            }
            *(int *)(d + 0x354) = func_001F9850(0xF);
        }
        if (func_001F9908((int *)(d + 0x358)) == 0) {
            break;
        }
        *(int *)(d + 0x350) = 0x10;
        moby[0x20] = 0xC;
        break;
    case 0x11: {
        int n;
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        if ((moby[0x70] & 2) != 0 && moby[0x53] != 12) {
            func_00213DE0((char *)moby, 0xC, 0, func_001F9850(0x14));
        }
        if ((moby[0x53] == 12 && func_001F9908((int *)(d + 0x354)) != 0) ||
            *(int *)(d + 0x354) == func_001F9850(0x1E)) {
            func_L00_001FF4B0(v0, (char *)moby + 0xD0,
                              *(int *)(d + 0x354) != 0 ? 4.3f : -4.3f);
            func_001F9BD8(v0, v0, d + 0x3C0);
            v0[2] = func_00214358(v0, 0, 0.5f);
            if (func_001F9B88(v0[2] - *(float *)(d + 0x3C8)) < 1.0f) {
                float speed = func_001F9D48(v0, v1) / (float)func_001F9850(0x3C);
                if (func_L18_002FDB28(*(int *)(d + 0x210), v0, v0, speed) != 0 &&
                    *(int *)(d + 0x354) == 0) {
                    *(int *)(d + 0x354) = func_001F9850(0x3C);
                }
            }
        }
        n = func_L18_002FDCA0(*(int *)(d + 0x210));
        if (n >= 11 && func_001F9850(0x3C) > *(int *)(d + 0x354)) {
            *(int *)(d + 0x354) = func_001F9850(1000);
        } else if (n < 3 && func_001F9850(0x3C) < *(int *)(d + 0x354)) {
            *(int *)(d + 0x358) = 0;
        }
        if (func_001F9908((int *)(d + 0x358)) == 0) {
            break;
        }
        *(int *)(d + 0x350) = 0x11;
        moby[0x20] = 0xC;
        if (moby[0x53] != 13) {
            func_00213DE0((char *)moby, 0xD, 0, func_001F9850(0x14));
        }
        break;
    }
    case 0x13: {
        int fire;
        char *g13 = (char *)D_0013E633_u + 0xE1D;
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        if (func_001F9908((int *)(d + 0x354)) != 0) {
            func_L18_002F7DC0((char *)moby);
        }
        func_L18_002DD848(*(char **)(d + 0x338), d + 0x3C0, 5.8f);
        if (func_L00_0028EB98((char *)moby, *(int *)(d + 0x3B0)) == 0) {
            *(int *)(d + 0x3B0) = func_0022ED80(0xE, 4, (int)(char *)moby);
        }
        fire = 0;
        if (func_001F9908((int *)(d + 0x358)) != 0) {
            fire = 1;
            if (*(int *)(g13 + 0x208C) == 0xF &&
                func_001F9908((int *)(d + 0x35C)) == 0) {
                fire = 0;
            }
        }
        if (*(int *)(g13 + 0x208C) == 0xF) {
            char *list = *(char **)(g13 + 0x560);
            if (func_001F9D48(g13 + 0x80, list + *(int *)list * 16) < 3.0f) {
                fire = 1;
            }
        }
        if (*(int *)(g13 + 0x2084) == 0x16 &&
            func_001F9D48((char *)moby + 0x10, g13 + 0x80) < 12.0f) {
            fire = 1;
        }
        if (func_001F9D48((float *)(D_0013E633_u + 0xE9D),
                          D_L18_0016016C + *(int *)(d + 600) * 0x80 + 0x30) >
            D_L18_00162408_f + 5.8f) {
            fire = 1;
        }
        if (fire) {
            i = *(int *)(d + 0x3B0);
            if (i != -1 && *(char **)(D_0013E633_u + 0xA5 + i * 0x70) == (char *)moby &&
                D_0013E633_u[0x91 + i * 0x70] != 0) {
                func_L00_0028EBF0(i);
            }
            *(int *)(d + 0x3B0) = -1;
            moby[0x20] = 0xC;
            if (moby[0x53] != 0) {
                func_00213DE0((char *)moby, 0, 0, func_001F9850(0x14));
            }
            *(int *)(d + 0x350) = 0x13;
        }
        break;
    }
    case 0x12:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(d + 0x70) - *(float *)((char *)moby + 0x10),
                                            *(float *)(d + 0x74) - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        if (func_L18_002F7CD8((char *)moby, D_0015EE6C * 12.0f,
                              D_L18_0016016C + *(int *)(d + 0x3A8) * 0x80 +
                                  0x30) != 0) {
            moby[0x20] = 0xE;
            if (moby[0x53] != 7) {
                func_00213DE0((char *)moby, 7, 0, func_001F9850(0x14));
            }
            qcopy(d + 0x320, tgt + 0x10);
        }
        break;
    case 0x15:
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(*(float *)(*(char **)(d + 0x330) + 0x10) -
                                                *(float *)((char *)moby + 0x10),
                                            *(float *)(*(char **)(d + 0x330) + 0x14) -
                                                *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002D81B0(*(void **)(d + 0x330));
        if (func_L18_002F7CD8((char *)moby, D_0015EE6C * 12.0f,
                              *(char **)(d + 0x330) + 0x10) != 0) {
            int ticks;
            switch (*(int *)(d + 0x34C)) {
            case 0:
                ticks = 0xE10;
                break;
            case 1:
                ticks = 0xA8C;
                break;
            case 2:
                ticks = 0x708;
                break;
            case 3:
                ticks = 0x4B0;
                break;
            case 4:
                ticks = 900;
                break;
            default:
                ticks = 600;
                break;
            }
            func_L18_002D9358(D_L18_00160058 + *(int *)(d + 0x21C) * 0x100,
                              func_001F9850(func_001F9850(ticks)));
            *(int *)(d + 0x354) = 0;
            moby[0x20] = *(int *)(d + 0x34C) == 4 ? 0x14 : 0xC;
            *(int *)(d + 0x364) = 1;
            *(int *)(d + 0x34C) = *(int *)(d + 0x34C) + 1;
        }
        if (D_L18_00162420 != 0) {
            moby[0x20] = 5;
        }
        break;
    case 0x1A: {
        int n;
        aim = (float *)((char *)moby + 0x48);
        ang = func_L00_001FF860(((float *)(D_0013E633_u + 0xE9D))[0] - *(float *)((char *)moby + 0x10),
                                            ((float *)(D_0013E633_u + 0xE9D))[1] - *(float *)((char *)moby + 0x14));
        func_L00_0025CE58(aim, (float *)(d + 0x370), ang,
                          D_0015EE70 * 6.2831855f, D_0015EE70 * 6.2831855f,
                          D_0015EE6C * 6.2831855f);
        func_L18_002F7F00((char *)moby);
        *(int *)(d + 0x358) = *(int *)(d + 0x358) + 1;
        n = func_001F9850(0x14);
        if (*(int *)(d + 0x358) % n == 0) {
            float a = func_001FA748(*(float *)((char *)moby + 0x48),
                                    func_002140F8(-30.0f, 30.0f) * 0.017453292f);
            v0[0] = func_001F9F90(a) * 6.0f;
            v0[1] = func_001F9FA8(a) * 6.0f;
            v0[2] = 0.0f;
            v0[2] = func_002140F8(3.0f, 8.0f);
            func_001F9BD8(v0, v0, d + 0x3C0);
            func_L00_0025F4A8_alt((char *)moby, D_L18_0015F660, v0, 0.0f, 0.0f, 0x14, 6,
                                  0x20, 1.0f, 2.5f, 9.0f, 1.0f, -1, 0.0f, 0, 0,
                                  -1, 0);
        }
        if (*(int *)(d + 0x358) == func_001F9850(0x78)) {
            if (moby[0x53] != 14) {
                func_00213DE0((char *)moby, 0xE, 5, 1);
            }
            v0[0] = func_001F9F90(*(float *)((char *)moby + 0x48)) * 3.0f;
            v0[1] = func_001F9FA8(*(float *)((char *)moby + 0x48)) * 3.0f;
            v0[2] = 6.0f;
            func_001F9BD8(v0, v0, d + 0x3C0);
            func_L00_0025F4A8_alt((char *)moby, D_L18_0015F660, v0, 0.0f, 0.0f, 0x28, 0xA,
                                  0x20, 2.5f, 5.0f, 9.0f, 2.0f, -1, 10.0f, 0, 0,
                                  -1, 0);
        }
        if (moby[0x52] == moby[0x53] && moby[0x52] == 14 &&
            func_00215B18((char *)moby, 30.0f) != 0) {
            moby[0x20] = 0x1B;
            func_L18_002D93C0(D_L18_00160058 + *(int *)(d + 0x21C) * 0x100);
        }
        break;
    }
    case 0x1B: {
        int k;
        *(int *)((char *)moby + 0x94) = 0;
        *(unsigned short *)((char *)moby + 0x34) =
            (*(unsigned short *)((char *)moby + 0x34) & 0xEFFF) | 0x41;
        k = *(int *)(d + 0x34C);
        if (k < 7) {
            func_L00_00299B68(3);
            *(int *)(d + 0x34C) = 7;
            ((char *)D_L18_0016D2E0_s)[0x4A] = 1;
            b = D_L18_0016016C + *(int *)(d + 0x294) * 0x80;
            qcopy((char *)D_L18_0016D2F0, b + 0x30);
            qcopy((char *)D_L18_0016D300, b + 0x70);
            moby[0x20] = 7;
            *(int *)(d + 0x39C) = 0x1B;
            break;
        }
        if (k == 7) {
            int n = *(int *)(d + 0x3AC);
            char *g = (char *)D_0013E633_u + 0xE1D;
            if (*(char **)(g + 0x2FC) != 0 && *(short *)(g + 0x30E) == 0 &&
                *(short *)(*(char **)(g + 0x2FC) + 0xA6) == 0x247 &&
                *(int *)(D_0013E633_u + 0x2EA1) == 0x22) {
                if (n == 0) {
                    *(int *)(d + 0x3AC) = 1;
                    n = *(int *)(d + 0x3AC);
                    if (n == 0) {
                        break;
                    }
                }
            } else if (n == 0) {
                func_L18_002D8140(*(int *)(d + 0x200));
                n = *(int *)(d + 0x3AC);
                if (n == 0) {
                    break;
                }
            }
            *(int *)(d + 0x3AC) = n + 1;
            if (func_001F9850(0x28) < n) {
                func_L00_00299B68(4);
                *(int *)(d + 0x34C) = 8;
                moby[0x20] = 7;
                *(int *)(d + 0x39C) = 0x1B;
            }
            break;
        }
        if (k == 8) {
            {
                Q4050 zero = {{0.0f, 0.0f, 0.0f, 0.0f}};
                *(Q4050 *)v0 = zero;
                *(Q4050 *)v1 = zero;
            }
            v0[0] = 660.6f;
            v0[1] = 481.4f;
            v0[2] = 112.6f;
            v1[1] = -0.12f;
            v1[2] = -2.76f;
            func_L00_002EBF50(v0, v1, 1, 0, 0);
            func_L00_00299B68(5);
            *(int *)(d + 0x34C) = 9;
            break;
        }
        if (k == 9) {
            if (D_L18_0015F6A8 == 0) {
                func_L00_0029ADD8();
                *(int *)(d + 0x34C) = 10;
                break;
            }
            k = *(int *)(d + 0x34C);
        }
        if (k == 10 && D_L18_0015F6A8 == 0) {
            func_L00_0029A8D0(0xB);
            *(int *)(d + 0x34C) = 0xB;
        } else if (k == 0xB && D_L18_0015F6A8 == 0) {
            D_L18_001623F0 = 0;
            D_L18_00162430 = 0;
            func_00219C70(0x21);
            func_0020D678(*(void **)(d + 0x3A0));
            if (*(int *)(d + 0x3A4) != 0) {
                func_0020D678(*(void **)(d + 0x3A4));
            }
            func_0020D678((char *)moby);
            return;
        }
        break;
    }
    }
    func_L18_002F86E8((char *)moby);
    if (moby[0x31] != 0) {
        func_L00_00250800((char *)moby, 8, v0);
        func_L00_00250800((char *)moby, 9, v1);
        func_001F9BF0(v2, v0, v1);
        func_L00_001FF4B0(v2, v2, 0.1f);
        func_L00_00264BE8(v0, v0, v2, 600000.0f, 0.0f);
    }
    if (*(int *)(d + 0x34C) > 1) {
        *(char **)(D_0013E633_u + 0x13F5) = (char *)moby;
    }
    qcopy((char *)moby + 0x10, d + 0x3C0);
    slow = 0.3f;
    fast = 0.03f;
    *(float *)(d + 0x3D4) =
        func_001FA748(*(float *)(d + 0x3D4), D_0015EE6C * 3.1415927f);
    func_00214D88(*(float *)(d + 0x3D0) * func_001F9FA8(*(float *)(d + 0x3D4)) +
                      *(float *)(d + 0x3D8),
                  D_0015EE70 * 4.0f, D_0015EE70 * 4.0f, D_0015EE6C * 4.0f,
                  (float *)(d + 0x3DC), (float *)(d + 0x3E0));
    *(float *)((char *)moby + 0x18) = *(float *)((char *)moby + 0x18) + *(float *)(d + 0x3DC);
    *(int *)(d + 0x3D8) = 0;
    func_L00_00263950((char *)moby, d + 0xC0, 7, D_0015EE64 * fast, D_0015EE64 * slow);
    func_L00_00263950((char *)moby, d + 0x140, 0x13, D_0015EE64 * fast, D_0015EE64 * slow);
    *(float *)(d + 0x3B4) =
        func_001FA748(*(float *)(d + 0x3B4), D_0015EE6C * 3.4906585f);
    {
        int c = func_001FA8A8(0x30C8C8C8, 0x1C393939,
                              func_001F9FA8(*(float *)(d + 0x3B4)) * 0.5f + 0.5f);
        *(int *)((char *)moby + 0x90) = c;
        *(int *)(d + 0x3B8) = (int)((unsigned int)c >> 24 << 24) |
                              ((c >> 0x10) & 0xFF) / 3 << 0x10 | (c & 0xFF00) |
                              (c & 0xFF) / 3;
    }
    if (moby[0x31] != 0) {
        {
            extern void f8ce0_cb(void) __asm__("func_L18_002F8CE0");
            func_001F49B0(f8ce0_cb, (char *)moby);
        }
    }
}
