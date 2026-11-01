#include "include_asm.h"
typedef struct {
    char reserved00[0x14];
    char *source;
    int value;
    char reserved1c[0x118];
    int fallback;
} CallbackState;
extern CallbackState D_001F2840;
extern int D_00138344;
extern void func_0028BD68(void);
extern int func_0029CBE0(int);

int func_PAL_002CE600(void) {
    int result = 0;
    int flags = D_00138344;
    if (flags & 0x10) {
        int value;
        func_0028BD68();
        value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            result = -1;
            goto done;
        }
        result = 0;
done:
        ;
    } else if (flags & 0x900) {
        func_0028BD68();
        result = 1;
    } else {
        func_0029CBE0(flags);
    }
    return result;
}
