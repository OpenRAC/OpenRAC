#include "include_asm.h"
extern int D_001A79F0;
extern char *func_002ED688(char *object);
extern void func_L05_00379FA0(char *, int, int, int, float, float, float, float, int, int, int);
extern int func_002A77E0(int);
extern void func_002E5DA0(int, int, char *, int);

/* Spawns the object's effect once its guards clear, then runs the timer and marks the run.
 * The two words of the effect's parameters are the same in both calls, so the compiler
 * loads the constant once and copies it into the two argument registers. */
void func_PAL_L05_003D4168(char *obj)
{
    char *s1 = *(char **)(obj + 0x68);
    char *r;

    r = func_002ED688(obj);
    if (r == 0)
        return;
    if (*(int *)(r + 0x120) == 0)
        return;
    if (*(int *)(s1 + 0x1B8) != 0)
        return;
    if (*(unsigned char *)(obj + 0x20) == 1) {
        if (D_001A79F0 == 0xF)
            func_L05_00379FA0(obj, 0x60008000, 1, 0, 0.6f, 2.0f, 1.0f, 1.0f, 0, -1, 0);
        else
            func_L05_00379FA0(obj, 0x60008000, 1, 0, 1.0f, 2.0f, 1.0f, 1.0f, 0, -1, 0);
    }
    if (func_002A77E0(3) == 0)
        func_002E5DA0(0, 0, obj, 0x957);
    *(int *)(s1 + 0x1B8) = 0x11;
}
