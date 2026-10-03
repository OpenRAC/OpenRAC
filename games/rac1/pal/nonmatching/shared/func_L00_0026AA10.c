/* NON_MATCHING func_L00_0026AA10 -- src/overlays/shared/partupd_0026A130.c
 * Best so far: BYTES 10/252 (96.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a type-5 particle (func_00218928(5)) at pos with packed RGB and a target; sets fields, stores a/func_00
 *   p1.c is 4 words off: saved-register assignment swaps pos ($s4 retail) and the third color byte ($s3 retail); p
 *   Unblock: something that raises pos's allocation priority over c2 (unknown); an allocator tie.
 */
extern void *func_00218928(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA888(int);
extern int D_L00_001B2414;

/* spawns a type-5 particle at pos with packed color and a target */
void func_L00_0026AA10(float a, float b, void *pos, unsigned char c0, unsigned char c1, unsigned char c2, int tgt) {
    char *p;
    float *q;
    if (tgt) {
        p = func_00218928(5);
        if (p) {
            qcopy(p + 0x10, pos);
            *(int *)(p + 4) = 0x7F000000 | (c2 << 16) | (c1 << 8) | c0;
            q = (float *)(p + 0x20);
            p[9] = func_001FA898_r(4.0f) - 0x60;
            p[3] = 0x48;
            *(float *)(p + 0xC) = b;
            p[1] = 0;
            p[8] = 0;
            p[2] = **(unsigned char **)&D_L00_001B2414;
            *(int *)(p + 0x20) = tgt;
            q[1] = a / func_001FA888(tgt);
        }
    }
}
