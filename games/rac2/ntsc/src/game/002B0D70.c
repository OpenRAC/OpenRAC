#include "common.h"

/* A unit of its own so that the call to func_002B0DB8 goes through its symbol: USA v2.00 has has
   that function as assembly at an address of its own, and a call within one object can only
   reach the object's own copy. */

extern s32 func_002B0D60(void);
extern int func_002B0DB8(void);

int func_002B0D70(void) {
    int count = func_002B0DB8();
    int diff = count - func_002B0D60();
    if (diff < 0) diff = 0;
    if (diff > 40) diff = 40;
    return diff;
}
