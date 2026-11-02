#include "include_asm.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef int TI128 __attribute__((mode(TI)));

extern void func_002A7750(char *, int, int);

extern void func_0032C700(void *a0, int a1, int a2, int a3, int a4, int a5, int a6,
                          int a7, float f12, float f13, int s1, int s2, int s3,
                          int s4);

extern void func_002A8C28(char *, char *, int, int, float, float, float, int, int);

extern void func_0029F338(char *object);
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_002E59F8(int);

/* func_L20_00449178 is defined in L20_00449178-state-g8.c. */

/* func_L20_00449338 is defined in L20_00449178-g8.c (it reaches data through $gp). */

/* func_L20_00449850 is defined in L20_00449850-g8.c (it reaches data through $gp). */

/* func_L20_004499E8 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

u32 func_L20_00449AD8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);

    return *(u32 *)(state + 0xAC);
}

void func_L20_00449A78(char *arg0) {
    *(u16 *)(arg0 + 0x34) &= 0xEFFF;
    func_002A7750(arg0, 1, -1);
    *(int *)(arg0 + 0x98) = 0;
    *(u16 *)(arg0 + 0x34) = (*(u16 *)(arg0 + 0x34) | 0x41) & 0xEFFF;
}

/* func_L20_00449AE8 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

/* func_L20_00449BB0 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

/* func_L20_00449CE0 is defined in L20_00449CE0-g16.c. */

/* func_L20_00449FD0 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

void func_L20_0044A108(char *arg0, char *arg1) {
    func_0032C700(arg0 + 0x10, 0, 5, 4, 4, 4, 7, 0, 0.333f, 20.0f, 0, 0, 0, 0);
    if (arg1 != 0) {
        func_002A8C28(*(char **)(arg0 + 0xB8), arg0 + 0x10, (int)arg1, 0, 1.5f,
                      4.0f, 1.0f, 1, 0);
    }
    func_0029F338(arg0);
}

/* func_L20_0044A1C8 is defined in L20_0044A1C8-g8.c (it reaches data through $gp). */

/* func_L20_0044A658 is defined in L20_00449178-g8.c (it reaches data through $gp). */

/* func_L20_0044A838 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

/* func_L20_0044A8F0 is defined in L20_00449178-g8.c (it reaches data through $gp). */

/* func_L20_0044AA88 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */

void func_L20_0044AB68(char *arg0) {
    union {
        TI128 q;
        float f[4];
    } v;
    char *state;

    v.q = 0;
    v.f[2] = 0.3f;
    func_00283098(&v, &v, arg0 + 0xC0);
    func_00282CC0(&v, &v, arg0 + 0x10);
    func_0032C700(&v, 0, 0x19, 0x14, 0x14, 0x14, 0x2D, 0, 0.666f, 20.0f, 1, 0,
                  0, 0);
    state = *(char **)(arg0 + 0x68);
    if (*(int *)(state + 0x1B8) != -1) {
        func_002E59F8(*(int *)(state + 0x1B8));
        *(int *)(state + 0x1B8) = -1;
    }
}

/* func_L20_0044AC38 is defined in L20_0044AC38-g8.c (it reaches a global
   through $gp).  Its near miss is nonmatching/levels/func_L20_0044AC38.c. */

/* func_L20_0044AE58 is defined in L20_0044AE58-g8.c. */

/* func_L20_0044B100 is defined in L20_00449178-g8.c (this unit has no $gp access for it). */
