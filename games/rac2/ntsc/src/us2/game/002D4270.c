#include "include_asm.h"

extern int D_00138180[];
extern char D_001F2840[];
extern int D_001A8CFC __attribute__((sda));
extern int func_0029C920(int);
extern int func_0033E310(int);
extern void func_002D5E98(int);

int func_US2_002D4270(void)
{
    int result = 0;
    int *state = D_00138180;
    int flags = state[0x1C4 / 4];
    if (flags & 0x10) {
        char *root = D_001F2840;
        int value = *(int *)(*(char **)(root + 0x14) + 0xE0);
        int status;
        if (value != 0) {
            *(int *)(root + 0x18) = value;
        } else {
            switch (*(int *)(root + 0x134)) {
            case 0:
                status = -1;
                goto finish;
            default:
                break;
            }
        }
        status = 0;
finish:
        return status;
    }
    if (flags & 0x900)
        result = 1;
    else {
        func_0029C920(flags);
        if ((state[0x1C4 / 4] & 0x40) && D_001A8CFC) {
            char *entry = (char *)func_0033E310(D_001A8CFC + 0x3F3F0);
            struct { short unused; unsigned short id; int value; } request;
            request.id = *(unsigned short *)(entry + 8);
            request.value = *(int *)(entry + 12);
            func_002D5E98((int)&request);
        }
    }
    return result;
}

extern void func_0027C540(long);
extern void func_0027C660(void);
void func_0027C540(long);
void func_0027C660(void);
extern void func_0027C540(long arg0);
extern void func_0027C540(long param_1);
extern void func_0029C960(void);

/* Candidate function: FUN_002d3390 */
unsigned long long func_US2_002D4340(void)
{
    func_0027C540(0);
    func_0029C960();
    func_0027C660();
    return 0;
}

extern void func_0029C5A0(void);

int func_US2_002D4438(void) {
    func_0027C540(0);
    func_0029C5A0();
    func_0027C660();
    return 0;
}

