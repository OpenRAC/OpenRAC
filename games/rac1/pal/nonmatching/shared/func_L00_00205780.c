/* NON_MATCHING func_L00_00205780 -- src/overlays/shared/help_00203E98.c
 * Best so far: SIZE ours 888 / retail 892, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00205780: physics-body settle step. If the body is at rest (six velocity floats zero, three speeds an
 *   Best candidate p4.c (run 5): every instruction matches except (a) retail leaves all the float-compare `bc1f` d
 *   Would unblock: whatever makes reorg skip the eager target-fill in retail (a reorg/liveness difference, possibl
 */
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char D_0015EEB4[] NOT_SDA;
extern float D_0015EE64 MACRO_ADDR;
extern char *func_L00_00205728(int);
extern float func_001F9B88(float);
extern void func_0020D9D8(void *, void *);
extern float func_L00_0025CCF0(char *, char *, int, float, float, float, float);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_0020D960(char *, int, void *);
extern void func_L00_0025AFA8(void *a, float *v);
extern void func_001F9BC0(float *);

/* Settles a physics body: snaps tiny velocities to rest or integrates and clamps them. */
void func_L00_00205780(char *m) {
    char *v = func_L00_00205728(*(short *)(m + 0xA2));
    if (v == 0) return;
    if (D_0013E633[0x2EC1] == 0) {
        int s = *(short *)(m + 0xA2);
        if (s == 4 || s == 2 || s == 3) return;
    } else {
        if (*(short *)(m + 0xA2) == 0) return;
    }
    if (*(float *)(m + 0x60) == 0.0f && *(float *)(m + 0x64) == 0.0f && *(float *)(m + 0x68) == 0.0f
        && *(float *)(m + 0x90) == 0.0f && *(float *)(m + 0x94) == 0.0f && *(float *)(m + 0x98) == 0.0f
        && func_001F9B88(*(float *)(m + 0x70)) < 0.003f
        && func_001F9B88(*(float *)(m + 0x74)) < 0.003f
        && func_001F9B88(*(float *)(m + 0x78)) < 0.003f
        && *(float *)(m + 0xAC) == 1.0f
        && func_001F9B88(*(float *)(m + 0x40)) < 0.005f
        && func_001F9B88(*(float *)(m + 0x44)) < 0.005f
        && func_001F9B88(*(float *)(m + 0x48)) < 0.005f) {
        if (*(unsigned char *)(m + 1) != 0) func_0020D9D8(v, m);
    } else {
        if (D_0015EEB4[1] != 0) {
            *(float *)(m + 0x60) = -*(float *)(m + 0x60);
            *(float *)(m + 0x68) = -*(float *)(m + 0x68);
            *(float *)(m + 0x74) = -*(float *)(m + 0x74);
        }
        func_L00_0025CCF0(m + 0x40, m + 0x50, 2, *(float *)(m + 0x60), *(float *)(m + 0xA4) * D_0015EE64, *(float *)(m + 0xA8) * D_0015EE64, 0.0f);
        {
            float u = *(float *)(m + 0xA4) * D_0015EE64;
            float w = *(float *)(m + 0xA8) * D_0015EE64;
            func_L00_0025CCF0(m + 0x44, m + 0x54, 2, *(float *)(m + 0x64), u, w, 0.0f);
        }
        func_L00_0025CCF0(m + 0x48, m + 0x58, 2, *(float *)(m + 0x68), *(float *)(m + 0xA4) * D_0015EE64, *(float *)(m + 0xA8) * D_0015EE64, 0.0f);
        func_L00_0025C918((float *)(m + 0x70), (float *)(m + 0x80), *(float *)(m + 0x90), *(float *)(m + 0xA4), *(float *)(m + 0xA8), 0.0f);
        func_L00_0025C918((float *)(m + 0x74), (float *)(m + 0x84), *(float *)(m + 0x94), *(float *)(m + 0xA4), *(float *)(m + 0xA8), 0.0f);
        func_L00_0025C918((float *)(m + 0x78), (float *)(m + 0x88), *(float *)(m + 0x98), *(float *)(m + 0xA4), *(float *)(m + 0xA8), 0.0f);
        if (*(unsigned char *)(m + 1) == 0) func_0020D960(v, *(short *)(m + 0xA0), m);
        func_L00_0025AFA8(m + 0x10, (float *)(m + 0x40));
        qcopy(m + 0x30, m + 0x70);
        *(float *)(m + 0x20) = *(float *)(m + 0xAC);
        *(float *)(m + 0x24) = *(float *)(m + 0xAC);
        *(float *)(m + 0x28) = *(float *)(m + 0xAC);
    }
    func_001F9BC0((float *)(m + 0x90));
    func_001F9BC0((float *)(m + 0x60));
    *(float *)(m + 0xAC) = 1.0f;
}
