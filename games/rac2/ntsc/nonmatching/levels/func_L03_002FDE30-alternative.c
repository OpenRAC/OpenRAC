/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L03_002FDE30 (L03, 608 bytes): NOT matched. This compiles to 612 bytes in the default -G0
 * profile of src/levels/03_endako/L03_002CCB70.c; the first half (entity loop, E29/D54/FA0 tests,
 * the mode-3 branch, $gp/absolute mix for D_001A79F0 and the SDATA store) is instruction-identical
 * except register numbers. Remaining differences: in the entity loop retail keeps the index in s1
 * and the element pointer in s2 (mine swaps them, which shifts the prologue save order); in the
 * 4x3 call loop retail keeps i in $v1 with i+1 in s3 and i*3 in s2 (mine keeps i in s2), and
 * the final copy loop gets the retail-style qcopy IVs only partly. Tried: pointer-walk loop,
 * i++ hoisted before the inner loop (strength reduction then changes the shape), local
 * declaration order. The 'e != 0' test before E29 only (D54 test has no null check) is what
 * reproduces retail's un-threaded branches. */
#include "common.h"

typedef struct {
    u8 pad00[0x30];
    s32 field30;
    s32 field34;
    u8 pad38[0xC];
    s16 count;
    u8 pad46[0x146];
    u8 *list[1];
} L03_DE30Block;

extern L03_DE30Block D_L03_001BD680;
typedef struct {
    u8 v[3][0x10];
} L03_DE30Row;

extern L03_DE30Row D_L03_001C64A0[4];
extern s32 D_001A79F0 __attribute__((sda));
extern f32 D_L03_001A9420 __attribute__((sda));
extern s32 D_L03_001A9424 SDATA(D_L03_001A9424);

extern void func_2FDBC8(void);
extern void func_3614E0(u8 *e);
extern void func_326410(u8 *e, u8 *p, u32 a, u32 b, f32 f);
extern void func_326458(u8 *e);
extern s32 func_312898(u8 *a, s32 b, void *c);

void func_L03_002FDE30(void)
{
    s32 i;
    s32 j;
    s32 k;

    func_2FDBC8();
    for (i = 0; i < D_L03_001BD680.count; i++) {
        u8 *e = D_L03_001BD680.list[i];

        if (e != 0 && *(s16 *)(e + 0xAA) == 0xE29) {
            *(u16 *)(e + 0x34) |= 0x800;
        }
        if (*(s16 *)(e + 0xAA) == 0xD54) {
            func_3614E0(e);
        }
        if (e != 0 && *(s16 *)(e + 0xAA) == 0xFA0 && *(u8 **)(e + 0x68) != 0) {
            func_326410(e, *(u8 **)(e + 0x68) + 0xC, 0x80AAAAD2, 0x80396464, 0.06108653545379639f);
        }
    }
    if (D_001A79F0 == 3) {
        if ((u32)D_L03_001BD680.field30 < 3) {
            u8 *p = *(u8 **)((u8 *)&D_L03_001BD680 + 0x190);

            *(f32 *)(p + 0x2C) = *(f32 *)(*(u8 **)(p + 0x24) + 0x24) * D_L03_001A9420;
        }
        if (D_L03_001BD680.field30 == 6) {
            func_326458(*(u8 **)((u8 *)&D_L03_001BD680 + 0x190));
        }
        if (D_L03_001BD680.field30 == 8) {
            func_326458(*(u8 **)((u8 *)&D_L03_001BD680 + 0x198));
        }
        if (D_L03_001BD680.field30 == 7 && D_L03_001BD680.field34 == 0x424) {
            f32 buf[8] __attribute__((aligned(16)));
            u8 *obj;

            D_L03_001A9424 = D_L03_001BD680.field34;
            obj = *(u8 **)((u8 *)&D_L03_001BD680 + 0x194);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 3; j++) {
                    func_312898(obj, i * 3 + j, D_L03_001C64A0[i].v[j]);
                }
            }
            for (k = 0; k < 4; k++) {
                qcopy(buf, D_L03_001C64A0[k].v[0]);
                qcopy(buf + 4, D_L03_001C64A0[k].v[1]);
                qcopy(buf, D_L03_001C64A0[k].v[0]);
                qcopy(buf + 4, D_L03_001C64A0[k].v[2]);
            }
        }
    }
}
