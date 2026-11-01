#include "common.h"
extern int D_L11_001CAE40;
extern int D_L11_001B46D4[] NOT_SDA __attribute__((sda));
extern int D_L11_001B30C8[] NOT_SDA __attribute__((sda));
extern void func_002A7750(void *, int, int);
extern void func_L00_002E6CF0(void *, void *);
extern void func_L11_0045B6C8(char *);
void func_L11_0045B5C8(char *p)
{
    char *q = *(char **)(p + 0x68);
    unsigned short c;
    int v;
    int old, current;

    if (D_L11_001CAE40 != 0)
        return;
    switch (*(unsigned char *)(p + 0x20)) {
    case 0:
        *(unsigned char *)(p + 0x30) = 0xFF;
        func_002A7750(p, 1, -1);
        *(int *)(q + 0x0) = 0x1000000;
        *(int *)(q + 0x8) = 1;
        *(int *)(q + 0x4) = 1;
        break;
    case 1:
        if (*(unsigned short *)(p + 0x96) >= 0x2D) {
            *(unsigned short *)(p + 0x96) = 0;
            *(int *)(q + 0x4) = -*(int *)(q + 0x4);
        }
        v = *(int *)(q + 0x8) + *(int *)(q + 0x4);
        old = D_L11_001B46D4[0];
        current = D_L11_001B30C8[0];
        *(int *)(q + 0x0) = v << 24;
        *(int *)(q + 0x8) = v;
        if (old != current) {
            D_L11_001B46D4[0] = current;
            func_L00_002E6CF0(func_L11_0045B6C8, p);
        }
        break;
    }
    c = *(unsigned short *)(p + 0x96);
    if (c <= 0xFFFE)
        *(unsigned short *)(p + 0x96) = c + 1;
}
