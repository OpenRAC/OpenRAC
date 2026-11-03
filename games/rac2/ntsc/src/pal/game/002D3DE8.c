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
extern void func_0029C760(int);

int func_PAL_002D3DE8(void) {
    int result = 0;
    int flags = D_00138344;
    if (flags & 0x10) {
        int value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else
        func_0029C760(flags);
    return result;
}

extern void func_0027C540(long);
extern void func_0027C660(void);
void func_0027C540(long);
void func_0027C660(void);
extern void func_0027C540(long arg0);
extern void func_0029C7A0(void);
extern void func_0027C540(long param_1);

unsigned long long func_PAL_002D3E70(void)
{
    func_0027C540(0);
    func_0029C7A0();
    func_0027C660();
    return 0;
}

extern void func_0029C7D0(int);

int func_PAL_002D3EA0(void) {
    int result = 0;
    int flags = D_00138344;
    if (flags & 0x10) {
        int value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else
        func_0029C7D0(flags);
    return result;
}

extern void func_0029C810(void);

int func_PAL_002D3F28(void) {
    func_0027C540(0);
    func_0029C810();
    func_0027C660();
    return 0;
}

extern void func_0029CA00(int);

int func_PAL_002D3F58(void) {
    int result = 0;
    int flags = D_00138344;
    if (flags & 0x10) {
        int value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else
        func_0029CA00(flags);
    return result;
}

extern void func_0029CA40(void);

int func_PAL_002D3FE0(void) {
    func_0027C540(0);
    func_0029CA40();
    func_0027C660();
    return 0;
}

extern void func_0029C990(int);

int func_PAL_002D4010(void) {
    int result = 0;
    int flags = D_00138344;
    if (flags & 0x10) {
        int value = *(int *)(D_001F2840.source + 0xE0);
        if (value != 0)
            D_001F2840.value = value;
        else if (D_001F2840.fallback == 0) {
            value = -1;
            goto return_value;
        }
        value = 0;
return_value:
        return value;
    }
    if (flags & 0x900)
        result = 1;
    else
        func_0029C990(flags);
    return result;
}

extern void func_0029C9D0(void);

int func_PAL_002D4098(void) {
    func_0027C540(0);
    func_0029C9D0();
    func_0027C660();
    return 0;
}

