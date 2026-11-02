#include "common.h"
extern void func_37BB30(int, unsigned long long);
extern void func_302298(int, int, int, int, int, int);
extern void func_2F0670(int, int, unsigned long long, char *, int);
extern int func_00115DA8(char *, char *, ...);
extern int D_L30_001B1B50 __attribute__((sda));
extern int D_L30_001B1B60 __attribute__((sda));
extern int D_L30_001B1B68 __attribute__((sda));
extern int D_L30_001B1B6C __attribute__((sda));
extern int D_L30_001B1B64 __attribute__((sda));
extern int D_L30_001B1B7C __attribute__((sda));
extern int D_L30_001B1B54, D_L30_001B1B58;
extern int D_L30_001B1B74;
extern char D_L30_001B1B80[];
extern char D_L30_001B1BB0[];
extern char D_L30_001B1BB8[];
extern char D_L30_001B1BC0[];
extern char D_L30_001B1BC8[];
extern void *D_L30_00260E30[];
extern char *D_L30_00260EB0[];
extern unsigned char D_L30_001B3D30[] __attribute__((sda)) NOT_SDA;
void func_L30_004135A0(char *obj) {
    char text[256];
    int i = 0, count = 0;
    int *st = *(int **)(obj + 0x68);
    func_37BB30(0x42, 0x8000000044ULL);
    func_302298(D_L30_001B1B50, D_L30_001B1B54, D_L30_001B1B58, D_L30_001B1B54 + D_L30_001B1B60 * (st[1] < 17 ? st[1] : 16), D_L30_001B1B7C, 0);
    func_2F0670(2, 8, D_L30_001B1B74, D_L30_001B1B80, -1);
    for (; i < 32 - st[2] && count < st[1] && count < 16; i++) {
        if (!D_L30_00260E30[i + st[2]]) continue;
        count++;
        if (D_L30_00260EB0[i]) {
            func_00115DA8(text, D_L30_001B1BB0, D_L30_00260EB0[i + st[2]]);
            func_2F0670(D_L30_001B1B68, D_L30_001B1B6C + i * D_L30_001B1B60, D_L30_001B1B74, text, -1);
        }
        if (st[0] == i + st[2]) {
            func_00115DA8(text, D_L30_001B1BB8);
            func_2F0670(D_L30_001B1B68 - 16, D_L30_001B1B6C + i * D_L30_001B1B60, 0x80008000ULL, text, -1);
        }
        switch (D_L30_001B3D30[i + st[2]]) {
        case 0:
            func_00115DA8(text, D_L30_001B1BC0, *(int *)D_L30_00260E30[i + st[2]]);
            func_2F0670(D_L30_001B1B64, D_L30_001B1B6C + i * D_L30_001B1B60, 0x80008080ULL, text, -1);
            break;
        case 1:
            func_00115DA8(text, D_L30_001B1BC8, (double)*(float *)D_L30_00260E30[i + st[2]]);
            func_2F0670(D_L30_001B1B64, D_L30_001B1B6C + i * D_L30_001B1B60, 0x80008080ULL, text, -1);
            break;
        case 2:
            func_00115DA8(text, D_L30_001B1BC0, *(unsigned char *)D_L30_00260E30[i + st[2]]);
            func_2F0670(D_L30_001B1B64, D_L30_001B1B6C + i * D_L30_001B1B60, 0x80008080ULL, text, -1);
            break;
        case 3:
            func_00115DA8(text, D_L30_001B1BC0, *(short *)D_L30_00260E30[i + st[2]]);
            func_2F0670(D_L30_001B1B64, D_L30_001B1B6C + i * D_L30_001B1B60, 0x80008080ULL, text, -1);
            break;
        }
    }
}
