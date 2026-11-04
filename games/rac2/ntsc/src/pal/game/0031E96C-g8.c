#include "include_asm.h"
extern int D_001B2758;
extern void func_0031EFC8(char *, char *);
extern char *func_002A7820(int, int);
extern float func_002A7940(void);
extern void func_0031EB00(char *, unsigned int, float, char *, float, int, int);

void func_PAL_0031F8B8(char *p)
{
    int offset = 0;
    int count = 0xB;
    char *entry;
    char *ptr;
    float f;

    do {
        count--;
        entry = (char *)(offset + (unsigned int)D_001B2758);
        if (*(int *)(entry + 0x8C) == (int)p) {
            func_0031EFC8(p, entry);
            if (*(int *)(entry + 0x94) == 0) {
                ptr = func_002A7820(0x50, 0x60);
                f = func_002A7940();
                func_0031EB00(entry + 0x30, 0x103080, 0.5f, ptr, f, 0x31, 2);
                ptr = func_002A7820(0x50, 0x60);
                f = func_002A7940();
                func_0031EB00(entry + 0x30, 0xFFFFFF, 0.3f, ptr, f, 0x31, 2);
            }
        }
        offset += 0xA0;
    } while (count >= 0);
}
