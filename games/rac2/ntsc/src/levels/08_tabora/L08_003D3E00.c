#include "include_asm.h"
#include "common.h"

extern void func_002B0A68(void *, int, void *, int, float, float, float, float);
extern void func_0029F338(void *);

typedef union {
    int q __attribute__((mode(TI)));
    float f[4];
} Quad __attribute__((aligned(16)));

void func_L08_003D3E00(unsigned char *a0) {
    int v1 = a0[0x20];
    Quad local;

    switch (v1) {
    case 0:
        a0[0x20] = 1;
        break;
    case 1:
        if (a0[0xBC] != 0)
            a0[0x20] = 2;
        break;
    case 2:
        qzero(&local);
        local.f[2] = 0.15f;
        func_002B0A68(a0, 0x7F, &local, 0xFF, 3.0f, 6.0f, 3.0f, 6.0f);
        func_0029F338(a0);
        break;
    }
}
