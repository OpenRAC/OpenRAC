/* NON_MATCHING func_L08_00280BC8 -- src/overlays/l08_batalia/partupd_0027ACC8.c
 * Best so far: SIZE ours 236 / retail 232, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Void function spawning class 0x4D particle: p4.c is nearly there (const 210000.0f, short t = func_001F9850(c))
 *   Left: retail does sh $v0,0xA; sll $v0; sra $a0 in place, with the qcopy lq in the jal delay slot; ours does sh
 *   Unblock: a form making the short store from $v0 and the sign extension in place.
 */
extern unsigned char *func_00218928(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9850(int);
extern float func_001FA888(int);
extern unsigned char *D_L08_001B2CB4;
extern void qcopy(void *, void *);
// Spawns a class 0x4D particle moby at a position.
void func_L08_00280BC8(char *a, char *b, int c, float f) {
    unsigned char *m = func_00218928(0x4D);
    char *q;
    short t;
    if (m != 0) {
        qcopy(m + 0x10, a);
        q = (char *)m + 0x20;
        *(int *)(m + 4) = 0x50504040;
        m[9] = func_001FA898_r(2.0f) + 0x40;
        m[3] = 0x48;
        m[1] = 0;
        m[2] = *D_L08_001B2CB4;
        m[8] = 0xA0;
        *(float *)(m + 0xC) = f * 210000.0f;
        t = func_001F9850(c);
        *(short *)(m + 0xA) = t;
        qcopy(m + 0x20, b);
        *(float *)(q + 0xC) = func_001FA888(t);
    }
}
