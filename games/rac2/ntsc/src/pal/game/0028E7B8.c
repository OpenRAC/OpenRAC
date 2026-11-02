#include "include_asm.h"

typedef unsigned char u8;

void func_PAL_0028E7B8(char *object) { extern void func_0028B950(u8 *); *(int *)(object + 0x58) = 0x20; *(int *)(object + 0x7C) = 0x82; *(int *)(object + 0x5C) = 0x20; func_0028B950((u8 *)object); }

