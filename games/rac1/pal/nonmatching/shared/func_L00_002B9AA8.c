/* NON_MATCHING func_L00_002B9AA8 -- src/overlays/shared/vendor_002B33E8.c
 * Best so far: SIZE ours 52 / retail 48, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
void func_L00_002B9AA8(int unused, int value, int index, int *table)
{
    int *entry;
    for (index++; index < 15; index++) {
        entry = &table[index + 1];
        if (*entry == 0) {
            *entry = value;
            break;
        }
    }
}
