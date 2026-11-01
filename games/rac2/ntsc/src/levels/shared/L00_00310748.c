#include "include_asm.h"
typedef int Q128 __attribute__((mode(TI)));

typedef struct {
    int f0;
    int count;
} Hdr_310748;

typedef struct {
    char pad0[4];
    float f4;
    char pad8[0x10 - 8];
    float v10[4];
} Ent_310748;

typedef struct {
    char pad0[0x10];
    float v10[4];
    char pad20[0x2C - 0x20];
    float f2C;
    char pad30[0x98 - 0x30];
    Hdr_310748 *hdr;
    char pad9C[0xC0 - 0x9C];
    float mC0[4];
} Obj_310748;

extern void func_00283698(void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);

void func_L00_00310748(Obj_310748 *self, int idx, void *p, float x, float y)
{
    float m[16];
    float v[4];
    char *h = (char *)self->hdr;
    float k;

    if (idx < (((Hdr_310748 *)h)->count >> 5)) {
        k = 1024.0f / self->f2C;
        h += idx * 32 + 0x10;
        func_00283698(m, self->mC0);
        func_00282CF0(v, p, self->v10);
        func_002830C0(v, v, m);
        func_00282D30(v, v, k);
        *(Q128 *)((Ent_310748 *)h)->v10 = *(Q128 *)v;
        ((Ent_310748 *)h)->f4 = y * k;
        ((Ent_310748 *)h)->v10[3] = x * k;
    }
}
