/* NON_MATCHING func_L00_0023DB30 -- src/overlays/shared/hud_00235960.c
 * Best so far: BYTES 36/572 (93.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0023DB30: vendor stock init. If D_0015EE84 >= 19 it fills two 0x25-byte flag arrays with 1, sets seve
 *   Best candidate p7.c: schedule identical to retail (source order bytes, then ints 4..0x18, then the 0x1C store 
 *   Would unblock: making the int-constant pseudos rank before the byte-constant pseudos without changing the sche
 */
extern unsigned char D_0015EED0[] MACRO_ADDR;
extern int D_L00_001C4198[];
extern void func_L00_0023DD70(int idx);
extern unsigned char D_0013D5EB[] NOT_SDA;
extern unsigned char D_0013D5CA[] NOT_SDA;
extern unsigned char D_0014171B[] NOT_SDA;
extern void func_L00_002618D8(int, int);

/* Initialise the vendor stock list: fixed defaults once the level is at least 19, else prune unknown entries. */
void func_L00_0023DB30(void) {
    int i, j, v, k;
    unsigned char c;
    if (D_0015EE84 >= 0x13) {
        for (k = 0; k < 0x25; k++) {
            (D_0013D50F + 0xB9)[k] = 1;
            (D_0013D5EB + 5)[k] = 1;
        }
        *(int *)(D_0014171B + 0x885) = 0xF;
        D_0015EED0[0] = 0x4A;
        D_0015EED0[1] = 0x50;
        D_0015EED0[2] = 0x4F;
        D_0015EED0[3] = 0x54;
        D_0015EED0[4] = 0x51;
        D_0015EED0[5] = 0x4E;
        D_0015EED0[6] = 0x4B;
        D_0015EED0[7] = 0x52;
        D_0015EED0[8] = 0x4D;
        D_0015EED0[9] = 0x59;
        D_0015EED0[10] = 0x58;
        D_0015EED0[11] = 0x53;
        *(int *)(D_0014171B + 0x889) = 0xC;
        *(int *)(D_0014171B + 0x88D) = 0xD;
        *(int *)(D_0014171B + 0x891) = 0xB;
        *(int *)(D_0014171B + 0x895) = 0x11;
        *(int *)(D_0014171B + 0x899) = 0xA;
        *(int *)(D_0014171B + 0x89D) = 0x10;
        *(int *)(D_0014171B + 0x8A1) = 0x13;
    } else {
        if (D_0013D5CA[8] == 0) {
            func_L00_002618D8(0xA, 1);
            D_0015EED0[0] = 0x4A;
            *(int *)(D_0014171B + 0x45) = 0xA;
        }
        for (i = 0; i < 12; i++) {
            c = D_0015EED0[i];
            if (c == 0xFF) continue;
            v = c & 0x3F;
            if (v == 10) continue;
            for (j = 0; j < 19; j++) {
                if (D_L00_001C4198[j] == v) break;
            }
            if (j == 19 || v == 0) {
                D_0015EED0[i] = 0xFF;
            }
        }
        func_L00_0023DD70(D_0015EE84);
    }
}
