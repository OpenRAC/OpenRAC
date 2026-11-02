#include "include_asm.h"

/* func_002EE988 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int V4 __attribute__((mode(TI)));
extern int func_002ED688(unsigned char *owner);
extern unsigned int func_002ED870(void);
extern void func_00282CF0(void *a0, void *a1, void *a2);
extern float func_00282DF0(char *a0);
extern void func_00282D30(char *a0, char *a1, float f);
extern void func_00282F20_10(V4 *, char *, float);
extern void func_00282CC0_26(char *, V4 *, char *);
extern int func_002761F8(char *a0, char *a1, int mode, char *self, int flag);
extern int func_00277258(char *a0, int mode, char *self, float scale);
extern V4 D_001BAFA0;
extern V4 D_001BAFB0;
extern char *D_001B1C54 __attribute__((sda));
extern int D_00214F90[];

void func_002EE988(char *owner)
{
    float a[4];
    float b[4];
    float c[4];
    float d[4];
    float e[4];
    char *object;
    char *pos;
    char *table;
    float len;

    if (func_002ED870() != 0)
        return;
    object = (char *)func_002ED688((unsigned char *)owner);
    if (object == 0)
        return;
    pos = object + 0x60;
    table = D_001B1C54 + D_00214F90[0] * 128;
    func_00282CF0(a, pos, table + 0x30);
    len = func_00282DF0((char *)a);
    if (*(float *)table < len) {
        func_00282F20_10((V4 *)a, (char *)a, len - (len - *(float *)table));
        func_00282CC0_26(pos, (V4 *)a, table + 0x30);
    }
    if (func_002761F8(owner + 0x10, pos, 2, owner, 0)) {
        V4 *pd = (V4 *)d;
        V4 *pe;
        *pd = D_001BAFA0;
        func_00282D30((char *)b, (char *)&D_001BAFA0 + 0x20, 0.00097656f);
        func_00282F20_10((V4 *)c, (char *)b, 5.0f);
        pe = (V4 *)e;
        func_00282CC0_26((char *)pe, pd, (char *)c);
        *(V4 *)pos = *pe;
    }
    if (func_00277258(pos, 2, owner, 5.0f)) {
        *(V4 *)pos = D_001BAFB0;
    }
}
