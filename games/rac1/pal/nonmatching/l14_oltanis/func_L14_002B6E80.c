/* NON_MATCHING func_L14_002B6E80 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: BYTES 15/100 (85.0% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Selects a path table entry and initializes the last segment index, float, and state vector.
 *   p5/p6/p7 compile identically at BYTES 15/100: data and entry use swapped a1/a2, early call-argument setup and 
 *   Stopped at three distinct wordings; data/entry allocator choice remains despite branch and table-address forms
 */
extern char *D_L14_001B0F30_a[] __asm__("D_L14_001B0F30") MACRO_ADDR;
extern void func_001F9BC0(void *);

void func_L14_002B6E80(char *moby)
{
    char *data = *(char **)(moby + 0x78);
    int index;
    char *entry;
    int count;
    index = (*(int *)(data + 0x10C) == 0) ? *(int *)(data + 0xD0) : *(int *)(data + 0x150);
    entry = D_L14_001B0F30_a[index];
    count = *(int *)entry - 2;
    *(int *)(data + 0xE8) = count;
    *(float *)(data + 0xEC) = *(float *)(entry + count * 16 + 0x1C);
    *(int *)(data + 0xE4) = 0;
    func_001F9BC0(data + 0xF0);
}
