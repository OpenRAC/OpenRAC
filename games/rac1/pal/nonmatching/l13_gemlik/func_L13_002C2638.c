/* NON_MATCHING func_L13_002C2638 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: BYTES 26/376 (93.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L13_002C2638(x, y, owner, p1, c, p2, w): creates a class-0x52 moby (func_0020D348), fills its data from o
 *   Retail: x->$f21, y->$f20; c=$s3, owner=$s4, p1=$s5, p2=$s6, w=$s7, (m+0x10)=$s2. Ours: x->$f20, c=$s2, (m+0x10
 *   Unblock: some source shape that changes allocation priority of owner/pos/x; looks like an allocator tie.
 */
extern char *func_0020D348(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00251E30(void *);
extern char *func_L00_0026E940(char *src, int col, int w, int v, float x);

// Creates a moby of class 0x52 attached to an owner, copies placement from the owner and the two vectors.
char *func_L13_002C2638(float x, float y, char *owner, void *p1, int c, void *p2, int w) {
    char *m = func_0020D348(0x52);
    if (m != 0) {
        char *data = *(char **)(m + 0x78);
        ((unsigned char *)m)[0x30] = 0xFF;
        *(short *)(m + 0x32) = 0xFF;
        m[0x31] = 1;
        m[0x20] = 0;
        *(int *)(data + 0x24) = c;
        *(char **)(data + 0x20) = owner;
        qcopy(m + 0x10, p1);
        qcopy(m + 0x40, p2);
        *(float *)(data + 0x38) = y;
        *(float *)(data + 0x2C) = x;
        *(int *)(data + 0x28) = w;
        *(int *)(data + 0x30) = 0;
        *(int *)(data + 0x34) = 0;
        qcopy(data, *(char **)(owner + 0x78) + 0xC0);
        if ((unsigned char)m[0x53] != 1)
            func_00213DE0(m, 1, 0, 10);
        func_001F9BD8(m + 0x10, m + 0x10, data);
        if (*(char **)(data + 0x24) != 0)
            qcopy(data + 0x10, *(char **)(data + 0x24) + 0x10);
        *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24);
        func_L00_00251E30(m);
        func_L00_0026E940(m, 0x60808080, w, 0, 500000.0f);
    }
    return m;
}
