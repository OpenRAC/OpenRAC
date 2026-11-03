#include "include_asm.h"
typedef struct {
    char pad0[0x2290];
    char *moby;
    int state;
    char pad2298[0x229C - 0x2298];
    int mode;
    char pad22A0[0x22B7 - 0x22A0];
    unsigned char b22B7;
    char pad22B8[0x22BC - 0x22B8];
    unsigned char b22BC;
    unsigned char pad22BD;
    unsigned char b22BE;
} Hero_2B2120;

extern Hero_2B2120 D_00189E20;
extern int func_L00_002ADF80(void);
extern void func_L00_002AFBF0(void);
extern void func_L00_002B0968(char *);
extern void func_L00_002D4A38(void);
extern void func_L00_002A7E60(void);
extern void func_L00_002B1F90(void);

void func_L00_002B2120(void)
{
    int m = D_00189E20.mode;

    if ((unsigned int)(m - 0x15) < 2 || m == 0x1F || func_L00_002ADF80() != 0) {
        D_00189E20.b22B7 = 1;
        D_00189E20.b22BC = 1;
    }
    if (func_L00_002ADF80() != 0 || D_00189E20.state == 0x31 || D_00189E20.mode == 3 ||
        D_00189E20.state == 0x12 || D_00189E20.state == 0x82)
        D_00189E20.b22BE = 1;
    else
        D_00189E20.b22BE = 0;
    func_L00_002AFBF0();
    func_L00_002B0968(D_00189E20.moby);
    func_L00_002D4A38();
    func_L00_002A7E60();
    func_L00_002B1F90();
}
