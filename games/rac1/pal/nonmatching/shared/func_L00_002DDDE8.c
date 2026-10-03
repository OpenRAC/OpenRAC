/* NON_MATCHING func_L00_002DDDE8 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: BYTES 15/184 (91.8% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1 (hand, from Lombyte PR #66's FUN_L00_002dc938)
 *   Everything matches except the zero quad store: retail `sq $zero,0($v1)` after `addiu $v1,$s0,0x40`;
 *   `(Q){{0.0f,...}}` gives `por $v1,$zero,$zero` + `sq`. Lombyte writes it as inline asm (`sq $0,0(%0)`),
 *   which this repo forbids, so this is the known `sq $zero` wall (docs/LEVERS.md).
 */
typedef struct { int v[16]; } __attribute__((aligned(16))) Mtx2DDDE8;
typedef struct { float v[4]; } __attribute__((aligned(16))) Q2DDDE8;
extern void func_L00_002DD3D8(char *, char *);

void func_L00_002DDDE8(char *p, char *q, int flag) {
    Mtx2DDDE8 m;
    void (*fn)(char *);
    if (flag) func_L00_002DD3D8(p, q);
    *(int *)(p + 0x98) = *(int *)(q + 0x84);
    *(Q2DDDE8 *)(p + 0x40) = (Q2DDDE8){{0.0f, 0.0f, 0.0f, 0.0f}};
    func_001FA218(&m, p + 0x40);
    func_001FA480(p + 0xC0, &m);
    func_0020EEE8(p);
    *(unsigned short *)(p + 0x34) &= ~4;
    *(int *)(p + 0x98) = *(int *)(q + 0x84);
    *(float *)(p + 0x2C) = *(float *)(*(char **)(p + 0x24) + 0x24);
    *(short *)(q + 0x68) = 0;
    fn = *(void (**)(char *))(*(char **)(*(char **)(p + 0x24) + 0x2C) + 0x14);
    if (fn) fn(p); else func_0020D678(p);
}
