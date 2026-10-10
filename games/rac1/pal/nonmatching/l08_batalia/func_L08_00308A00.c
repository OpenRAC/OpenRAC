/* NON_MATCHING func_L08_00308A00 -- src/overlays/l08_batalia/vendor_002EAF48.c
 * Best so far: SIZE ours 1632 / retail 1612, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Level 8 moby update (class 1349): reads the hero checks (state 0/1 at d+0x10), then four per-channel timers in
 *   Best so far p2.c (block-local k and table pointer per timer block): size 1636 against retail 1612 (24 bytes ov
 *   - Prologue: ours frames 0xA0 and keeps one more saved register (its $s7 is retail's $s6 = hi(hero2)); retail's
 *   - Block registers: retail keeps 600 in a saved register across blocks and divides by it with the divide-by-zer
 *   - Case 0/1 layout: retail's `beql`/`bnel` forms for the hero-state tests do not appear in ours.
 *   Unblock: how retail divides by the 600 step (a variable rather than a literal?), then the saved-register count
 *   Runs: 5 of 10 used on this function (p0 to p3, one repeated compile by mistake). Stopped with budget left: the
 *   Housekeeping: a copy of the claim printout was saved by mistake to build-sn/try/.claim_out_s01_2.txt (outside 
 */
typedef int u128 __attribute__((mode(TI)));

typedef struct L08Dat {
    int v0;
    int v4;
    int v8;
    int vc;
    int state;
} L08Dat;

extern char D_0013E633[];
extern char D_0013D50F[];
extern char D_0013D355[];
extern char D_0014171B[];
extern int D_0015EFA4 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern int func_00215570(void *arg0, int arg1);
extern int func_0022EE28(int, int, int);
extern void func_L00_00264DB8(int, int);
extern int func_001F9850(int);
extern int func_L00_00203F20(int a, int b);
extern float func_001F9C78(void *a, void *b);

/* Level 8 moby update (class 1349): hero checks, then the per-channel timers in the level table. */
void func_L08_00308A00(char *m) {
    L08Dat *d = *(L08Dat **)(m + 0x78);
    unsigned char *b9;
    char *T;
    unsigned char *fl;
    int st;
    int r;
    int rA;
    int rB;
    int rC;
    int rD;
    int i;
    int diff;
    float f;
    float tmp[4];

    st = d->state;
    if (st == 0)
        goto L44;
    if (st == 1)
        goto L7C;
    goto AE8;

L44:
    r = func_00215570(D_0013E633 + 0xE9D, d->v8);
    if (r == 0)
        goto AE8;
    if (*(int *)(D_0013E633 + 0xE9D + 0x200C) != 0xF)
        goto AE8;
    d->state = 1;
    goto AE8;

L7C:
    if (*(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x42) {
        d->state = 0;
        goto AE8;
    }
    r = func_00215570(D_0013E633 + 0xE1D + 0x80, d->vc);
    if (r == 0)
        goto AE8;
    fl = D_0013D50F + 1;
    if (fl[0xB] != 0)
        goto AE8;
    fl[0xB] = (unsigned char)st;
    func_0022EE28(1, 0, 0);
    func_L00_00264DB8(0x53DB, -1);
    goto AE8;

AE8:
    b9 = D_0013D50F + 0xB9;
    if (b9[0x1D] != 0)
        goto BE8;
    r = func_00215570(D_0013E633 + 0xE9D, d->v0);
    if (r == 0)
        goto BD8;
    rA = func_001F9850(D_0015EFA4);
    T = D_0014171B + 0x34D;
    diff = rA - *(unsigned short *)(T + 0x182) * 600;
    rB = func_001F9850(0x12);
    i = (int)((float)rB * 60.0f);
    if (i < diff)
        goto B78;
    if (*(unsigned short *)(T + 0x182) * 600 != 0)
        goto B88;
B78:
    func_L00_00203F20(0x1F40, 0x30);
    goto BD8;
B88:
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x182) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x182) = rD / 600;
    }
    b9 = D_0013D50F + 0xB9;
BD8:
    if (b9[0x1D] == 0)
        goto D4C;
BE8:
    r = func_00215570(D_0013E633 + 0xE9D, d->v0);
    if (r == 0)
        goto D4C;
    fl = D_0013D355 + 0x13B;
    if (fl[0x2F] != 0)
        goto D4C2;
    T = D_0014171B + 0x34D;
    if (*(unsigned short *)(T + 0x188) == 0)
        goto CD8;
    rA = func_001F9850(D_0015EFA4);
    diff = rA - *(unsigned short *)(T + 0x18A) * 600;
    rB = func_001F9850(0x12);
    i = (int)((float)rB * 60.0f);
    if (i < diff)
        goto C78;
    if (*(unsigned short *)(T + 0x18A) * 600 != 0)
        goto C88;
C78:
    func_L00_00203F20(0x1F41, 0x31);
    goto D50;
C88:
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x18A) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x18A) = rD / 600;
    }
    goto D50;
CD8:
    *(unsigned short *)(T + 0x188) = *(unsigned short *)(T + 0x188) + 1;
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x18A) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x18A) = rD / 600;
    }
    *(int *)(T + 0x18C) = *(int *)(T + 0x18C) | (1 << D_0015EE84) | 0x80000000;
    goto D50;
D4C:
    b9 = D_0013D50F + 0xB9;
D4C2:
D50:
    if (b9[0x1C] != 0)
        goto E50;
    r = func_00215570(D_0013E633 + 0xE9D, d->v4);
    if (r == 0)
        goto E3C;
    if (D_0013DE4B[7] == 0)
        goto E3C;
    rA = func_001F9850(D_0015EFA4);
    T = D_0014171B + 0x34D;
    diff = rA - *(unsigned short *)(T + 0x192) * 600;
    rB = func_001F9850(0x12);
    i = (int)((float)rB * 60.0f);
    if (i < diff)
        goto DDC;
    if (*(unsigned short *)(T + 0x192) * 600 != 0)
        goto DEC;
DDC:
    func_L00_00203F20(0x1F42, 0x32);
    goto FB0;
DEC:
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x192) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x192) = rD / 600;
    }
    goto FB0;
E3C:
    b9 = D_0013D50F + 0xB9;
    if (b9[0x1C] == 0)
        goto FB0;
E50:
    r = func_00215570(D_0013E633 + 0xE9D, d->v4);
    if (r == 0)
        goto FB0;
    if (D_0013DE4B[7] == 0)
        goto FB0;
    T = D_0014171B + 0x34D;
    if (*(unsigned short *)(T + 0x380) == 0)
        goto F38;
    rA = func_001F9850(D_0015EFA4);
    diff = rA - *(unsigned short *)(T + 0x382) * 600;
    rB = func_001F9850(0x12);
    i = (int)((float)rB * 60.0f);
    if (i < diff)
        goto ED8;
    if (*(unsigned short *)(T + 0x382) * 600 != 0)
        goto EE8;
ED8:
    func_L00_00203F20(0x1F45, 0x70);
    goto FB0;
EE8:
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x382) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x382) = rD / 600;
    }
    goto FB0;
F38:
    *(unsigned short *)(T + 0x380) = *(unsigned short *)(T + 0x380) + 1;
    rC = func_001F9850(D_0015EFA4);
    if (*(unsigned short *)(T + 0x382) < rC / 600) {
        rD = func_001F9850(D_0015EFA4);
        *(unsigned short *)(T + 0x382) = rD / 600;
    }
    *(int *)(T + 0x384) = *(int *)(T + 0x384) | (1 << D_0015EE84) | 0x80000000;
FB0:
    *(u128 *)tmp = 0;
    tmp[2] = -1.0f;
    f = func_001F9C78(D_0013E633 + 0x10AD, tmp);
    if (f < 0.1f)
        *(unsigned short *)(D_0014171B + 0x6CD) = 0xFFFF;
    fl = D_0013D355 + 0x13B;
    if (fl[0x2F] != 0)
        return;
    if (*(int *)(D_0013E633 + 0x10AD + 0x1DFC) != 0xF)
        return;
    fl[0x2F] = 1;
}
