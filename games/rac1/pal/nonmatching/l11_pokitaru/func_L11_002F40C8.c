/* NON_MATCHING func_L11_002F40C8 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 8/740 (98.9% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    float v[16];
    int c[4];
    float uv[8];
    long g[4];
} Quad_2f40c8;
extern short D_L11_001619B0, D_L11_001619B4, D_L11_001619B8, D_L11_001619BC, D_L11_001619C0, D_L11_001619C4;
extern short D_L11_001619C8;
extern float D_L11_001D3800[];
extern unsigned char D_001414F5_r __asm__("D_001414F5") NOT_SDA;
extern void func_001FA460(void *, void *);
extern long func_001F4868(int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L00_001FD1D8(void *, void *, int);

/* Draws the moby's shimmering ring as 32 four-vertex strips around its radius, thicker as the ring
 * shrinks below 1.15, and flags when it gets below 0.65. */
void func_L11_002F40C8(char *moby) {
    Quad_2f40c8 q;
    float m[12];
    float pos[4];
    char *d;
    float h;
    int i, j;
    d = *(char **)(moby + 0x78);
    func_001FA460(m, moby + 0xC0);
    qcopy(pos, moby + 0x10);
    q.g[1] = func_001F4868(*(int *)&D_L11_001619C0);
    q.g[3] = (long)*(int *)&D_L11_001619B0 | ((long)*(int *)&D_L11_001619B4 << 2) | ((long)*(int *)&D_L11_001619B8 << 4) | ((long)*(int *)&D_L11_001619BC << 6) | (0x8000L << 24);
    q.g[2] = 0xFF9000000260UL;
    q.g[0] = 0;
    for (i = 0; i < 4; i++) {
        q.uv[i * 2] = D_L11_001D3800[i * 2];
        *(&q.uv[i * 2] + 1) = D_L11_001D3800[i * 2 + 1];
        q.c[i] = *(int *)&D_L11_001619C4;
    }
    h = 0.1f;
    if (*(float *)(d + 0x3C) < 1.15f) {
        float t = (1.15f - *(float *)(d + 0x3C)) * 2.0f;
        if (1.0f < t) {
            t = 1.0f;
        } else if (t < 0.0f) {
            t = 0.0f;
        }
        h = t + 0.1f;
        if (*(float *)(d + 0x3C) < 0.65f) {
            D_001414F5_r = 1;
        } else {
            D_001414F5_r = 0;
        }
    }
    for (j = 0; j < 32; j++) {
        int k;
        float *p;
        for (k = 0, p = q.v; k < 4;) {
            float a = (float)(j + k / 2) * 6.28318f * 0.03125f - 3.14159f;
            p[0] = func_001F9F90(a) * *(float *)(d + 0x3C);
            p[1] = func_001F9FA8(a) * *(float *)(d + 0x3C);
            p[2] = 0.0f;
            if (k++ & 1) {
                p[2] = *(float *)&D_L11_001619C8 + h + p[2];
            } else {
                p[2] = *(float *)&D_L11_001619C8 - h + p[2];
            }
            p[3] = 1.0f;
            p += 4;
        }
        func_L00_001FD1D8(&q, m, 0);
    }
}
