/* NON_MATCHING func_L11_0030EC70 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 1292 / retail 1300, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Update for moby class 1157 (level 11): a 6-way state machine on m[0x20]; state 0 spawns a 4x6 table of child m
 *   Differences left: the operand order of the three FA790/FA748 pairs in case 3 (retail loads the 0x68 offset, th
 *   Unblock: a way to force the three-pair load order without reordering the arguments, and the shared tail block 
 */
extern char *func_0020D348(int);
extern void func_0022ED80(int, int, int);
extern int func_001F9850(int);
extern float func_00214D28(float *, float, float);
extern float func_001F9B88(float);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_L00_002EBF50(void *, void *, int, int, int);
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);
extern float func_00214D88_f(float *, float *, float, float, float, float) __asm__("func_00214D88");
extern void func_001F9C08(void *, void *, void *, float);
extern void func_001F9BC0(void *);
extern void func_L00_002EBE88(void *);
extern void func_L00_002EBEE0(void *);
extern void func_L00_002EC0C8(int);
extern void func_L11_0030F188(void *);
extern int D_L11_00160058_m __asm__("D_L11_00160058") MACRO_ADDR;
extern char *D_L11_0016016C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern short D_L11_00161FE4;
extern short D_L11_00161FE8;
extern short D_L11_00161FEC;
extern short D_L11_00161FF0;
extern char D_0013E633[];
typedef struct { char *slot[4][6]; int w60; int w64; int w68; float f6C; float f70; float f74; float f78; } EcData;

// Update for moby class 1157: spawns six child mobies per row, then eases a pair of vectors between two linked objects.
void func_L11_0030EC70(char *m) {
    EcData *d = *(EcData **)(m + 0x78);
    char *o;
    char *p;
    char *ob;
    char *q;
    float f20;
    float f21;
    float ta;
    float tb;
    float f0;
    float va[4];
    float vb[4];
    int i;
    int k;
    int r;
    float f13;
    float f14;
    float f15;
    float f12;
    char *g;
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        g = D_0013E633 + 0xE1D;
        for (i = 0; i < 4; i++) {
            char **pp = &d->slot[i][0];
            for (k = 5; k >= 0; k--) {
                ob = func_0020D348(0x4B7);
                *pp = ob;
                *(short *)(ob + 0x32) = 0x40;
                (*pp)[0x31] = 1;
                *(long *)(*pp + 0x38) = *(long *)(*(char **)(g + 0x2080) + 0x38);
                *(unsigned short *)((*pp) + 0x34) = *(unsigned short *)(m + 0x34);
                qcopy(*pp + 0x10, m + 0x10);
                qcopy(*pp + 0x40, m + 0x40);
                pp++;
            }
        }
        *(int *)&d->f6C = 0;
        m[0x20] = 1;
        break;
    case 1:
        o = (char *)(D_L11_00160058_m + (d->w60 << 8));
        if (*(unsigned char *)(o + 0xBC) == 1) {
            m[0x20] = 2;
            d->f78 = -1.0f;
            *(int *)&D_L11_00161FE4 = 0;
            func_0022ED80(0, 0, (int)m);
        } else if (*(unsigned char *)(o + 0xBC) == 2) {
            d->f6C = 1.0f;
            m[0x20] = 5;
        }
        break;
    case 2:
        f21 = d->f78;
        r = func_001F9850(0x14);
        f20 = 1.0f;
        func_00214D28((float *)&d->f78, 1.0f, 1.0f / (float)r);
        f0 = func_001F9B88(d->f78);
        f20 = f20 - f0;
        *(float *)&D_L11_0015F4FC = f20;
        if (f21 < 0.0f) {
            if (0.0f <= d->f78) {
                o = (char *)(D_L11_00160058_m + (d->w60 << 8));
                func_L00_00217718(o + 0x10, o + 0x40, 0x72, 1);
                p = D_L11_0016016C + (d->w64 << 7);
                func_L00_002EBF50(p + 0x30, p + 0x70, 1, 0, 0);
                break;
            }
        }
        if (1.0f <= d->f78) {
            m[0x20] = 3;
        }
        break;
    case 3:
        f0 = D_0015EE70;
        f12 = d->f6C;
        f13 = *(float *)&D_L11_00161FE8 * f0;
        f14 = *(float *)&D_L11_00161FEC * f0;
        f15 = *(float *)&D_L11_00161FF0 * D_0015EE6C;
        f20 = 1.0f;
        d->f74 = f12;
        func_00214D88_f((float *)&d->f6C, (float *)&d->f70, 1.0f, f13, f14, f15);
        func_001F9C08(va, D_L11_0016016C + (d->w64 << 7) + 0x30,
                      D_L11_0016016C + (d->w68 << 7) + 0x30, d->f6C);
        func_001F9BC0(vb);
        ta = *(float *)(D_L11_0016016C + (d->w64 << 7) + 0x70);
        tb = *(float *)(D_L11_0016016C + (d->w68 << 7) + 0x70);
        vb[0] = func_001FA790(tb, ta) * d->f6C;
        vb[0] = func_001FA748(*(float *)(D_L11_0016016C + (d->w64 << 7) + 0x70), vb[0]);
        ta = *(float *)(D_L11_0016016C + (d->w64 << 7) + 0x74);
        tb = *(float *)(D_L11_0016016C + (d->w68 << 7) + 0x74);
        vb[1] = func_001FA790(tb, ta) * d->f6C;
        vb[1] = func_001FA748(*(float *)(D_L11_0016016C + (d->w64 << 7) + 0x74), vb[1]);
        ta = *(float *)(D_L11_0016016C + (d->w64 << 7) + 0x78);
        tb = *(float *)(D_L11_0016016C + (d->w68 << 7) + 0x78);
        vb[2] = func_001FA790(tb, ta) * d->f6C;
        vb[2] = func_001FA748(*(float *)(D_L11_0016016C + (d->w64 << 7) + 0x78), vb[2]);
        func_L00_002EBEE0(vb);
        if (f20 <= d->f6C) {
            d->f78 = -1.0f;
            m[0x20] = 4;
        }
        break;
    case 4:
        f21 = d->f78;
        r = func_001F9850(0x14);
        f20 = 1.0f;
        func_00214D28((float *)&d->f78, 1.0f, 1.0f / (float)r);
        f0 = func_001F9B88(d->f78);
        f20 = f20 - f0;
        *(float *)&D_L11_0015F4FC = f20;
        if (f21 < 0.0f) {
            if (0.0f <= d->f78) {
                func_L00_00217718(D_0013E633 + 0xE9D, D_0013E633 + 0xE9D + 0x10, 0, 1);
                func_L00_002EC0C8(3);
                break;
            }
        }
        if (1.0f <= d->f78) {
            m[0x20] = 5;
        }
        break;
    case 5:
    default:
        break;
    }
    func_L11_0030F188(m);
}
