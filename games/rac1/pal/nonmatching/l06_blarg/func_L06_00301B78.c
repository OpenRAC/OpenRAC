/* NON_MATCHING func_L06_00301B78 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 556 / retail 568, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Culls and draws 96 patches: per patch transforms a vertex by a light matrix, skips back-facing ones, else fill
 *   p4 is the best; left: retail spills the matrix pointer/i to the stack (sw 0xF0/0xF4) and uses 10 saved regs wh
 *   also the long-shift OR chain scheduling and the two inner-loop spills (sq $v1,0x100 / sq $a2,0x110). Needs a s
 */
extern int func_001F4868(int);
extern void func_001FA190(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9C78(void *a, void *b);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L06_00167640[];
extern char D_L06_001FB340[];
extern char D_L06_001FA640[];
extern float D_L06_001F8800[];
extern char D_L06_001F7B00[];
extern short D_L06_001F8200[];
extern short D_L06_001620F4;
extern short D_L06_001620E8;
extern short D_L06_001620E4;
extern short D_L06_001620EC;
extern short D_L06_001620F0;
extern short D_L06_001620F8;
extern short D_L06_001620FC;
extern short D_L06_001620DC;
extern short D_L06_00162104;

// Draws the 96 flagged patches as lit quads: culls back-facing ones and fills a quad packet from tables.
void func_L06_00301B78(void)
{
    struct {
        float q[16];
        int col[4];
        float uv[8];
        long l70, l78, l80, l88;
    } s;
    float lv[16];
    float *light = lv;
    float u[4];
    float w[4];
    int i;
    int k;
    int j;
    short *tp;
    float *uvp;
    char *qp;

    i = 0;
    s.l78 = func_001F4868(*(int *)&D_L06_001620F4);
    s.l88 = (long)*(int *)&D_L06_001620E4 | ((long)*(int *)&D_L06_001620E8 << 2) | ((long)*(int *)&D_L06_001620EC << 4)
          | ((long)*(int *)&D_L06_001620F0 << 6) | ((long)*(int *)&D_L06_001620F8 << 32);
    s.l80 = 0xFF90000000000260L;
    s.l70 = 0;
    func_001FA190(light);
    s.col[0] = *(int *)&D_L06_001620FC;
    light[14] = *(float *)&D_L06_001620DC;
    s.col[3] = s.col[0];
    s.col[2] = s.col[0];
    s.col[1] = s.col[0];
    do {
        k = i * 16;
        i++;
        func_001F9C30(w, D_L06_001FB340 + k, *(float *)&D_L06_00162104);
        func_001F9EE8(w, w, light);
        func_001F9BF0(u, w, D_L06_00167640);
        func_L00_001FF4B0(u, u, 1.0f);
        func_001F9EC0(w, D_L06_001FA640 + k, light);
        if (!(0.0f < func_001F9C78(u, w))) {
            tp = (short *)((char *)D_L06_001F8200 + k);
            uvp = s.uv;
            qp = (char *)s.q;
            for (j = 3; j >= 0; j--) {
                int a = tp[0];
                int b = tp[1];
                func_001F9C30(qp, D_L06_001F7B00 + a * 16, *(float *)&D_L06_00162104);
                tp += 2;
                qp += 16;
                uvp[0] = D_L06_001F8800[b * 2];
                uvp[1] = D_L06_001F8800[b * 2 + 1];
                uvp += 2;
            }
            func_L00_001FD1D8(&s, light, 0);
        }
    } while (i < 0x60);
}
