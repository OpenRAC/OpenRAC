#include "common.h"

extern int func_00282948(void *);
extern void func_002AB8C0(void *, unsigned char *);
extern void func_L03_003C4578(unsigned char *);
extern void func_002AEC48(void *, void *, int, float, float);
extern int func_L03_00419B58(unsigned int, int);
extern unsigned char D_001A7A43;

void func_L03_003C4468(unsigned char *object)
{
    unsigned char *state = *(unsigned char **)(object + 0x68);
    func_002AB8C0(object, state + 0x50);
    if (*(unsigned short *)(object + 0x96) < 0xFFFF)
        ++*(unsigned short *)(object + 0x96);
    func_00282948(state + 0x364);
    func_L03_003C4578(object);
    if (D_001A7A43 != 0 && object[0x20] != 1)
        *(float *)(state + 0x2B0) = 1.75f;
    func_002AEC48(object, state + 0x240, 1, 0.03f, 0.3f);
    func_002AEC48(object, state + 0x2C0, 0, 0.03f, 0.3f);
    if (object[0x95] != 7 && object[0x20] != 1)
        *(int *)(state + 0x14C) = func_L03_00419B58(*(unsigned int *)(state + 0x220), *(int *)(state + 0x368));
}
