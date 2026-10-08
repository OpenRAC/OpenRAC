/* NON_MATCHING func_L01_00286530 -- src/overlays/shared/partupd_00280428.c
 * Best so far: BYTES 8/316 (97.5% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   PartType41Spawn: allocates particle type 0x29, copies pos (qcopy), fills fields from args; p[3]/p[9] colour pi
 *   Best p3.c (18 bytes differ): only swap of saved regs: retail arg1->$fp, short b->$s7; ours b->$fp, arg1->$s7 (
 *   Budget spent; unblock would need a way to flip allocation priority of arg1 vs b.
 *   Round q28: p9 (q local inside the if) gets 8/316 bytes: only arg1/short b saved regs (fp/s7) swapped in 4 insn
 */
extern unsigned char *func_00218928(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern unsigned char *D_L01_001B28A4;

/* spawn a type-0x29 particle at a position */
unsigned char *func_L01_00286530(float scale, void *pos, int arg1, int idx, int arg3, unsigned char a, short b, unsigned char c) {
    unsigned char *p = func_00218928(0x29);
    if (p != 0) {
        int *q = (int *)(p + 0x20);
        int bb = b;
        qcopy(p + 0x10, pos);
        *(int *)(p + 4) = arg3;
        if (c == 0xFF) {
            p[9] = (a >> 5) * 16 + func_001FA898_r(4.0f);
            if (a & 1) {
                p[3] = 0x48;
            } else {
                p[3] = 0x44;
            }
        } else {
            p[9] = a;
            p[3] = c;
        }
        p[1] = 0;
        p[2] = D_L01_001B28A4[idx];
        *(float *)(p + 0xC) = scale * 210000.0f;
        *(short *)(p + 0xA) = bb;
        p[8] = 0xA0;
        *q = arg1;
    }
    return p;
}
