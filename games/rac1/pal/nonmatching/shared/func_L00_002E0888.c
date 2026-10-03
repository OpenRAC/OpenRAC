/* NON_MATCHING func_L00_002E0888 -- src/overlays/shared/vendor_002D9438.c
 * Best so far: SIZE ours 212 / retail 208, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks a moby list (func_L00_0025A208/25A2F0 iterator, list in a stack slot), and for each live moby (byte 0x31
 *   Difference: SIZE 212 vs 208. Retail allocates the 0x78/4 pointer chain into $5 and fills the inner bne delay s
 *   Likely allocator/scheduler tie not reachable by rewording.
 */
extern void func_L00_0025A208(int *, int, int, int);
extern int func_L00_0025A2F0(int *, int, int, int);
extern void func_L00_00264690(void *, int, float, float);
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;

/* applies a pulse effect to every matching moby in the list */
void func_L00_002E0888(unsigned char *m) {
    unsigned char *list;
    float scale = 1.2f;
    if (D_0015EE84_m == 0 || D_0015EE84_m == 0x12) scale = 1.0f;
    func_L00_0025A208((int *)&list, m[0x21], 0, 0);
    if (list != 0) {
        do {
            if (list[0x31] != 0) {
                if (*(short *)(m + 0xA6) == *(short *)(list + 0xA6)) {
                    func_L00_00264690(list + 0x10, *(int *)(*(int *)(list + 0x78) + 4), scale, 0.5f);
                }
            }
            func_L00_0025A2F0((int *)&list, (int)list, 0, 0);
        } while (list != 0);
    }
}
