/* NON_MATCHING func_L15_002DA238 -- src/overlays/shared/vendor_002D7C00.c
 * Best so far: BYTES 16/696 (97.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   each followed by func_L00_001FD1D8. Same size at best (p4.c, 696 bytes, 410 bytes differ).
 *   Difference: register allocation. Retail keeps the b-vector address in a separate saved reg ($fp)
 *   from the one used in the first calls ($s3), and loop pointers are s0,u1,v2,x3,i5; ours merges
 *   them or gets a different order. Also our stage 2 gets its D_39C0/D_3A00 lui/addiu hoisted above the
 *   func_L00_001FD1D8 call (gcse hoist) while retail recomputes them after func_L00_00258C80.
 *   Would unblock: the original spelling of the loop/pointer variables (maybe a struct local
 *   holding q/w/pr/pkt/b/c/d); 10 runs spent.
 *   The file's position-dependent ordering left one delay-slot choice: retail fills the first call's slot with the
 */
typedef struct { float f[4]; } __attribute__((aligned(16))) V_2da238;
typedef struct { float u, v; } UV_2da238;
typedef struct {
    V_2da238 corner[4];
    unsigned int color[4];
    UV_2da238 uv[4];
    long unk70, tex, unk80, unk88;
} Q_2da238;
extern int func_001F4868(int);
extern void func_001F9CA0(void *, void *, void *);
extern float func_L00_00258C80(float lo, float hi);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_L15_00167440[];
extern V_2da238 D_L15_001D39C0[];
extern UV_2da238 D_L15_001D3A00[];
extern short D_L15_00161D2C;
extern short D_L15_00161D34;
extern short D_L15_00161CE0;
extern short D_L15_00161D38;
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern char D_0013E633[];

/* Draws two flickering glow quads at the moby's data point +0xF0 (func_L18_002ED520 for a moby). */
void func_L15_002DA238(char *m) {
    char *data = *(char **)(m + 0x78);
    V_2da238 *pos = (V_2da238 *)(data + 0xF0);
    Q_2da238 quad;
    V_2da238 mat[4];
    unsigned int c;
    float s;
    int i;
    quad.tex = func_001F4868(0xB);
    quad.unk88 = 0x8000000048L;
    quad.unk80 = 0xFF9000000260L;
    quad.unk70 = 0;
    func_001F9BF0(&mat[3], D_L15_00167440, pos);
    func_L00_001FF4B0(&mat[3], &mat[3], 0.3f);
    func_001F9BD8(&mat[3], &mat[3], pos);
    mat[3].f[3] = 1.0f;
    func_001F9BF0(&mat[0], D_L15_00167440, &mat[3]);
    func_L00_001FF4B0(&mat[0], &mat[0], 1.0f);
    func_001F9CA0(&mat[1], &mat[0], D_0013E633 + 0x10AD);
    func_L00_001FF4B0(&mat[1], &mat[1], -1.0f);
    func_001F9CA0(&mat[2], &mat[1], &mat[0]);
    c = *(int *)&D_L15_00161D2C;
    quad.color[3] = c;
    quad.color[2] = c;
    quad.color[1] = c;
    quad.color[0] = c;
    s = *(float *)&D_L15_00161D34 + func_L00_00258C80(0.0f, 0.025f);
    for (i = 0; i < 4; i++) {
        func_001F9C30(&quad.corner[i], &D_L15_001D39C0[i], s);
        func_001F9EE8(&quad.corner[i], &quad.corner[i], &mat[0]);
        quad.uv[i].u = D_L15_001D3A00[i].u;
        quad.uv[i].v = D_L15_001D3A00[i].v;
    }
    func_L00_001FD1D8(&quad, 0, 0);
    c = *(int *)&D_L15_00161CE0;
    quad.color[3] = c;
    quad.color[2] = c;
    quad.color[1] = c;
    quad.color[0] = c;
    s = *(float *)&D_L15_00161D38 + func_L00_00258C80(0.0f, 0.05f);
    for (i = 0; i < 4; i++) {
        func_001F9C30(&quad.corner[i], &D_L15_001D39C0[i], s);
        func_001F9EE8(&quad.corner[i], &quad.corner[i], &mat[0]);
        quad.uv[i].u = D_L15_001D3A00[i].u;
        quad.uv[i].v = D_L15_001D3A00[i].v;
    }
    func_L00_001FD1D8(&quad, 0, 0);
}
