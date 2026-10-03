/* NON_MATCHING func_L15_00248E58 -- src/overlays/l15_quartu/mobyutil_00248E58.c
 * Best so far: SIZE ours 356 / retail 352, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moves a moby by a delta vector (BD8 add), then probes up to 6 steps: copy pos to a work vec, raise z by dz, co
 *   Best p8.c/p9.c (57 bytes differ, same structure, same spill of delta to 0x20($sp), same frame): only saved-reg
 *   Needs a wording that makes pos (copy of moby+0x10) the lowest-priority pseudo; retail took moby+0x10 in $a1 th
 */
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern void func_001F9BF0(void *, void *, void *);
extern char D_L15_001744F0[];

/* move a moby by a delta, probing six steps and pushing back on hits */
int func_L15_00248E58(char *moby, void *delta, int flags, float dz, float range) {
    float saved[4];
    float work[4];
    char *pos;
    char *s = D_L15_001744F0 - 0x30;
    int hit = 0;
    int i = 0;
    qcopy(saved, moby + 0x10);
    pos = moby + 0x10;
    func_001F9BD8(pos, pos, delta);
    flags = (flags << 1) & 0x20;
    do {
        qcopy(work, pos);
        work[2] += dz;
        if (func_L00_001F10E0(range, work, flags | 4, moby) == 0) {
            break;
        }
        if (func_L00_001FF860(*(float *)(s + 0x48), func_001F9CE8(s + 0x40)) > 0.52359879f) {
            qcopy(pos, D_L15_001744F0);
            *(float *)(moby + 0x18) -= dz;
            hit |= 1;
        }
        i++;
    } while (i < 6);
    func_001F9BF0(delta, pos, saved);
    return hit;
}
