/* NON_MATCHING func_L00_00207310 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 2/264 (99.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (expression alias): rewrite that in plain C first.
 * What the last attempts found:
 *   func_L00_00207310: periodic help-message check. Trigger 0x4E2E/0x78 if heli pack flag and func_L00_0020DC00();
 *   Best candidate p8.c: body and loop match, size 256 vs retail 264. Retail forms the lo half of D_0014171B+0x34D
 *   Offsets 0x3C0/0x280 are table index 0x78/0x50 times 8 (the message ids passed to func_L00_00203F20). Folded co
 */
extern unsigned short D_msgtab[] __asm__("D_0014171B+0x34D") NOT_SDA;
extern unsigned short D_msgaux __asm__("D_0014171B+0x2D5") NOT_SDA;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern unsigned char D_0013D5CA[];
extern unsigned char D_0013D50F[];
extern int func_L00_0020DC00(void);
extern int func_L00_00203F20(int a, int b);

// Periodic check: triggers one of two help messages depending on the tick count and slot usage.
void func_L00_00207310(void) {
    if (D_msgtab[0x78 * 4] == 0 && D_0013D5CA[2] != 0) {
        if (func_L00_0020DC00()) {
            func_L00_00203F20(0x4E2E, 0x78);
        }
    }
    if (D_L00_0015F6B0 > 0 && (D_L00_0015F6B0 & 0x7F) == 0) {
        if (D_msgtab[0x50 * 4] == 0 && D_msgaux == 0) {
            char *p = (char *)D_L00_00179BC0;
            unsigned char *f = D_0013D50F + 0xB9;
            int i, n = 0;
            for (i = 0; i < 0x25; i++) {
                if (*(int *)(p + i * 0x4C + 8) == 0 && i != 8 && i != 0x18) {
                    if (i[f]) n++;
                }
            }
            if (n >= 9) {
                func_L00_00203F20(0x4E26, 0x50);
            }
        }
    }
}
