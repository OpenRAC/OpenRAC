#include "common.h"
typedef int Quad __attribute__((mode(TI)));
/* Partial resident state, through the last byte read here. */
typedef struct {
    u8 unknown0000[0x2290];
    u8 *active;
    u8 unknown2294[0x20];
    u8 mode;
} ResidentState;
extern ResidentState D_00189E20;
extern u8 D_00189EA0[];
extern void *D_L00_001B8E14;
extern void func_00282F20(void *, void *, float);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282DD8(void *, void *, void *);

/* Builds a normalized direction and its mode-dependent scale, then updates the
 * owner's basis and keeps a full aligned copy of its resulting first row. */
void func_L00_00426560(u8 *owner)
{
    Quad direction;
    Quad scaled;
    u8 mode;
    void *basis;
    func_00282F20(&direction, D_00189E20.active + 0xE0, 1.0f);
    mode = D_00189E20.mode;
    if (mode == 2)
        func_00282D30(&scaled, &direction, 9.5f);
    else if (mode == 1)
        func_00282D30(&scaled, &direction, 0.75f);
    else
        func_00282D30(&scaled, &direction, 1.6f);
    func_00282CC0(owner + 0x30, D_00189EA0, &scaled);
    basis = D_L00_001B8E14;
    qcopy(owner + 0x20, &direction);
    func_00282DD8(owner + 0x10, basis, &direction);
    func_00282F20(owner + 0x10, owner + 0x10, 1.0f);
    func_00282DD8(owner, &direction, owner + 0x10);
    qcopy(owner + 0x40, owner);
}
