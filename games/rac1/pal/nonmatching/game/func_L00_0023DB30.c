/* Veldin level code: vendor stock-level table setup (retail func_L00_0023DB30, 0x23C bytes).
 * With the level number below 19 it fills the id list at D_0015EED0 from the table D_L00_001C4198
 * (ids in the low six bits, 0xFF ends the list), dropping ids the table lacks; otherwise it fills two
 * flag arrays and the 8-word record at D_0014171B + 0x885. Ends with func_L00_0023DD70 on the level. */
extern unsigned char D_0013D5CA[];
extern char D_0013D5EB[];
extern int D_L00_001C4198[];
extern unsigned char D_0015EED0[] MACRO_ADDR;
void func_L00_002618D8(int, int);
void func_L00_0023DD70(int);

void func_L00_0023DB30(void) {
    int lvl = D_0015EE84;
    int i;

    if (lvl >= 0x13) {
        char *b7 = D_0013D50F + 0xB9;
        char *b6 = D_0013D5EB + 5;
        int *s = (int *)((char *)D_0014171B_i + 0x885);

        for (i = 0; i < 0x25; i++) {
            b7[i] = 1;
            b6[i] = 1;
        }
        s[0] = 0xF;
        s[1] = 0xC;
        s[2] = 0xD;
        s[3] = 0xB;
        s[4] = 0x11;
        s[5] = 0xA;
        s[6] = 0x10;
        s[7] = 0x13;
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
    } else {
        int *t = D_L00_001C4198;

        if (D_0013D5CA[8] == 0) {
            func_L00_002618D8(0xA, 1);
            D_0015EED0[0] = 0x4A;
            *(int *)((char *)D_0014171B_i + 0x45) = 0xA;
        }
        i = 0;
        while (i < 12) {
            int v = D_0015EED0[i];
            int v4;
            int k;

            if (v == 0xFF) {
                i++;
                continue;
            }
            v4 = v & 0x3F;
            if (v4 == 0xA) {
                i++;
                continue;
            }
            if (t[0] == v4) {
                k = 0;
            } else {
                for (k = 1; k < 0x13; k++) {
                    if (t[k] == v4)
                        break;
                }
            }
            if (k == 0x13 || v4 == 0)
                D_0015EED0[i] = 0xFF;
            i++;
        }
        func_L00_0023DD70(D_0015EE84);
    }
}
