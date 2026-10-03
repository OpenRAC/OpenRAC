/* NON_MATCHING func_L00_00205B50 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 32/196 (83.7% of the bytes match), checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   (daddu $s0,$v0,$0), taking q/r/s from $v0 and the end (+0x1590) from $s0; we allocate $s0 directly (one insn s
 *   Tried separate/typed address expressions, declaration order, end from D, &D[0x40] spellings.
 *   ## Round 1 (hand, from Lombyte PR #66's FUN_L00_00205538)
 *   Size exact. Index loop over a 0xB0-byte entry struct with b and c as running char pointers (b declared
 *   before c). Left: retail computes both b and c from the base register ($v0, lui+addiu then `addiu $s2,$v0,0x60`
 *   `addiu $s3,$v0,0x90`); ours derives c from b (`addiu $s3,$s2,0x30`) and keeps the base in $v1, so the prologue
 *   and the two register names differ. `int` instead of `char *` for b, c, and the pointer-only / int-only
 *   versions of the whole loop, did not change it.
 */
typedef struct {
    unsigned char a0;
    unsigned char active;
    char pad[0x3E];
    char m40[0x20];
    char m60[0x30];
    char m90[0x12];
    short type;
    char pad2[8];
    float scale;
} Ent205B50;
extern char D_L00_0017A780[];
extern int func_L00_00205728(int);
extern void func_0020D9D8(int, void *);
extern void func_001F9BC0(float *);

void func_L00_00205B50(void) {
    int i;
    char *b = D_L00_0017A780 + 0x60;
    char *c = D_L00_0017A780 + 0x90;
    for (i = 0; i < 31; i++) {
        Ent205B50 *e = &((Ent205B50 *)D_L00_0017A780)[i];
        if (e->active != 0) {
            int r = func_L00_00205728(e->type);
            if (r != 0) func_0020D9D8(r, e);
        }
        func_001F9BC0((float *)((Ent205B50 *)D_L00_0017A780)[i].m40);
        func_001F9BC0((float *)b); b += sizeof(Ent205B50);
        func_001F9BC0((float *)c); c += sizeof(Ent205B50);
        e->scale = 1.0f;
    }
}
