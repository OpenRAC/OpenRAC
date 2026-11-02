/* func_002D2018 in its own unit, a sibling of func_002D01D0: the cursor goes through $gp, so the
   file is compiled at -G32 (config/cflags.txt). The text table's address is built by one lui/addiu
   pair in one register, the assembler's la macro, so it is declared MACRO_ADDR. */
#include "common.h"

typedef struct { char prefix[0x14]; char *source; int value; char gap[0x118]; int fallback; } SelectionCallback;
typedef struct { char reserved[0x1C4]; int flags; } CallbackMenuState;
typedef struct { char reserved00[0x34]; int text; int f38; int timer; } MenuPanel;
typedef struct { short text; short pad; } MenuText;

extern SelectionCallback D_001F2840;
extern CallbackMenuState D_00138180;
extern int func_002E5E58(int, unsigned int, void *);
extern int D_001ABA60 __attribute__((sda));
extern MenuPanel D_0025CAA0[];
extern int D_0025CB50[];
extern MenuText D_001ABD6A[] MACRO_ADDR;

int func_002D2018(void)
{
    int result = 0;
    int flags = ((CallbackMenuState *)&D_00138180)->flags;
    int old = D_001ABA60;

    if (flags & 0x10) {
        int value;
        int status;
        D_001ABA60 = 0;
        value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            status = -1;
            goto finish;
        }
        status = 0;
finish:
        return status;
    }
    if (flags & 0x900) {
        D_001ABA60 = 0;
        result = 1;
    } else if (flags & 0x8000) {
        if (old > 0)
            func_002E5E58(3, 0, 0);
        else
            func_002E5E58(5, 0, 0);
        if (--D_001ABA60 < 0)
            D_001ABA60 = 0;
    } else if (flags & 0x2000) {
        if (old < 5)
            func_002E5E58(3, 0, 0);
        else
            func_002E5E58(5, 0, 0);
        if (++D_001ABA60 > 5)
            D_001ABA60 = 5;
    }
    if (old != D_001ABA60)
        D_0025CAA0[0].timer = -300;
    D_0025CB50[0] = D_001ABA60;
    D_0025CAA0[0].text = D_001ABD6A[D_001ABA60].text;
    return result;
}
