#include "include_asm.h"

extern int D_L00_001A904C_3 __attribute__((sda));
extern int func_00344378(int x);

/* When the global is set, derives a pointer from it plus a fixed offset, runs it through a callee and stores the result into a field of the argument. */
int func_PAL_L00_003493D0(int *obj)
{
    if (D_L00_001A904C_3 != 0)
        obj[13] = func_00344378(D_L00_001A904C_3 + 0x3A0B0);
    return 0;
}

extern void func_0027C540_7(long);
extern void func_L00_0030D528(void);
extern void func_0027C660_7(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
long long func_PAL_L00_00349418(void)
{
    func_0027C540_7(0LL);
    func_L00_0030D528();
    func_0027C660_7();
    return 0LL;
}

typedef struct {
    char reserved00[0x14];
    char *source;
    int value;
    char reserved1c[0x118];
    int fallback;
} CallbackStateL001FEC40;
extern CallbackStateL001FEC40 D_L00_001FEC40;
extern int D_00138180[];
extern int func_L00_0030D5C8(int);
extern int func_003415B8(int);
extern void func_002D5E98(int);

int func_PAL_L00_003494A8(void)
{
    int result = 0;
    int *state = D_00138180;
    int flags = state[0x1C4 / 4];

    if (flags & 0x10) {
        int value = *(int *)(D_L00_001FEC40.source + 0xE0);

        if (value != 0)
            D_L00_001FEC40.value = value;
        else if (D_L00_001FEC40.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else {
        func_L00_0030D5C8(flags);
        if ((state[0x1C4 / 4] & 0x40) && D_L00_001A904C_3) {
            char *entry = (char *)func_003415B8(D_L00_001A904C_3 + 0x3C210);
            struct { short unused; unsigned short id; int value; } request;

            request.id = *(unsigned short *)(entry + 8);
            request.value = *(int *)(entry + 12);
            func_002D5E98((int)&request);
        }
    }
    return result;
}

