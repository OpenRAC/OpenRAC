/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* The release's item menu update (rac2-decomp's func_US2_003446B8, src/us2/game/00344110-g82.c),
   in the release's names. The decompilation types it void, but its last call's result stays in
   $v0 and its callers (func_0029D408, func_L00_0030E8F0) return it, as the primary's
   item_menu_update does: so it returns that result. Its file's declarations are this C's. */
int func_003446B8(char *self, int arg)
{
    f32_F7036ef03 *v;
    char *sub = self + 712;

    v = *(f32_F7036ef03 **)(self + 696);
    func_00336C40(self, D_L04_001AE480 + v[0], D_L04_001AE484 + v[1], 0.0f, 0.0f);
    v = *(f32_F7036ef03 **)(self + 696);
    func_00336C40(self + 76, D_L04_001AE488 + v[0], D_L04_001AE48C + v[1], 0.0f, 0.0f);
    v = *(f32_F7036ef03 **)(self + 696);
    func_00336C40(self + 152, D_L04_001AE490 + v[0], D_L04_001AE494 + v[1], 0.0f, 0.0f);
    v = *(f32_F7036ef03 **)(self + 696);
    func_00336C40(self + 608, D_L04_001AE498 + v[0], D_L04_001AE49C + v[1], 0.0f, 0.0f);
    v = *(f32_F7036ef03 **)(self + 696);
    func_00343F50(sub, D_L04_001AE4A0 + v[0], D_L04_001AE4A4 + v[1]);
    func_00344558(self);
    func_003444E8(self);
    return *(int *)(self + 416) = func_00343888(sub, arg);
}
