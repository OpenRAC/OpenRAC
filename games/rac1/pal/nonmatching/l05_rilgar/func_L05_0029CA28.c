/* NON_MATCHING func_L05_0029CA28 -- src/overlays/l05_rilgar/partupd_0029CA28.c
 * Best so far: BYTES 12/292 (95.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Allocates a 0x30-byte particle record (func_00218928), fills it (qcopy of pos and vel, scale*210000, byte fiel
 *   Best p9 (12 bytes off): everything matches except the order of the five trailing stores (retail: swc1 0x10, sh
 *   Source permutations of the trailing stores (p4, p6-p10) trade the register swap against store order; would nee
 */
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_002140B0(int);
extern float func_001FA888(int);
extern unsigned char *D_L05_001B2940;

// allocate and fill a particle record
void *func_L05_0029CA28(void *pos, void *vel, int a2, int a3, int flags, float scale)
{
    char *p = func_00218928(0x30);
    if (p != 0) {
        char *q;
        float two;
        int mask;
        qcopy(p + 0x10, pos);
        mask = flags & 0xFFFFFF;
        two = 2.0f;
        q = p + 0x20;
        *(int *)(p + 4) = mask;
        p[9] = func_001FA898_r(two) + 0x10;
        p[3] = 0x44;
        p[1] = 0;
        p[2] = *D_L05_001B2940;
        *(float *)(p + 0xC) = scale * 210000.0f;
        p[8] = func_002140B0(0x100);
        qcopy(q, vel);
        *(float *)(q + 0x10) = two / func_001FA888(a2);
        *(int *)(q + 0x14) = mask;
        *(int *)(q + 0x18) = flags;
        *(short *)(q + 0x1C) = a3;
        *(short *)(p + 0xA) = a2;
    }
    return p;
}
