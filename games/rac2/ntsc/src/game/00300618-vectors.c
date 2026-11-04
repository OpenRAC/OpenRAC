#include "common.h"
typedef unsigned int Quad __attribute__((mode(TI)));
typedef union { Quad bits; float f[4]; } Vector;
extern char *func_002FEC50(int);
extern void func_00282DD8(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_002AB600(void *, void *);
extern void func_0029FCC0(void *, int, void *);
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);



void func_00305310(char *owner, Quad *input)
{
    Vector local[7];
    char *transform = func_002FEC50(*(int *)(*(char **)(owner + 0x68) + 0x48));
    qcopy(&local[3], input);
    qcopy(&local[0], &local[3]);
    local[4].bits = 0;
    local[4].f[2] = 1.0f;
    func_00282DD8(&local[1], &local[0], &local[4]);
    func_00282F20(&local[1], &local[1], 1.0f);
    func_00282DD8(&local[2], &local[1], &local[0]);
    func_002AB600(transform + 0x10, &local[0]);
    func_0029FCC0(owner, 0, &local[5]);
    func_00283098(&local[3], input, owner + 0xC0);
    func_00282CC0(&local[6], &local[3], &local[5]);
}

void func_00307AD0(char *owner, Quad *input)
{
    Vector local[7];
    char *transform = func_002FEC50(*(int *)(*(char **)(owner + 0x68) + 0x48));
    qcopy(&local[3], input);
    qcopy(&local[0], &local[3]);
    local[4].bits = 0;
    local[4].f[2] = 1.0f;
    func_00282DD8(&local[1], &local[0], &local[4]);
    func_00282F20(&local[1], &local[1], 1.0f);
    func_00282DD8(&local[2], &local[1], &local[0]);
    func_002AB600(transform + 0x10, &local[0]);
    func_0029FCC0(owner, 0, &local[5]);
    func_00283098(&local[3], input, owner + 0xC0);
    func_00282CC0(&local[6], &local[3], &local[5]);
}
