#include "include_asm.h"

extern void func_0034CEA0(void *);
extern void func_0034E4B8(int);
extern void func_00338A60(int);
extern void func_0029CB58(void);

void func_PAL_00350660(char *p)
{
    func_0034CEA0(p + 0x7A0);
    func_0034E4B8((int)p);
    func_00338A60((int)(p + 0x1FDC));
    func_0029CB58();
}

void func_00338A60(int);

void func_PAL_003506A0(int param_1)
{
  func_00338A60(param_1 + 0x1D8C);
}

void func_00338F58(int);

void func_PAL_003506C0(int param_1)
{
  func_00338F58(param_1 + 0x1D8C);
  return;
}

