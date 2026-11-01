#include "include_asm.h"

extern void func_0027C540(int a);
extern void func_0027C660(void);
extern void func_L00_0030DBF8(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
int func_US2_L00_0034FBF0(void)
{
    func_0027C540(0);
    func_L00_0030DBF8();
    func_0027C660();
    return 0;
}

extern int D_L00_001A904C_3 __attribute__((sda));
typedef struct {
    char reserved00[0x14];
    char *source;
    int value;
    char reserved1c[0x118];
    int fallback;
} CallbackStateL001FEC40;
extern CallbackStateL001FEC40 D_L00_001FEC40;
extern int D_00138180[];
extern int func_L00_0030DB48(int);
extern int func_003423B0(int);
extern void func_002D5E98(int);

/* The same guarded callback update with its own setup call and pending-request reader. */
int func_US2_L00_0034FC78(void)
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
        func_L00_0030DB48(flags);
        if ((state[0x1C4 / 4] & 0x40) && D_L00_001A904C_3) {
            char *entry = (char *)func_003423B0(D_L00_001A904C_3 + 0x3E760);
            struct { short unused; unsigned short id; int value; } request;

            request.id = *(unsigned short *)(entry + 8);
            request.value = *(int *)(entry + 12);
            func_002D5E98((int)&request);
        }
    }
    return result;
}

