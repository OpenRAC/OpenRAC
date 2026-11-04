#include "include_asm.h"

/* The model ends in a variable-length table of 16-byte records. */
typedef struct {
    char pad00[0x80];
    int index;
    int state;
    char pad88[0xBC];
    struct { int first; int limit; int third; int fourth; } cells[0]; /* Variable-length trailing table. */
} LimitModel;
extern LimitModel D_L00_0022F7C0;

extern int D_L00_001AD7A0 __attribute__((sda));

extern int D_L00_001AD7A4;

extern int func_002B0D70(void);

extern unsigned char *func_00288FB8(int);

extern int func_0027FB40(int, int, long, unsigned char *, int);

void func_L00_00378DE0(void)
{
    LimitModel *base = &D_L00_0022F7C0;
    int index;
    int limit;
    int r;

    if (base->state == 0)
        return;
    index = base->index;
    limit = base->cells[index].limit;
    r = func_002B0D70();
    if (r >= limit)
        func_0027FB40(D_L00_001AD7A0, D_L00_001AD7A4, 0x80F0F0F0, func_00288FB8(0x1129), -1);
    else
        func_0027FB40(D_L00_001AD7A0, D_L00_001AD7A4, 0x80F0F0F0, func_00288FB8(0x1128), -1);
}
