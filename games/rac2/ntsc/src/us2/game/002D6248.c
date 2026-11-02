#include "include_asm.h"

extern void func_0027C540(long);
extern void func_0027C660(void);
void func_0027C540(long);
void func_0027C660(void);
extern void func_0027C540(long arg0);
extern void func_0027C540(long param_1);
extern void func_0029CB20(void);

int func_US2_002D6380(void) {
    func_0027C540(0);
    func_0029CB20();
    func_0027C660();
    return 0;
}

extern char *D_001A8CFC[] __attribute__((sda));
extern void func_00341CD8(char *, int);
extern void func_00341EB0(char *, char *);
extern char D_00261540[];

int func_US2_002D63B0(void)
{
    if (D_001A8CFC[0] != 0) {
        func_00341CD8(D_001A8CFC[0] + 0x3E9C8, 1);
        func_00341EB0(D_001A8CFC[0] + 0x3E9C8, D_00261540);
    }
    return 0;
}

extern int D_00138180[];
extern char D_001F2840[];
extern int func_0029CA70(int);
extern int func_003423B0(unsigned char *);
extern void func_002D5E98(int);

int func_US2_002D6408(void)
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
        func_0029CA70(flags);
        if ((state[0x1C4 / 4] & 0x40) && D_001A8CFC[0]) {
            char *entry = (char *)func_003423B0((unsigned char *)(D_001A8CFC[0] + 0x3E760));
            struct { short unused; unsigned short id; int value; } request;
            request.id = *(unsigned short *)(entry + 8);
            request.value = *(int *)(entry + 12);
            func_002D5E98((int)&request);
        }
    }
    return result;
}

