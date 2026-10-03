/* NON_MATCHING func_L00_001FE940 -- src/overlays/shared/drawquad_001FD1D8.c
 * Best so far: BYTES 6/132 (95.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds a free slot (life<=0) in the 16-entry D_L00_0016EB40 table, fills it (vec copy via stack, float, life 0x
 *   Best p4/p2: 6/132 bytes differ; only the order of `sq` vs `swc1 $f12,0x1C` stores (retail sq first); source re
 */
typedef int u128 __attribute__((mode(TI)));
typedef struct { u128 v; short life; char pad[2]; short b; char pad2[2]; float f; float g; } Q;
extern Q D_slots[16] __asm__("D_L00_0016EB40");
extern float func_00214158(void);

// Finds a free slot in the 16-entry table, fills it and returns its index (or -1).
int func_L00_001FE940(void *src_, float x, int life, int b) {
    u128 tmp[1];
    u128 *t = tmp;
    int i;
    Q *q = D_slots;
    *t = *(u128 *)src_;
    for (i = 0; i < 16; i++, q++) {
        if (q->life <= 0) {
            q->g = x;
            q->life = 24;
            q->b = life;
            q->v = *t;
            q->f = func_00214158();
            return i;
        }
    }
    return -1;
}
