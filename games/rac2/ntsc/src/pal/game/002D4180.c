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
extern void func_0029C8B0(int);

int func_PAL_002D4180(void) {
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

extern void func_0027C540(long);
extern void func_0027C660(void);
void func_0027C540(long);
void func_0027C660(void);
extern void func_0027C540(long arg0);
extern void func_0029C8F0(void);
extern void func_0027C540(long param_1);

/* Implementation matching decompiled output */
long func_PAL_002D4208(void)
{
  func_0027C540(0);
  func_0029C8F0();
  func_0027C660();
  return 0;
}

extern void func_0029C960(void);

/* Candidate function: FUN_002d3390 */
unsigned long long func_PAL_002D4308(void)
{
    func_0027C540(0);
    func_0029C960();
    func_0027C660();
    return 0;
}

extern char *D_001A8CFC[] __attribute__((sda));
extern float ref_L00_002D33C0_00[];
extern void ref_L00_002D33C0_01(unsigned char *, float *);
extern char *D_001A8CFC[] __attribute__((sda));
extern char *D_001A8CFC[] __attribute__((sda));

int func_PAL_002D4338(void)
{
    if (D_001A8CFC[0] != 0)
        ref_L00_002D33C0_01((unsigned char *)(D_001A8CFC[0] + 0x3F4A0), ref_L00_002D33C0_00);
    return 0;
}

typedef struct {
    char reserved00[0x1C0];
    int message;
    int flags;
} MenuGameState;
extern MenuGameState D_00138180;
extern void func_0029C5D0(int);

int func_PAL_002D4378(void)
{
    int flags = D_00138180.flags;
    int result = 0;
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
        func_0029C5D0(D_00138180.message);
    return result;
}

extern void func_0029C5A0(void);

int func_PAL_002D4400(void) {
    func_0027C540(0);
    func_0029C5A0();
    func_0027C660();
    return 0;
}

