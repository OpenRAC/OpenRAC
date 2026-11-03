#include "include_asm.h"

extern char *D_001A8CFC[] __attribute__((sda));
extern void func_003423A8(char *, int);
extern void func_00342380(char *, char *);
extern char D_00261590[];

int func_US2_002D6508(void)
{
    if (D_001A8CFC[0] != 0) {
        func_003423A8(D_001A8CFC[0] + 0x3E760, 1);
        func_00342380(D_001A8CFC[0] + 0x3E760, D_00261590);
    }
    return 0;
}

