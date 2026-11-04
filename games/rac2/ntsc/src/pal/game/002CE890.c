#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00341550(void *, int);
extern char *D_001A8CFC[] __attribute__((sda));
extern void *func_00341558(void *);
extern char *D_001A8CFC[] __attribute__((sda));

int func_PAL_002CE890(void *object) { char *root=D_001A8CFC[0]; if (root != 0) { func_00341550(root+0x3C210,1); *(void **)((char *)object+0x34)=func_00341558(D_001A8CFC[0]+0x3C210); } return 0; }

