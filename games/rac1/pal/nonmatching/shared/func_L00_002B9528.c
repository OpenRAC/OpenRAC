/* NON_MATCHING func_L00_002B9528 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: SIZE ours 524 / retail 520, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby update: state 2 retires the moby (frees handles, clears slot owner); otherwise counts an idle timer in d+
 *   Differences are register-only: retail has m in $s2 and d (the +0x78 data pointer) in $s1, mine the reverse, an
 *   Retail reads the same global both gp-relative and lui-relative (two decls: short alias and NOT_SDA alias); it 
 *   Wave lb1/p05: the context changed under the old best p9: src/overlays/shared/vendor_002B33E8.c now declares `e
 *   Tried p10 (d loaded after the state test) and p11 (local copy of the parameter): still m in $s1, d in $s2 (ret
 *   Unblock (lead): the same symbol cannot be both gp-relative and lui/lw in one file while another function has a
 */
extern void func_0020D678(int);
extern void func_L00_0023F1D0(int);
extern void func_L00_0028EBF0(int);
extern int func_001F9850(int);
extern int func_L00_00203F20(int, int);
extern char D_0013E633[] NOT_SDA;
extern char D_0014171B[] NOT_SDA;
extern char D_0014171B_b[] __asm__("D_0014171B") NOT_SDA;
extern short D_0015EFA4_g __asm__("D_0015EFA4");
extern int D_0015EFA4_far __asm__("D_0015EFA4") NOT_SDA;
extern short D_0015EE84_g __asm__("D_0015EE84");
extern int D_0015EE84_far __asm__("D_0015EE84") NOT_SDA;
/* moby update: retire a finished one, else tick the idle counter and possibly raise an event flag */
void func_L00_002B9528(char *m) {
    char *d = *(char **)(m + 0x78);
    char *g;
    int s;
    int a;
    int b;
    int r;
    int t;
    unsigned u;
    unsigned char *c;
    if (*(unsigned char *)(m + 0x53) == 2) {
        int i;
        *(unsigned char *)(m + 0x20) = 4;
        if (*(int *)d != 0) func_0020D678(*(int *)d);
        if (*(short *)(d + 0x44) != -1) {
            func_L00_0023F1D0(*(short *)(d + 0x44));
            *(short *)(d + 0x44) = -1;
        }
        i = *(short *)(d + 0x4A);
        if (i != -1) {
            char *e = D_0013E633 + 0x1D + i * 0x70;
            if (*(char **)(e + 0x88) == m && *(unsigned char *)(e + 0x74) != 0) func_L00_0028EBF0(i);
        }
        *(short *)(d + 0x4A) = -1;
        return;
    }
    if (*(unsigned short *)(D_0014171B + 0x5BD) != 0) return;
    g = D_0014171B_b + 0x22D;
    if (*(unsigned short *)(g + 0x50) != 0) return;
    if (*(unsigned char *)(m + 0x20) == 3) {
        *(unsigned short *)(d + 0x50) = *(unsigned short *)(d + 0x50) + 1;
        return;
    }
    s = *(short *)(d + 0x50);
    if (s == 0) return;
    if (func_001F9850(0x78) < s) {
        u = *(unsigned short *)(g + 0x50);
        a = *(int *)&D_0015EFA4_g;
        if (u < 0xFFFF) {
            *(unsigned short *)(g + 0x50) = u + 1;
            a = D_0015EFA4_far;
        }
        r = func_001F9850(a) / 600;
        b = *(int *)&D_0015EE84_g;
        if (*(unsigned short *)(g + 0x52) < r) {
            *(unsigned short *)(g + 0x52) = func_001F9850(*(int *)&D_0015EFA4_g) / 600;
            b = D_0015EE84_far;
        }
        c = (unsigned char *)(D_0013E633 + 0xE1D);
        *(unsigned *)(g + 0x54) = (*(unsigned *)(g + 0x54) | (1 << b)) | 0x80000000;
    } else {
        t = *(short *)(d + 0x50);
        r = func_001F9850(0x1E);
        c = (unsigned char *)(D_0013E633 + 0xE1D);
        if (t < r) c[0x20B4]++;
    }
    *(short *)(d + 0x50) = 0;
    c = (unsigned char *)(D_0013E633 + 0xE1D);
    if (c[0x20B4] >= 3) func_L00_00203F20(0x4E24, 0x4E);
}
