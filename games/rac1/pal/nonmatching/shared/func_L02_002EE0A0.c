/* NON_MATCHING func_L02_002EE0A0 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: COMPILE failed, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks a moby list (func_L00_0025A208/25A2F0); for entries with flag 0x31 set and equal 0xA6 type as arg, if da
 *   Best candidate p4.c: 22/228 bytes differ; same bytes from p4/p5/p6/p7 (m local, m outside loop, for-loop, list
 *   Difference: register allocation of the list[0] load (retail test in $v1, m in $a0; ours $v0/$v1) and delay-slo
 */
extern void func_L00_00264690(void *, int, float, float);

/* for each moby of the same type as arg, fires an effect on its sub-entries */
void func_L02_002EE0A0(unsigned char *arg) {
    int list[4];
    func_L00_0025A208(list, arg[0x21], 0, 0);
    while (list[0] != 0) {
        unsigned char *m = (unsigned char *)list[0];
        if (m[0x31] != 0 && *(short *)(m + 0xA6) == *(short *)(arg + 0xA6)) {
            unsigned char *d = *(unsigned char **)(m + 0x78);
            int v = d[0xE0];
            if (v > 0x80) {
                unsigned char *p = d + 0x80;
                int i;
                for (i = 0; i < 6; i++) {
                    func_L00_00264690(p, *(int *)(d + 0xE0), 1.2f, 0.57f);
                    p += 0x10;
                }
            }
        }
        func_L00_0025A2F0(list, list[0], 0, 0);
    }
}
