#include "include_asm.h"

extern int func_00341E90(char *);

int func_US2_00342D68(char *object) { int index = func_00341E90(object); return *(int *)(object + 0xE8) + index * 20; }

