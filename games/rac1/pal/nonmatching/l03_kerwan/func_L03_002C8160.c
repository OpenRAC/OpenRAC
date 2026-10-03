/* NON_MATCHING func_L03_002C8160 -- src/overlays/l03_kerwan/vendor_00293720.c
 * Best so far: SIZE ours 136 / retail 128, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_L00_0025A208(void **, int, int, int);
extern int func_L03_002C6F40(unsigned char *);
extern void func_L00_0025A2F0(void **, void *, int, int);

int func_L03_002C8160(int id) {
    unsigned char *item;
    if (id == 0xFF) return 0;
    func_L00_0025A208((void **)&item, id, 0, 0);
    while (item != 0) {
        if (func_L03_002C6F40(item)) return 1;
        if (func_L03_002C6F40(item)) return 1;
        func_L00_0025A2F0((void **)&item, item, 0, 0);
    }
    return 0;
}
