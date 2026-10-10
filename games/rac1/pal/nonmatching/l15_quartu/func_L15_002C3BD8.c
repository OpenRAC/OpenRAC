/* NON_MATCHING func_L15_002C3BD8 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: SIZE ours 1672 / retail 1684, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped at budget (10 runs), 1672 vs 1684 bytes, not EXACT. Best: p8.c (p9 differs only by an int index). Matc
 *   Still differs: retail copies the list index into $v0 before the sll (`daddu $v0,$v1`) and copies the lhu id in
 */
extern char *func_L00_0025B478(void *, int, int);
extern void func_L00_00263950(char *, char *, int, float, float);
extern float func_001F9878(float);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9938(void *);
extern float func_001F9D10(void *, void *);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern float func_001FA850(float, float);
extern void func_00213DE0(void *, int, int, int);
extern float func_L00_00259148(float *vel, float cur, float target, float k, float d, float max);
extern float func_00214158(void);
extern int func_002140B0(int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_001FF240(void *, void *, void *);
extern void func_L00_00250800(void *, int, void *);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern int func_0022ED80(int, int, int);
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_L01_00279790(void *);
extern void *func_L00_00265050(void *, int, void *, void *, int, int, float, void *, void *, void *);
extern void func_0020D678(void *);

extern int D_L15_001AC140[];
extern int D_L15_00160058_m __asm__("D_L15_00160058") MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L15_0015F660[] MACRO_ADDR;
extern short D_0015EE64_s __asm__("D_0015EE64");
extern short D_L15_00161A80;
extern short D_L15_00161A84;
extern short D_L15_00161A7C;
extern short D_L15_00161A74;
extern short D_L15_00161A78;
extern short D_L15_00161A6C;

/* Construction arm update (moby class 221, level 15): a state machine on moby[0x20]. */
void func_L15_002C3BD8(char *moby) {
    char *data = *(char **)(moby + 0x78);
    unsigned char *um = (unsigned char *)moby;
    int flag = 0;
    if (um[0x20] != 6) {
        char *r = func_L00_0025B478(moby, 0x10000, 0);
        if (r != 0 && 0.0f < *(float *)(r + 0x2C)) flag = 1;
        if (flag) moby[0x20] = 6;
    }
    if (um[0x20] != 0) {
        float g = *(float *)&D_0015EE64_s;
        float v = *(float *)(data + 0x84);
        float a = g * 0.03f;
        float b = g * 0.3f;
        *(float *)(data + 0x68) = v;
        func_L00_00263950(moby, data, 0, a, b);
    }
    switch (um[0x20]) {
    case 0: {
        float a = func_001F9878(*(float *)&D_L15_00161A80 * 60.0f);
        float b = func_001F9878(*(float *)&D_L15_00161A84 * 60.0f);
        int d = func_001FA898_r(func_002140F8(a, b));
        *(short *)(data + 0x80) = d;
        *(int *)(data + 0x84) = 0;
        *(short *)(data + 0x82) = 0;
        *(int *)(data + 0x88) = 0;
        moby[0x20] = 1;
        moby[0x30] = 0xFF;
        break;
    }
    case 1: {
        int count;
        int ok;
        int id;
        if (!func_001F9938(data + 0x80)) break;
        if (!(func_001F9D10(D_L15_00167440, moby + 0x10) < 42.0f)) break;
        {
            int k = um[0x21];
            ok = 1;
            if (k != 0xFF) {
                int lst = D_L15_001AC140[k];
                count = 0;
                if (lst == 0) break;
                {
                    int base = D_L15_00160058_m;
                    unsigned short *p = (unsigned short *)lst;
                    do {
                        char *e;
                        id = *p;
                        e = (char *)(base + ((id & 0x7FFF) << 8));
                        if (e[0x20] >= 0 && *(short *)(e + 0xA6) == *(short *)(moby + 0xA6) && (unsigned char)e[0x20] != 1)
                            count++;
                        p++;
                    } while ((id << 16) >= 0);
                }
                if (count != 0) {
                    *(short *)(data + 0x80) = func_001F9850(0x3C);
                    ok = 0;
                }
            }
        }
        if (!ok) break;
        *(float *)(data + 0x8C) = func_002140F8(-0.209439516f, 0.209439516f);
        moby[0x20] = 2;
        break;
    }
    case 2: {
        int r = func_L00_00258BC8(0, 1);
        *(short *)(data + 0x82) = r;
        if (func_001FA850(*(float *)(data + 0x84), *(float *)(data + 0x8C)) < 0.00872664619f) {
            int k = *(short *)(data + 0x82) * 2 + 1;
            int q = func_001F9850(6);
            func_00213DE0(moby, k, 0, q);
            moby[0x20] = 3;
        }
        {
            float s = D_0015EE70;
            float m = D_0015EE6C;
            *(float *)(data + 0x84) = func_L00_00259148((float *)(data + 0x88), *(float *)(data + 0x84), *(float *)(data + 0x8C), s * 1.57079637f, s * 0.785398185f, m * 0.785398185f);
        }
        break;
    }
    case 3: {
        if (um[0x53] != *(short *)(data + 0x82) * 2 + 1) break;
        if (!(um[0x70] & 2)) break;
        *(short *)(data + 0x80) = func_001F9850(func_L00_00258BC8(0xF, 0x2D));
        {
            int k = *(short *)(data + 0x82) * 2 + 2;
            int q = func_001F9850(3);
            func_00213DE0(moby, k, 0, q);
        }
        *(float *)(data + 0x90) = func_00214158();
        moby[0x20] = 4;
        break;
    }
    case 4: {
        if (func_001F9D10(moby + 0x10, D_L15_00167440) < 20.0f && func_002140B0(2) == 0 && um[0x31] != 0) {
            float A = *(float *)&D_L15_00161A7C;
            float t1 = func_002140F8(A / 5.0f, A);
            float f22 = t1 * D_0015EE6C;
            float t2 = func_002140F8(0.0f, *(float *)&D_L15_00161A7C / 5.0f);
            float f23 = t2 * D_0015EE6C;
            float t3 = func_00214158();
            float vP[4];
            float vA[4];
            float vB[4];
            float vO[4];
            float u1;
            float u2;
            float u3;
            float u4;
            u1 = func_001F9F90(*(float *)(data + 0x90));
            vA[0] = u1 * f22;
            u2 = func_001F9FA8(*(float *)(data + 0x90));
            vA[1] = u2 * f22;
            vA[2] = 0.0f;
            u3 = func_001F9F90(t3);
            vB[0] = u3 * f23;
            u4 = func_001F9FA8(t3);
            vB[1] = u4 * f23;
            vB[2] = 0.0f;
            func_L00_001FF240(vO, vA, vB);
            func_L00_00250800(moby, 1, vP);
            {
                int ra = func_001F9850(*(int *)&D_L15_00161A74);
                int rb = func_001F9850(*(int *)&D_L15_00161A78);
                int c = func_L00_00258BC8(ra, rb);
                func_L00_0026DA50(vP, vA, *(int *)&D_L15_00161A6C, *(int *)&D_L15_00161A6C, c, 1, 40000.0f);
            }
        }
        if (!func_001F9938(data + 0x80)) break;
        {
            int k = *(short *)(data + 0x82) * 2 + 3;
            int q = func_001F9850(6);
            func_00213DE0(moby, k, 0, q);
        }
        moby[0x20] = 5;
        break;
    }
    case 5: {
        float a;
        float b;
        int d;
        int r;
        if (um[0x53] != *(short *)(data + 0x82) * 2 + 3) break;
        if (!(um[0x70] & 2)) break;
        a = func_001F9878(*(float *)&D_L15_00161A80 * 60.0f);
        b = func_001F9878(*(float *)&D_L15_00161A84 * 60.0f);
        d = func_001FA898_r(func_002140F8(a, b));
        *(short *)(data + 0x80) = d;
        r = func_001F9850(6);
        func_00213DE0(moby, 0, 0, r);
        moby[0x20] = 1;
        break;
    }
    case 6:
        func_0022ED80(0, 0, (int)moby);
        func_L00_00260108(moby, moby + 0x10, -1, 1.5f, 13.0f);
        func_L01_00279790(moby);
        func_L00_00265050(moby, 0x74B, moby + 0x10, moby + 0x40, 0, 0, 0.0f, D_L15_0015F660, D_L15_0015F660, D_L15_0015F660);
        func_L00_00265050(moby, 0x74C, moby + 0x10, moby + 0x40, 0, 0, 0.0f, D_L15_0015F660, D_L15_0015F660, D_L15_0015F660);
        func_L00_00265050(moby, 0x74D, moby + 0x10, moby + 0x40, 0, 0, 0.0f, D_L15_0015F660, D_L15_0015F660, D_L15_0015F660);
        func_0020D678(moby);
        break;
    }
}
