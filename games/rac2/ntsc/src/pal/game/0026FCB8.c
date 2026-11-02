#include "include_asm.h"

typedef struct { char pad[0x32C]; int base; int offset; int count; } Table26FE48;
extern Table26FE48 D_0014B540;
extern char D_001C5540[];
extern int D_001A730C __attribute__((sda));
extern unsigned long long D_001A7468 __attribute__((sda));
extern int func_002B7C80(char *, int, int);
extern void func_0026FC78(char *, unsigned long long *, int, int);

void func_PAL_0026FCB8(void)
{
    unsigned long long local[4];
    char *data = D_001C5540;
    func_002B7C80(data, D_0014B540.offset + D_0014B540.base, D_0014B540.count);
    func_0026FC78(data, local, D_001A730C, 0x3FFC00);
    *(volatile unsigned long long *)&D_001A7468 = local[0];
}

