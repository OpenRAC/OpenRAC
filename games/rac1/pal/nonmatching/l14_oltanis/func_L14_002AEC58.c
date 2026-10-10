/* NON_MATCHING func_L14_002AEC58 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 4/816 (99.5% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level-14 moby update: calls func_L14_002AF2E8, copies two vectors, runs two loops that fill a 3-float table an
 *   Left: the two loops' induction pointers (retail keeps the index array in $17 and the table in $23 with separat
 *   Unblock: a tie in the scheduler's order of the induction-pointer increments; rewording did not move it within 
 *   hq12/s08 (5 runs, fresh budget): p7 (int *ip = ind inside the first loop, used for the index store) closes the
 *   Tried on top of p7: per-iteration pointer for the second loop's stores (p8), a block-scoped pointer for it (p9
 *   Unblock: a form that makes the compiler recompute sp+0x40 for loop two instead of reusing the base it CSEs acr
 */
extern void func_L14_002AF2E8(char *moby);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
u64 func_001F4868(s32);
extern void func_00234C98(int, long);
extern void func_001F7868(void);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FDE48(int, int, int, void *, int);
extern void func_001F9C30(void *, void *, float);
extern char D_L14_001675C0[];
extern float D_L14_001D7D60[];
extern int D_L14_001D83C8[];
extern char D_L14_001D8138[];
extern short D_L14_001614E0;

/* Rebuilds the vertex table for a level-14 moby and emits it through the draw helpers. */
void func_L14_002AEC58(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float v[4];
    float b[4];
    float a[4];
    float w[4];
    float c[248];
    int ind[84];
    float t[4];
    int i;
    int j;

    func_L14_002AF2E8(moby);
    qcopy(v, data + 0xA0);
    func_001F9BF0(a, data + 0xD0, D_L14_001675C0);
    func_001F9CA0(b, v, a);
    func_L00_001FF4B0(b, b, 1.0f);
    func_001F9CA0(a, b, v);
    qcopy(w, data + 0xD0);
    w[3] = 1.0f;
    func_00234C98(6, func_001F4868(22));
    func_00234C98(0x42, ((long)0x8000 << 24) | 0x48);
    func_00234C98(8, 5);
    func_00234C98(0x14, ((long)0xFF90 << 32) | 0x260);
    func_00234C98(0x47, 0x50000);
    func_00234C98(0x4A, 0);
    func_001F7868();

    for (i = 0; i < *(int *)&D_L14_001614E0; i++) {
        int *ip = ind;
        float *cp1 = c;
        t[0] = D_L14_001D7D60[i * 3 + 0];
        t[1] = D_L14_001D7D60[i * 3 + 1];
        t[2] = D_L14_001D7D60[i * 3 + 2];
        t[3] = 1.0f;
        func_001F9EE8(t, t, v);
        cp1[i * 3 + 0] = t[0];
        cp1[i * 3 + 1] = t[1];
        cp1[i * 3 + 2] = t[2];
        if (*(short *)(data + 0x202) != 0) {
            ip[i] = D_L14_001D83C8[i] & 0xFF0000FF;
        }
    }

    if (*(short *)(data + 0x202) != 0) {
        func_L00_001FDE48(*(int *)&D_L14_001614E0, (int)c, (int)ind, D_L14_001D8138, 1);
    } else {
        func_L00_001FDE48(*(int *)&D_L14_001614E0, (int)c, (int)D_L14_001D83C8, D_L14_001D8138, 1);
    }

    func_001F9C30(b, b, -1.0f);
    func_001F9C30(a, a, -1.0f);

    for (j = 0; j < *(int *)&D_L14_001614E0; j++) {
        t[0] = D_L14_001D7D60[j * 3 + 0];
        t[1] = D_L14_001D7D60[j * 3 + 1];
        t[2] = D_L14_001D7D60[j * 3 + 2];
        t[3] = 1.0f;
        func_001F9EE8(t, t, v);
        c[j * 3 + 0] = t[0];
        c[j * 3 + 1] = t[1];
        c[j * 3 + 2] = t[2];
    }

    if (*(short *)(data + 0x202) != 0) {
        func_L00_001FDE48(*(int *)&D_L14_001614E0, (int)c, (int)ind, D_L14_001D8138, 1);
    } else {
        func_L00_001FDE48(*(int *)&D_L14_001614E0, (int)c, (int)D_L14_001D83C8, D_L14_001D8138, 1);
    }
}
