#include "include_asm.h"
extern char *D_001A8CFC;
extern int ref_L00_002D5570_00 __attribute__((sda));
extern int ref_L00_002D5570_01 __attribute__((sda));
extern void func_00339798(char *);
extern void func_00339818(char *);

int func_US2_002D61D8(char *a0)
{
    if (D_001A8CFC != 0) {
        if (a0 == (char *)&ref_L00_002D5570_00)
            func_00339798(D_001A8CFC + 0x3CEA0);
        else if (a0 == (char *)&ref_L00_002D5570_01)
            func_00339818(D_001A8CFC + 0x3CEA0);
    }
    return 0;
}
