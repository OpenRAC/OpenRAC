#include "include_asm.h"
#include "common.h"

extern char *D_001B2348 __attribute__((sda));
extern void func_002FF290(char *, int);
extern void func_002A7398(char *, int, int);
extern void func_002A0F48(char *);

char *func_002FF300(int a0, int a1)
{
    char *obj;
    obj = D_001B2348;
    func_002FF290(obj, a0);
    *(unsigned short *)(obj + 0x34) &= 0xFFBC;
    *(unsigned char *)(obj + 0x30) = 0xFF;
    *(short *)(obj + 0x32) = 0x20;
    *(unsigned char *)(obj + 0x31) = 1;
    *(float *)(obj + 0x10) = 5.0f;
    *(float *)(obj + 0x14) = 5.0f;
    *(float *)(obj + 0x18) = 900.0f;
    *(float *)(obj + 0x1C) = 1.0f;
    qzero(obj + 0xF0);
    if (a1 >= 0) {
        func_002A7398(obj, a1, 0);
    } else {
        char *p = *(char **)(obj + 0x24);
        if (p != 0 && *(unsigned char *)(p + 0xC) >= 2)
            func_002A7398(obj, 1, 0);
    }
    if ((*(unsigned short *)(obj + 0x34) & 4) == 0)
        func_002A0F48(obj);
    return obj;
}
