/* Calls each callback in the table at D_L00_001690F0, in order, for as many entries as the
 * count D_L00_0015F04C says (re-read after each call), then sets the count to zero. */
extern int D_L00_0015F04C MACRO_ADDR;
extern void (*D_L00_001690F0[])(void);

void func_L00_001EB508(void) {
    int i;
    for (i = 0; i < D_L00_0015F04C; i++) {
        D_L00_001690F0[i]();
    }
    D_L00_0015F04C = 0;
}
