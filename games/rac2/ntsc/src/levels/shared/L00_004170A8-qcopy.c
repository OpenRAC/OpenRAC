#include "include_asm.h"
typedef int V4_L00_004170A8 __attribute__((mode(TI)));
extern char D_L00_0025A920[];
extern V4_L00_004170A8 D_L00_001BF220;
extern int func_00282948(void *);
extern void func_L00_004173B0(char *);
extern float func_002A7940(void);
extern void func_002AF000(void *, float, float, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002761F8(void *, void *, int, void *, int);
extern void func_L00_00417A40(char *, void *, int);
extern void func_L00_00417D70(char *);
extern void func_L00_00417218(char *);
extern void func_L00_00417700(char *);
extern void func_0029F338(char *);
extern void ref_L00_004170A8_00(void (*)(char *), char *);

void func_L00_004170A8(char *object)
{
    char *state = *(char **)(object + 0x68);
    V4_L00_004170A8 local;
    float a, b;
    int i;
    char *p;
    if (func_00282948(state) == 0) {
        if (*(int *)(state + 4) == 0)
            func_L00_004173B0(object);
        if (*(int *)(state + 4) == 0) {
            a = func_002A7940();
            b = func_002A7940();
            func_002AF000(&local, 7.0f, a, b);
        } else {
            a = func_002A7940();
            b = func_002A7940();
            func_002AF000(&local, 5.0f, a, b);
        }
        func_00282CC0(&local, &local, object + 0x10);
        if (func_002761F8(object + 0x10, &local, 0, object, 0) != 0)
            local = D_L00_001BF220;
        func_L00_00417A40(object, &local, 20);
        func_L00_00417D70(object);
        ref_L00_004170A8_00(func_L00_00417700, object);
    } else {
        p = D_L00_0025A920;
        p += 0x106C;
        for (i = 23; i >= 0; --i, p -= 0xB0)
            *(int *)p = 0;
        if (*(int *)(state + 4) == 0)
            func_L00_00417218(object);
        func_0029F338(object);
    }
}
