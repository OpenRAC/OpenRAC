#include "include_asm.h"
extern unsigned char D_001A7AFA;
extern char D_00189EA0[];
extern unsigned short D_0018C31C[];
extern int func_002ACBC8(void *, int);
extern void func_L02_003DBFD8(void);
extern void func_002A7750(char *, int, int);
extern void func_002A7750();

void func_PAL_L02_003E0110(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    switch (*(unsigned char *)(arg0 + 0x20)) {
    case 0:
        *(unsigned char *)(arg0 + 0x30) = 0xFF;
        if (D_001A7AFA == 0)
            D_0018C31C[0] = 1;
        func_002A7750(arg0, 1, -1);
        break;
    case 1:
        func_L02_003DBFD8();
        if (func_002ACBC8(D_00189EA0, *(int *)state))
            *(unsigned short *)(D_00189EA0 + 0x19A) = 4;
        break;
    }
}
