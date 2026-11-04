#include "include_asm.h"

extern int D_L20_001B20CC __attribute__((sda));
/* The five-entry state table is reached through $gp. */
extern int D_L20_001B20D8[5] __attribute__((sda));
extern int func_0032F680(int);

void func_L20_00442CD8(char *p) {
    char *s = *(char **)(p + 0x68);
    int mask, i, r, b;

    b = D_L20_001B20CC;
    *(float *)(s + 0x1838) = 50.0f;
    if (b) {
        *(int *)(s + 0x1834) = D_L20_001B20D8[b - 1];
        if (*(int *)(s + 0x1834) == 10)
            *(float *)(s + 0x1838) = 30.0f;
    } else if (*(int *)(s + 0x183C) == 0x12D3) {
        *(int *)(s + 0x1834) = 11;
    } else {
        mask = 0;
        switch (*(int *)(s + 0x182C)) {
        case 0:
            mask = 1;
            break;
        case 1:
            mask = 6;
            break;
        case 2:
            mask = 15;
            break;
        }
        if (*(int *)(s + 0x184C) & mask)
            mask = *(int *)(s + 0x184C) & mask;
        r = func_0032F680(4);
        for (i = 0; i < 4; i++) {
            int bit = 1 << ((r + i) % 4);

            if (bit & mask) {
                *(int *)(s + 0x184C) &= ~bit;
                switch (bit) {
                case 1:
                    *(int *)(s + 0x1834) = 7;
                    break;
                case 2:
                    *(int *)(s + 0x1834) = 10;
                    break;
                case 4:
                    *(int *)(s + 0x1834) = 9;
                    break;
                case 8:
                    *(int *)(s + 0x1834) = 8;
                    break;
                }
                break;
            }
        }
    }
}
