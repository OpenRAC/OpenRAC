/* NON_MATCHING func_L13_0030B080 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 696 / retail 704, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Loops over a 16-slot table (16-byte entries at moby data +0x210, float at +0xC); for each active slot
 *   builds two random direction vectors (sin/cos * random range), scales them, and calls func_00219780 to spawn a 
 *   Best candidate p5.c: same size (704), loop shape and stack layout right (for i++, j = i+1 local, off = i*16 lo
 *   Remaining difference: `p = s + off` (addu) is hoisted by the scheduler into the delay slot of the second func_
 *   call, so `mov.s $f21,$f0` moves up and the rest of the body shifts by one slot; retail keeps the addu just bef
 *   func_001F9C08 and in-place on $16. Would need a source form that keeps that addu late (none of 4 tried did).
 */
extern float func_002140F8(float, float);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9C30(void *, void *, float);
extern void func_001F9C08(void *, void *, void *, float);
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern short D_L13_00161F40;
extern short D_L13_00161F44;
extern short D_L13_00161F48;
extern short D_L13_00161F4C;
extern short D_L13_00161F50;
extern short D_L13_00161F54;
extern short D_L13_00161F58;
extern short D_L13_00161F5C;
extern short D_L13_00161F60;
extern short D_L13_00161F64;
extern short D_L13_00161F68;
extern short D_L13_00161F6C;
extern short D_L13_00161F70;
extern short D_L13_00161F74;
extern short D_L13_00161F78;
extern short D_L13_00161F7C;

#define G(x) (*(float *)&D_L13_00161F##x)
#define GI(x) (*(int *)&D_L13_00161F##x)

/* spawns particles for each active entry of the moby's 16-slot table */
void func_L13_0030B080(char *m) {
    char *s = *(char **)(m + 0x78);
    int i;
    float v0[4];
    float v1[4];
    float v2[4];
    for (i = 0; i < 15; i++) {
        int off = i * 16;
        float a, b, c;
        int r1, r2, r3;
        char *p;
        if (*(float *)(s + off + 0x21C) < 0.99f && *(float *)(s + (i + 1) * 16 + 0x21C) < 0.99f) {
            continue;
        }
        if (G(78) < func_002140F8(0.0f, 1.0f)) {
            continue;
        }
        a = func_00214158();
        off += 0x210;
        b = func_00214158();
        v1[0] = func_001F9F90(a) * func_002140F8(G(68), G(6C));
        v1[1] = func_001F9FA8(a) * func_002140F8(G(68), G(6C));
        v1[2] = 0;
        v2[0] = func_001F9F90(b) * func_002140F8(G(68), G(6C));
        v2[1] = func_001F9FA8(b) * func_002140F8(G(68), G(6C));
        v2[2] = 0;
        v1[2] = func_002140F8(G(70), G(74));
        v2[2] = func_002140F8(G(70), G(74));
        r1 = func_001FA898_r(func_001F9878(func_002140F8(G(50), G(54))));
        r2 = func_001FA898_r(func_001F9878(func_002140F8(G(58), G(5C))));
        r3 = func_001FA898_r(func_001F9878(func_002140F8(G(60), G(64))));
        func_001F9C30(v1, v1, 1.0f / (float)r1);
        v1[3] = G(48);
        func_001F9C30(v2, v2, 1.0f / (float)r3);
        v2[3] = G(4C);
        p = s + off;
        func_001F9C08(v0, p, p, func_002140F8(0.0f, 1.0f));
        func_00219780(v0, v1, v2, GI(40), GI(44), r1, r2, r3, GI(7C));
    }
}
