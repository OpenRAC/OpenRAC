#include "include_asm.h"
typedef struct { int a; int b; } Pair;
typedef struct { char pad[0x7AC8]; Pair pair[2]; } State;
extern char *D_001A8CFC __attribute__((sda));
void func_PAL_0029C1A8(int index)
{
    if ((unsigned int)index < 2) {
        State *s = (State *)(D_001A8CFC + 0x38000);
        s->pair[index].a = 0;
        s->pair[index].b = 0;
    }
}
