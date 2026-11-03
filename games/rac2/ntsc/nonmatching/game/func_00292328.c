/*
 * Measured plain-C -G8 near miss, 2026-10-24.
 * tools/check.py reported 0/28 placements in USA, PAL, and US2; every
 * placement was 160 bytes against the 152-byte target. The generated code
 * preserves the D_001CDB80 table pointer across func_002933D0 and saves/restores it,
 * adding eight bytes. The target reloads the table after the call.
 */
extern int D_001A7298 __attribute__((section(".data")));
extern void *D_001A7294 __attribute__((section(".data")));
extern unsigned long long D_001A72A0[] __attribute__((section(".data")));
extern unsigned long long D_L00_001C4DC0[] __attribute__((section(".data")));
extern unsigned long long D_L00_001A91D0 __attribute__((section(".sdata")));
extern char *D_001CDB80[];
extern void func_002933D0(void *, int, void *);

void func_00292328(void *param)
{
    int count = D_001A7298;

    if (count > 0) {
        unsigned long long *source = D_001A72A0;
        unsigned long long *destination = D_L00_001C4DC0;
        unsigned long long constant1 = 0x0000FFA0000000E0ULL;
        unsigned long long constant2 = 0x0040000400004000ULL;

        do {
            destination[0] = *source;
            destination[1] = constant1;
            destination[2] = constant2;
            source++;
            destination += 3;
        } while (--count != 0);
    }
    func_002933D0(D_001A7294, 0, &D_L00_001A91D0);
    D_001CDB80[0][9] = D_001CDB80[0][8];
}
