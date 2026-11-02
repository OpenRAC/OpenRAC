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
extern void func_0029C990(int);

int func_US2_002D4030(void) {
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

extern void func_0027C540(long);
extern void func_0027C660(void);
void func_0027C540(long);
void func_0027C660(void);
extern void func_0027C540(long arg0);
extern void func_0027C540(long param_1);
extern void func_0029C9D0(void);

int func_US2_002D40B8(void) {
    func_0027C540(0);
    func_0029C9D0();
    func_0027C660();
    return 0;
}

extern int func_0029C840(int);
extern void func_002D5B58(short, int, int *);

int func_US2_002D40E8(void) {
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
    {
        int request;
        if (func_0029C840(flags)) {
            request = 0;
            func_002D5B58(13, 0, &request);
        }
    }
    return result;
}

extern void func_0029C880(void);

int func_US2_002D4188(void) {
    func_0027C540(0);
    func_0029C880();
    func_0027C660();
    return 0;
}

extern void func_0029C8B0(int);

int func_US2_002D41B8(void) {
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
        func_0029C8B0(flags);
    return result;
}

extern void func_0029C8F0(void);

/* Implementation matching decompiled output */
long func_US2_002D4240(void)
{
  func_0027C540(0);
  func_0029C8F0();
  func_0027C660();
  return 0;
}

