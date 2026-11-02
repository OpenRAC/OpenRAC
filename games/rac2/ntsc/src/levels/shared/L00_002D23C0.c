#include "include_asm.h"
typedef unsigned char u8;
typedef struct { u8 pad[0x10]; u8 count; } Sub_2D23C0;
typedef struct { u8 pad[0x48]; Sub_2D23C0 *items[1]; } Tab_2D23C0;
typedef struct {
    u8 pad0[0x24];
    Tab_2D23C0 *tab;
    u8 pad1[0x40 - 0x28];
    u8 b40, b41, b42, b43;
    int w44;
    u8 pad2[0x58 - 0x48];
    int w58;
    int w5C;
} Obj_2D23C0;
extern Obj_2D23C0 *D_0018C0B0;
extern void func_0029F390(void *);
extern int func_L00_002A4DB0(void *a0, int a1, int a2, char *a3);
extern char D_001B4400[];

void func_L00_002D23C0(int a0, int a1)
{
    Obj_2D23C0 *o = D_0018C0B0;
    int t;
    o->b41 = a1 + 1;
    o->b40 = a1;
    t = o->tab->items[a0]->count - 1;
    if (t < o->b41)
        o->b41 = t;
    o->w44 = 0;
    o->b42 = a0;
    o->b43 = a0;
    func_0029F390(o);
    o->w58 = func_L00_002A4DB0(o, o->b42, o->b40, D_001B4400);
    o->w5C = func_L00_002A4DB0(o, o->b43, o->b41, D_001B4400 + 0x600);
}
