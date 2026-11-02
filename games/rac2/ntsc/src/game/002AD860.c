#include "include_asm.h"
typedef struct { char pad[12]; int state; int pad10; int handle; char rest[200]; } RequestRow224;
extern unsigned char D_00139568[];
extern RequestRow224 D_00239BA0[];
extern short D_001A63AC;
extern int D_0018C0E8, D_0018C0EC, D_0018C0F0, D_0018C0F4;
extern int func_00294630(int);
extern void func_00294420(int);
int func_002AD860(int index)
{
    RequestRow224 *rows = D_00239BA0;
    unsigned char *key = D_00139568 + index;
    int result = 0;
    int state = rows[*key].state;
    if (state == 0) {
        if (!func_00294630(rows[*key].handle) && D_001A63AC) goto done;
        D_0018C0E8 = index;
        if (!func_00294630(rows[*key].handle)) func_00294420(rows[*key].handle);
        result = 1;
    } else if (state == 3) { D_0018C0F4 = index; result = 1; }
    else if (state == 2) { D_0018C0F0 = index; result = 1; }
    else if (state == 1) { D_0018C0EC = index; result = 1; }
done:
    return result;
}