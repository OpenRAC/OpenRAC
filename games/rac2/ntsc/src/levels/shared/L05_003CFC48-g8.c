#include "common.h"

typedef struct { char pad[16]; int limit,value; char tail[16]; } RewardSlot;
extern RewardSlot D_L05_002A1800[];
extern int D_L05_00221D10[];
extern int func_002ED8A0(int);
extern char *func_L05_0041C118(char *);
extern char *func_L05_0041D498(char *);

void func_L05_003CFC48(char *object)
{
    int slot=func_002ED8A0(1);
    if(slot<0 || D_L05_002A1800[slot].value==D_L05_002A1800[slot].limit) {
        func_L05_0041C118(object);
        D_L05_00221D10[23]++;
        D_L05_00221D10[28]++;
    } else {
        func_L05_0041D498(object);
        D_L05_00221D10[25]++;
        D_L05_00221D10[28]++;
    }
}
