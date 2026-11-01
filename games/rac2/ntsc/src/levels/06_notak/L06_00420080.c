#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));

extern int *D_L06_001FE500[];
extern char D_00189EA0[];
typedef struct {
    char pad0[0x80];
    TI128 f80;
    char pad1[0x2290 - 0x90];
    char *f2290;
} L06Hero;

extern L06Hero D_00189E20;

extern int func_35A620(int);
extern int func_361930(char *, void *, int, int, void *, int, float);
extern int func_361728(char *, void *, float);
extern float func_32D7C0(void *, void *);
extern void func_36A798(char *, int, int, int, float, float);

int func_L06_00420080(char *a0)
{
    char *state = *(char **)(a0 + 0x68);
    int *entry;

    if (*(unsigned char *)(a0 + 0x20) == 0 || func_35A620(7) == 0) {
        if (*(int *)(state + 0x260) != -1) {
            char *target;

            if (*(unsigned char *)(a0 + 0x20) < 3)
                entry = D_L06_001FE500[*(int *)(state + 0x260)];
            else
                entry = D_L06_001FE500[*(int *)(state + 0x264)];
            target = state + 0x1C0;
            if (func_361930(a0, target, 0, 0, (char *)entry + 0x10, *entry, *(float *)(state + 0x290)) != 2) {
                if (func_32D7C0(a0 + 0x10, target) < 8.0f)
                    *(int *)(state + 0x258) = 2;
                else
                    *(int *)(state + 0x258) = 1;
            } else {
                *(TI128 *)target = *(TI128 *)D_00189EA0;
                *(int *)(state + 0x258) = 0;
            }
        } else {
            char *target = state + 0x1C0;

            if (func_361728(a0, target, *(float *)(state + 0x290)) == 2) {
                *(TI128 *)target = *(TI128 *)D_00189EA0;
                *(int *)(state + 0x258) = 0;
            } else if (func_32D7C0(a0 + 0x10, target) < 8.0f) {
                *(int *)(state + 0x258) = 2;
            } else {
                *(int *)(state + 0x258) = 1;
            }
        }
    } else if (*(char **)(state + 0x200) == D_00189E20.f2290) {
        *(TI128 *)(state + 0x1C0) = D_00189E20.f80;
    }
    func_36A798(a0, -1, 0, 0, *(float *)(state + 0x338) * 20.0f, 10.0f);
    return *(int *)(state + 0x258);
}
