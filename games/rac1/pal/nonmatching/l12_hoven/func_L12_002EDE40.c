/* NON_MATCHING func_L12_002EDE40 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 540 / retail 548, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L12_002EDE40: builds a 4-vertex GS-style quad struct (verts, colors, uv, 4 u64 tag words) and draws a rin
 *   Best is p5.c (89/548 bytes counted, structure right): locals order Quad,m[12],pos[4]; u64 as long. Left: init 
 *   Try: loop as do{}while(--i>=0) with different pointer locals; budget spent.
 */
extern void func_001FA460(void *, void *);
extern int func_001F4868(int);
extern float func_001F9FA8(float);
extern int func_001FA8A8(int, int, float);
extern float func_001F9F90(float);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L12_001F5DE0[];
extern short D_L12_00161A8C;
extern short D_L12_00161A90;
extern short D_L12_00161A94;
extern short D_L12_00161A98;
extern short D_L12_00161A9C;
extern short D_L12_00161AA0;
extern short D_L12_00161AA4;

typedef int u128 __attribute__((mode(TI)));

typedef struct {
    float v[16];
    int c[4];
    float uv[8];
    long g[4];
} Quad;

// Draws a ring of four-vertex strips around the moby with a lit colour.
void func_L12_002EDE40(char *moby) {
    char *data = *(char **)(moby + 0x78);
    Quad q;
    float m[12];
    float pos[4];
    float a, b;
    int i, col;
    func_001FA460(m, moby + 0xC0);
    *(u128 *)pos = *(u128 *)(moby + 0x10);
    pos[2] = pos[2] + *(float *)(data + 8);
    pos[3] = 1.0f;
    q.g[1] = func_001F4868(0x10);
    q.g[0] = 0;
    q.g[2] = 0xFF90000000000260UL;
    q.g[3] = (long)*(int *)&D_L12_00161A8C | ((long)*(int *)&D_L12_00161A90 << 2) | ((long)*(int *)&D_L12_00161A94 << 4) | ((long)*(int *)&D_L12_00161A98 << 6) | (0x8000L << 24);
    col = func_001FA8A8(*(int *)&D_L12_00161A9C, *(int *)&D_L12_00161AA0, (func_001F9FA8(*(float *)data) + 1.0f) * 0.5f);
    for (i = 0; i < 4; i++) {
        q.uv[i * 2] = 0.5f;
        q.uv[i * 2 + 1] = 0.5f;
        q.c[i] = col;
        *(u128 *)&q.v[i * 4] = *(u128 *)(D_L12_001F5DE0 + i * 16);
    }
    for (a = -0.4f; a < 0.36f; a += 0.1f) {
        b = a + 0.1f;
        q.v[1] = a;
        q.v[9] = a;
        q.v[13] = b;
        q.v[5] = b;
        if (moby[0xBC] & 1) {
            q.v[2] = -func_001F9F90(a * *(float *)&D_L12_00161AA4) + 2.0f;
            q.v[6] = -func_001F9F90(b * *(float *)&D_L12_00161AA4) + 2.0f;
        } else {
            q.v[2] = func_001F9F90(a * *(float *)&D_L12_00161AA4);
            q.v[6] = func_001F9F90(b * *(float *)&D_L12_00161AA4);
        }
        func_L00_001FD1D8(&q, m, 0);
    }
}
