#include "include_asm.h"

extern void func_002E37B0(void);
extern void func_002E3548(void);
extern void func_002E3820(void);
extern void func_002FC560(unsigned int reg, unsigned long value);

/* per level: the pointer handed to the first setter and the $gp slot it lands in */
extern int ref_L01_002F1F70_00 __attribute__((sda));
extern char ref_L01_002F1F70_01[];

extern int D_001A72E0 __attribute__((sda));

void func_L01_002F1F70(void)
{
    func_002E37B0();
    ref_L01_002F1F70_00 = (int)ref_L01_002F1F70_01;
    func_002E3548();
    func_002E3820();
    func_002FC560(0x47, 0x5360B);
    func_002FC560(0x4E, 0x01000000 | (D_001A72E0 >> 13));
}
