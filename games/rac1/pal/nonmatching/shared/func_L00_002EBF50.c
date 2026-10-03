/* NON_MATCHING func_L00_002EBF50 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: BYTES 5/372 (98.7% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   CameraScript setup: fills the type-5 moby from two positions, a mode (2 or 3) and two values. p3.c is 5 bytes 
 *   Only difference: in the mode-3 block the copy D_0013E633+0xE9D -> p+0x110 uses dest $a0 / src $v1 in ours, des
 *   Three wordings of that copy (plain, named src local, named dest local) compile to identical bytes: an allocato
 */
extern char D_0013E633[];
extern char *func_L00_001EB578(int);
extern int func_L00_001ED9B0_2(float *, float *) __asm__("func_L00_001ED9B0");

/* Set up a camera script moby from two positions, a mode and two values. */
void func_L00_002EBF50(void *p0, void *p1, int mode, int val, int val2) {
    float a[4] __attribute__((aligned(16)));
    float b[4] __attribute__((aligned(16)));
    float c[4] __attribute__((aligned(16)));
    float d[4] __attribute__((aligned(16)));
    char *m = func_L00_001EB578(5);
    char *p;
    char *q;
    char *t;
    qcopy(a, p0);
    qcopy(b, p1);
    qcopy(c, D_L00_00166EC0);
    qcopy(d, D_L00_00166EC0 + 0x10);
    func_L00_001EB890(m);
    p = *(char **)(m + 0x70);
    q = p + 0xE0;
    t = p + 0x80;
    m[0x7C] = 7;
    m[0x7D] = 1;
    qcopy(m + 0x30, a);
    qcopy(D_L00_00166EC0, a);
    qcopy(m + 0x40, b);
    qcopy(t, a);
    qcopy(p + 0x90, b);
    *(int *)(q + 0x40) = val2;
    m[0x88] = mode;
    if (mode != 2) {
        if (mode != 3) return;
        *(int *)(q + 0x48) = 0;
        *(float *)(q + 0x44) = 1.0f;
        qcopy(m + 0x30, c);
        qcopy(m + 0x40, d);
        { char *w = p + 0x110; qcopy(w, D_0013E633 + 0xE9D); }
        func_L00_001ED9B0_2((float *)(p + 0xFC), c);
    }
    *(int *)(q + 4) = val;
    *(int *)(p + 0xE0) = val;
}
