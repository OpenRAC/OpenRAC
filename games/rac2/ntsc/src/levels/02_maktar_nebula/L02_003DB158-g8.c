#include "include_asm.h"

/* func_L02_003DB158 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int s32;
typedef float f32;
typedef int TQ __attribute__((mode(TI)));

extern f32 D_L02_001B0474 __attribute__((sda));
extern f32 D_L02_001B0478 __attribute__((sda));
extern s32 D_L02_001B0480 __attribute__((sda));

extern void *func_0029FCC0(char *, int, void *);
extern void func_0029FC20(char *, int, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_L00_00325AE8(void *, int, float, float);

typedef struct { f32 x, y, z, w; } V4;
typedef struct { f32 v[16]; } M4;

void func_L02_003DB158(char *a0, s32 a1) {
    char *s3 = *(char **)(*(char **)(a0 + 0xB8) + 0x68);
    V4 src, copy;
    M4 block;
    V4 out;

    func_0029FCC0(a0, a1, &src);
    *(TQ *)&copy = *(TQ *)&src;
    func_0029FC20(a0, a1, &block);
    func_002830C0(&out, &D_L02_001B0480, &block);
    func_00282CC0(&src, &src, &out);
    func_L00_00325AE8(&src, *(s32 *)(s3 + 0x250), D_L02_001B0478, D_L02_001B0474);
}
