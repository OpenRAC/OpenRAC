#include "include_asm.h"

extern char *D_L00_001B1E9C;
extern char *D_L00_001B1EA4;
extern int D_L00_001AFD7C __attribute__((sda));
extern char *D_L00_0023DBA0[];
extern int D_L00_0023DCA0[];
extern char D_L00_001AFD80[];
extern int D_001A79F0;
extern void func_0026FEB8(const char *,...);

void func_L00_003B9170(void)
{
    char *p;
    D_L00_001AFD7C=0;
    for (p=D_L00_001B1E9C; p<D_L00_001B1EA4; p+=0x100) {
        short id=*(short *)(p+0xaa);
        if (id==0x1146 || id==0x1185 || id==0x119b || id==0x11a4 || id==0x1204 || id==0x11fc || id==0x120c || id==0x1135 || id==0x1148 || id==0x11fc) {
            D_L00_0023DBA0[D_L00_001AFD7C]=p;
            D_L00_0023DCA0[D_L00_001AFD7C]=*(int *)(p+0x98);
            D_L00_001AFD7C++;
            if (D_L00_001AFD7C>=64) {
                func_0026FEB8(D_L00_001AFD80,64,D_001A79F0);
                return;
            }
        }
    }
}
