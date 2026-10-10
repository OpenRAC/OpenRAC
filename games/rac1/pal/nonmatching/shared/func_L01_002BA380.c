/* NON_MATCHING func_L01_002BA380 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 1284 / retail 1272, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped at budget (10 of 10). Ripple quad-builder: 4-vertex stack quad, three calls per vertex, per-bit tile e
 *   Remaining differences: retail keeps the next-entry pointer (p+0x1190) in a register it spills to nothing, whil
 */
extern float D_L01_001CAF80[];
extern char D_L01_001672C0[];
extern short D_L01_001612A0;
extern short D_L01_001612A8;
extern short D_L01_001612AC;
extern short D_L01_001612B0;
extern short D_L01_001612B4;
extern char *D_L01_00161240 MACRO_ADDR;
extern void func_L01_002BA150(char *a, int b);
extern void func_L01_00262DA8(char *a);
extern void func_L01_002B9E68(char *a, int b, int c);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern int func_001F9B20(float *);
extern void func_L01_00262BC0(char *a, char *b);
extern void func_L01_002B9DC0(float *pos);
extern void func_L01_002B9288(float *m);
extern int func_001F4868(int);

// Draws the active ripple patches: transforms each quad, emits its tiles to the GS packet buffer.
void func_L01_002BA380(char *base, int n) {
    char *p;
    char *nx;
    int i;
    int k;
    unsigned int id21;
    unsigned int id23;
    float quad[16];
    float oa[4];
    float ob[4];
    char *v;
    char *tex;
    int f19;
    char *g;
    int j;
    int cnt;
    char *a;
    float fx, fy, fz, d2, d3, x0, x1, y0, y1;
    int ra, rb;

    func_L01_002BA150(base, n);
    p = base;
    for (i = 0; i < n; i++, p = nx) {
        nx = p + 0x1190;
        if (*(unsigned short *)(p + 0x1E) != 0) {
            tex = D_L01_001672C0;
            id21 = 0;
            id23 = 0;
            fx = *(float *)p;
            d2 = D_L01_001CAF80[2];
            d3 = D_L01_001CAF80[3];
            fy = *(float *)(p + 4);
            fz = *(float *)(p + 8);
            x0 = fx + d2;
            x1 = fx - d2;
            y0 = fy + d3;
            y1 = fy - d3;
            quad[0] = x0;
            quad[1] = y0;
            quad[5] = y0;
            quad[8] = x0;
            quad[12] = x1;
            quad[13] = y1;
            quad[14] = fz;
            quad[2] = fz;
            quad[3] = 1.0f;
            quad[4] = x1;
            quad[6] = fz;
            quad[7] = 1.0f;
            quad[9] = y1;
            quad[10] = fz;
            quad[11] = 1.0f;
            quad[15] = 1.0f;
            v = (char *)quad;
            k = 3;
            do {
                func_001F9BF0(oa, v, tex);
                v += 0x10;
                k--;
                func_001F9C30(ob, oa, 1024.0f);
                func_001F9C30(oa, oa, 1024.0f);
                func_001F9EE8(ob, ob, tex - 0x80);
                func_001F9EE8(oa, oa, tex - 0x40);
                ra = func_001F9B20(ob);
                id23 = (id23 << 8) | ra;
                rb = func_001F9B20(oa);
                id21 = (id21 << 8) | rb;
            } while (k >= 0);
            if (!(((id21 & 0x1010101) == 0x1010101) || ((id21 & 0x2020202) == 0x2020202) ||
                  ((id21 & 0x4040404) == 0x4040404) || ((id21 & 0x8080808) == 0x8080808) ||
                  ((id21 & 0x20202020) == 0x20202020))) {
                func_L01_00262BC0((char *)0x70000000, p + (*(int *)&D_L01_001612A0 * 0x5C0 + 0x50));
                func_L01_00262DA8(p);
                func_L01_002B9DC0((float *)p);
                func_L01_002B9288((float *)p);
                if (*(int *)(p + 0x44) & 1) {
                    a = (char *)0x70002008;
                    j = 0x10;
                    do {
                        j--;
                        *(float *)a = *(float *)(p + 8);
                        a += 0xC;
                    } while (j >= 0);
                }
                if (*(int *)(p + 0x44) & 2) {
                    a = (char *)0x700020C8;
                    j = 0x10;
                    do {
                        j--;
                        *(float *)a = *(float *)(p + 8);
                        a += 0xCC;
                    } while (j >= 0);
                }
                if (*(int *)(p + 0x44) & 4) {
                    a = (char *)0x70002CC8;
                    j = 0x10;
                    do {
                        j--;
                        *(float *)a = *(float *)(p + 8);
                        a += 0xC;
                    } while (j >= 0);
                }
                if (*(int *)(p + 0x44) & 8) {
                    a = (char *)0x70002008;
                    j = 0x10;
                    do {
                        j--;
                        *(float *)a = *(float *)(p + 8);
                        a += 0xCC;
                    } while (j >= 0);
                }
                f19 = (id23 & 0x2F2F2F2F) != 0;
                if (!f19) {
                    f19 = 2;
                    if ((id21 & 0x2F2F2F2F) == 0) {
                        f19 = 0;
                    }
                }
                cnt = *(unsigned short *)(p + 0x1E);
                if (!(*(int *)(p + 0x14) == *(int *)&D_L01_001612A8 &&
                      *(int *)(p + 0x18) == *(int *)&D_L01_001612AC &&
                      *(unsigned char *)(p + 0x1C) == *(int *)&D_L01_001612B0 &&
                      *(unsigned char *)(p + 0x1D) == *(int *)&D_L01_001612B4)) {
                    *(int *)D_L01_00161240 = 0x10000005;
                    *(int *)(D_L01_00161240 + 4) = 0;
                    *(int *)(D_L01_00161240 + 8) = 0x11000000;
                    *(int *)(D_L01_00161240 + 0xC) = 0x50000005;
                    g = D_L01_00161240;
                    D_L01_00161240 = g + 0x10;
                    *(u64 *)(g + 0x10) = ((u64)0x8000 << 47) | 0x8001;
                    *(u64 *)(g + 0x18) = 0xEEEE;
                    *(u64 *)(g + 0x20) = ((u64)*(unsigned char *)(p + 0x1D) << 32) | 0x64;
                    *(u64 *)(g + 0x28) = 0x42;
                    *(u64 *)(g + 0x30) = ((u64)*(unsigned char *)(p + 0x1C) << 32) | 0x64;
                    *(u64 *)(g + 0x38) = 0x43;
                    *(u64 *)(g + 0x40) = (u64)func_001F4868(*(int *)(p + 0x18));
                    *(u64 *)(g + 0x48) = 6;
                    *(u64 *)(g + 0x50) = (u64)func_001F4868(*(int *)(p + 0x14));
                    *(u64 *)(g + 0x58) = 7;
                    g += 0x60;
                    D_L01_00161240 = g;
                    *(int *)&D_L01_001612A8 = *(int *)(p + 0x14);
                    *(int *)&D_L01_001612AC = *(int *)(p + 0x18);
                    *(int *)&D_L01_001612B0 = *(unsigned char *)(p + 0x1C);
                    *(int *)&D_L01_001612B4 = *(unsigned char *)(p + 0x1D);
                }
                for (j = 0; j < 16; j++) {
                    if (cnt & 1) {
                        func_L01_002B9E68(p, j, f19);
                    }
                    cnt = cnt >> 1;
                }
            }
        }
    }
    *(int *)D_L01_00161240 = 0x10000000;
    *(int *)(D_L01_00161240 + 4) = 0;
    *(int *)(D_L01_00161240 + 8) = 0x13000000;
    *(int *)(D_L01_00161240 + 0xC) = 0;
    D_L01_00161240 = D_L01_00161240 + 0x10;
}
