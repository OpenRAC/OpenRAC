extern int D_0015EE84 MACRO_ADDR;
extern int D_L00_001C47B8[];
extern int D_L00_001792B8[];

/* Finds key in the list of the current state D_0015EE84 (only 0..18 has one;
   other values give 0). The list runs over D_L00_001792B8 from index
   D_L00_001C47B8[s] up to, not including, D_L00_001C47B8[s + 1]. Returns the
   index where key is first found, or -1. The second argument is unused. */
int func_L00_00267618(int key, int unused) {
    int s = D_0015EE84;
    int lo;
    int hi;
    int i;

    if ((unsigned int)s >= 0x13) {
        return 0;
    }
    lo = D_L00_001C47B8[s];
    hi = D_L00_001C47B8[s + 1];
    for (i = lo; i < hi; i++) {
        if (D_L00_001792B8[i] == key) {
            return i;
        }
    }
    return -1;
}
