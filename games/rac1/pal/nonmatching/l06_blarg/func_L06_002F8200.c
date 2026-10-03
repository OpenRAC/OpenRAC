/* NON_MATCHING func_L06_002F8200 -- src/overlays/l06_blarg/vendor_002B5990.c
 * Best so far: BYTES 59/560 (89.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws nine transformed matrices (GS register setup, a loop building arr[9][16] via func_001FA218/540/9EE8/9BD8
 *   Left: register naming only (retail keeps the arr base in $s7 and the struct pointer in $fp; ours swaps them) p
 */
extern void func_001FA218(float *, float *);
extern void func_001FA1C0(float *, float);
extern void func_001FA540(void *, void *, void *);
extern int func_001F4868(int);
extern void func_00234C98(int, long);
extern void func_001F7868(void);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001FA8A8(int, float);
extern void func_L06_00216B38(void *, void *);
extern float D_L06_001DB3B0[];
extern float D_L06_001DB320[];
extern int D_L06_00161EE0 MACRO_ADDR;
extern int D_L06_00161EE4 MACRO_ADDR;
extern char D_L06_001DB440[];
extern char D_L06_00161ED0[];
extern short D_L06_00161E9C;
extern short D_L06_00161E64;
extern short D_L06_00161E60;
extern short D_L06_00161E68;
extern short D_L06_00161E6C;
extern short D_L06_00161E80;
extern short D_L06_00161E84;
extern short D_L06_00161E88;
extern short D_L06_00161E8C;

// Builds nine transformed copies of a model matrix and draws each one.
void func_L06_002F8200(char *moby) {
    float t[16];
    float m[16];
    float arr[9][16];
    float c[4];
    int st[12];
    char *data = *(char **)(moby + 0x78);
    int i = 8;
    int j;
    float *p;
    float *q;
    float *ta;
    float *tb;
    func_001FA218(t, (float *)(moby + 0x40));
    func_001FA1C0(m, *(float *)&D_L06_00161E9C);
    func_001FA540(t, t, m);
    func_00234C98(6, func_001F4868(0xE));
    func_00234C98(0x14, 0xFF9000000260L);
    func_00234C98(8, 0);
    func_00234C98(0x42, (long)*(int *)&D_L06_00161E60 | ((long)*(int *)&D_L06_00161E64 << 2) | ((long)*(int *)&D_L06_00161E68 << 4) | ((long)*(int *)&D_L06_00161E6C << 6) | 0x8000000000L);
    func_001F7868();
    p = arr[0];
    q = arr[0] + 12;
    ta = D_L06_001DB3B0;
    tb = D_L06_001DB320;
    for (; i >= 0; i--) {
        func_001FA218(p, ta);
        ta += 4;
        func_001FA540(p, p, t);
        func_001F9EE8(c, tb, t);
        tb += 4;
        func_001F9BD8(q, c, moby + 0x10);
        q += 16;
        p[15] = 1.0f;
        p += 16;
    }
    st[3] = func_001FA8A8(*(int *)&D_L06_00161E80 & 0xFFFFFF, *(float *)(data + 0x18));
    st[0] = D_L06_00161EE0;
    st[1] = D_L06_00161EE4;
    st[2] = (int)D_L06_00161ED0;
    st[4] = *(int *)&D_L06_00161E84;
    *(float *)&st[7] = *(float *)&D_L06_00161E88;
    *(float *)&st[8] = *(float *)&D_L06_00161E8C;
    st[5] = 0x1E;
    st[6] = (int)D_L06_001DB440;
    p = arr[0];
    for (j = 8; j >= 0; j--) {
        func_L06_00216B38(st, p);
        p += 16;
    }
}
