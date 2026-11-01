#include "include_asm.h"
extern char *D_001B23CC[] __attribute__((sda));
extern char *ref_L00_0034EF58_01[] __attribute__((sda));
extern char ref_L00_0034EF58_00[];
extern void func_0026FEB8(void *, ...);
extern void func_00284088(int, int);
extern int func_0011AB10(void);
extern void func_0011AAB0(int, int);
extern int func_0034F408(void *, void *, int);
extern int func_0034F048(void *, void *, void *);

int func_PAL_00351140(void *first, void *second, char *pool, char *state, int mode, int volume)
{
    int result = 0;
    ref_L00_0034EF58_01[0] = pool;
    D_001B23CC[0] = state;
    func_0026FEB8(ref_L00_0034EF58_00, 0x38cac0);
    func_00284088(volume, volume);
    func_0011AAB0(func_0011AB10(), 1);
    if (func_0034F408(first, second, mode)) {
        result = func_0034F048(D_001B23CC[0] + 0xd9048, D_001B23CC[0], D_001B23CC[0] + 0xd9040);
    }
    func_0034F690();
    ref_L00_0034EF58_01[0] = 0;
    D_001B23CC[0] = 0;
    return result;
}
