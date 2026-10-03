/* NON_MATCHING func_L13_0030C320 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 228 / retail 232, checked 2026-10-02.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-03): match its declarations to the file's first.
 * What the last attempts found:
 *   Counts the mobys in list D_L13_001ABE40[idx] (stop at entry with bit 15 set) that are alive, unbusy (func_L00_
 *   Logic matches (p0.c: 228 bytes vs 232, structure identical). Wall: retail reads D_L13_00160058 twice in one fu
 *   try_func emits both `.extern D,2` and `.extern D,4` for aliased decls and the assembler then makes BOTH access
 */
#include "common.h"
extern short *D_L13_001ABE40[];
extern short D_L13_00160058;
extern char *func_L00_002DCD40(char *);

/* Counts the mobys of a list that are alive and idle, optionally ignoring those in a given state. */
int func_L13_0030C320(int idx, int arg) {
    short *p = D_L13_001ABE40[idx];
    int count = 0;
    if (p == 0) return 0;
    do {
        int id = *p & 0x7FFF;
        char *m = (char *)(id << 8) + D_L13_00160058_m;
        if (m[0x20] >= 0) {
            char *f = func_L00_002DCD40(m);
            if (f == 0 || *(short *)(f + 0x68) < 4) {
                if (arg == -1 || *(unsigned char *)((char *)(id << 8) + *(int *)&D_L13_00160058 + 0x20) != arg) {
                    count++;
                }
            }
        }
    } while (*p++ >= 0);
    return count;
}
