#include "common.h"
typedef struct { unsigned char pad[142]; unsigned short f8E; unsigned char rest[80]; } Entry_262570;
extern Entry_262570 D_L00_00262570[];

extern int func_L00_002D3FB0(int);
extern unsigned char D_00139568[];
extern int D_00139688[];

int func_L00_002D3FB8(int a0, int a1)
{
    int idx = func_L00_002D3FB0(a0);
    int *slot = &D_00139688[idx];
    Entry_262570 *e;
    int v = *slot + a1;

    *slot = v;
    e = &D_L00_00262570[D_00139568[idx]];
    if (e->f8E != 0) {
        int overflow = v - e->f8E;
        if (overflow <= 0) return 0;
        *slot = e->f8E;
        return overflow;
    }
    return 0;
}
