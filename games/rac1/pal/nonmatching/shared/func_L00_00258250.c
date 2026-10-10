/* NON_MATCHING func_L00_00258250 -- src/overlays/shared/mobyproc_00251A78.c
 * Best so far: SIZE ours 584 / retail 596, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Picks one of up to 12 candidate ids from D_0015EED0 (byte & 0x3F) weighted by table entries (D_0013D50F+0xB9 f
 *   Best candidate p6.c (SIZE 580 vs retail 596; p4.c 572): the logic and most instructions match (bit test, three
 *   This looks like the "repeated lui for one symbol" per-function compiler flag wall (no cse related-value deriva
 */
extern int func_002140B0(int);
extern unsigned char D_0015EED0[] MACRO_ADDR;
struct Ent { char pad[0xE]; unsigned short v; char pad2[8]; };
extern struct Ent D_L00_001C43B0[];
extern char D_0014171B[];
extern char D_0013D50F[];

/* picks an index into a list of up to 12 weighted entries and stores it; returns 1 or 2 */
int func_L00_00258250(char *a, int *out) {
    int n10 = 0, n11 = 0, n9 = 0;
    int k, r7, res;
    if (*out <= 0 || (a != 0 && (*(int *)(*(int *)&D_0015EE84 * 256 + ((short)*(unsigned short *)(a + 0xB2) >> 5) * 4 + D_0014171B + 0xAB75) >> (*(unsigned short *)(a + 0xB2) & 0x1F) & 1))) {
        int i;
        for (i = 0; i < 12 && D_0015EED0[i] != 0xFF; i++) {
            int e = D_0015EED0[i] & 0x3F;
            if (*(unsigned char *)(D_0013D50F + 0xB9 + e)) {
                int h = D_L00_001C43B0[e].v;
                int w = *(int *)(D_0013D50F + 0x21 + e * 4);
                n11++;
                if (h) n9++;
                n10 += w < h;
            }
        }
        k = -1;
        if (n10 != 0) {
            r7 = func_002140B0(n10);
            while (r7 >= 0) {
                int e;
                k++;
                e = D_0015EED0[k] & 0x3F;
                if (*(unsigned char *)(D_0013D50F + 0xB9 + e)) {
                    int h = D_L00_001C43B0[e].v;
                    int w = *(int *)(D_0013D50F + 0x21 + e * 4);
                    r7 -= w < h;
                }
            }
            res = D_0015EED0[k] & 0x3F;
        } else {
            res = 10;
            if (n11 != 0) {
                r7 = func_002140B0(n9);
                while (r7 >= 0) {
                    int e;
                    k++;
                    e = D_0015EED0[k] & 0x3F;
                    if (*(unsigned char *)(D_0013D50F + 0xB9 + e)) {
                        int h = D_L00_001C43B0[e].v;
                        if (h) r7--;
                    }
                }
                res = D_0015EED0[k] & 0x3F;
            }
        }
        *out = res;
    }
    res = 2;
    if (func_002140B0(5)) res = 1;
    return res;
}
