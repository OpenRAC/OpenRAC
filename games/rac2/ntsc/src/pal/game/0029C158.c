#include "include_asm.h"
typedef struct { int a; int b; } Pair;
typedef struct { char pad[0x7AC8]; Pair pair[2]; } State;
extern char *D_001A8CFC[] __attribute__((sda));
int func_PAL_0029C158(int a0, int a1, int a2)
{
    if (D_001A8CFC[0] != 0 && (unsigned int)a2 < 2) {
        State *s = (State *)(D_001A8CFC[0] + 0x38000);
        s->pair[a2].a = a0;
        s->pair[a2].b = a1;
        return 1;
    }
    return 0;
}
