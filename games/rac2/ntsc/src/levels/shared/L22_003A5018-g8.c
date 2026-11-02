#include "include_asm.h"

typedef unsigned char u8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;
typedef int TI128 __attribute__((mode(TI)));

/* The resident global is an array here on purpose: with a scalar declaration
 * cc1 folds the load and reuses the destination as the address base, while the
 * retail materialises the address in its own register first -- declaring the
 * symbol as an array and indexing it [0] restores the two-instruction form
 * (`lui $v1,%hi; lw $a1,%lo($v1)`). */
extern u32 D_0018C0B0[];

extern int D_L22_001AFA0C __attribute__((sda));

extern int D_L22_001AFA08 __attribute__((sda));

extern float D_L22_001AF3EC __attribute__((sda));

extern float D_L22_001AF5C8 __attribute__((sda));

extern s16 D_0018A16E[];

extern f32 func_00282C48(f32 f);

s32 func_L22_003B9428(u32 a0) {
    u32 g = D_0018C0B0[0];
    int *p = (a0 == g) ? &D_L22_001AFA08 : &D_L22_001AFA0C;
    return *p != 0;
}

s32 func_L22_003A61A8(u8 *a0) {
    u8 *s = *(u8 **)(a0 + 0x68);
    f32 t;
    if (*(f32 *)(s + 0x3DC) < D_L22_001AF3EC) return 1;
    t = func_00282C48(*(f32 *)(s + 0x414) - *(f32 *)(s + 0x410));
    if (D_0018A16E[0] < 0xB) return 0;
    if (*(f32 *)(s + 0x3E0) < D_L22_001AF3EC) {
        if (t < D_L22_001AF5C8) return 1;
    }
    return 0;
}

/* The four resident words sit at the level-22 addresses; in the other program that carries
 * func_L22_003B9358 they are elsewhere, so they take per-program names. */
extern int ref_L22_003B9358_00 __attribute__((sda));   /* the level-22 word D_L22_001AFA08 */

extern int ref_L22_003B9358_01 __attribute__((sda));   /* the level-22 word D_L22_001AFA00 */

extern int ref_L22_003B9358_02 __attribute__((sda));   /* the level-22 word D_L22_001AFA0C */

extern int ref_L22_003B9358_03 __attribute__((sda));   /* the level-22 word D_L22_001AFA04 */

extern void func_002E59F8(int handle);

extern void func_00291450(int index);

extern void func_0029F338_22(char *object);

extern int D_L22_001A8AB0;
extern int D_L22_001A8AC0;

/* Drops the resident slot the argument's record selects: the slot word is released and stamped
 * -1, and the object the slot holds is torn down when its id is 0x5F3 and it has not started
 * (the two resident words it owns are cleared first, and its stored index is released when it is
 * not -1). An object with any other id only clears the slot. */
void func_L22_003B9358(u32 a0)
{
    u32 g = D_0018C0B0[0];
    int *slot;
    char *object;

    if (a0 == g) {
        slot = &ref_L22_003B9358_00;
        if (ref_L22_003B9358_01 != -1) {
            func_002E59F8(ref_L22_003B9358_01);
            ref_L22_003B9358_01 = -1;
        }
    } else {
        slot = &ref_L22_003B9358_02;
        if (ref_L22_003B9358_03 != -1) {
            func_002E59F8(ref_L22_003B9358_03);
            ref_L22_003B9358_03 = -1;
        }
    }
    object = (char *)*slot;
    if (object == 0)
        return;
    if (*(short *)(object + 0xAA) != 0x5F3) {
        *slot = 0;
        return;
    }
    if (*(unsigned char *)(object + 0x20) != 0)
        return;
    {
        char *inner = *(char **)(object + 0x68);
        int index;

        D_L22_001A8AB0 = 0;
        D_L22_001A8AC0 = 0;
        index = *(short *)(inner + 0x2C);
        if (index != -1)
            func_00291450(index);
    }
    func_0029F338_22((char *)*slot);
    *slot = 0;
}

/* func_L22_003F5580 is another function of this unit that reaches a level word through
 * $gp; the scale factor it multiplies is at a different address in the other program that
 * carries the function, so it takes a per-program name.
 */
extern f32 func_00283198(f32);
extern f32 func_00283180(f32);
extern void func_00283098(f32 *, const f32 *, const f32 *);
extern void func_00282CC0(char *, char *, char *);
extern f32 func_00283B98(f32, f32);
extern f32 func_002AA2E8(f32 *, f32, f32);
extern float ref_L22_003F5580_00 __attribute__((sda));

/* Scales the controller of the object: when the controller is idle its position is copied to
 * the object, otherwise the controller's values are scaled and written back.
 */
void func_L22_003F5580(u8 *obj) {
    u8 *ctl = *(u8 **)(obj + 0x68);
    f32 buf[4];
    f32 a, b, c, d, g, r;

    if (*(f32 *)(ctl + 0xBC) == 0.0f) {
        if (*(f32 *)(ctl + 0xC0) == 0.0f)
            return;
        *(TI128 *)(obj + 0xE0) = *(TI128 *)ctl;
        *(f32 *)(ctl + 0xC0) = 0.0f;
        return;
    }
    a = func_00283198(*(f32 *)(ctl + 0xC4));
    b = *(f32 *)(ctl + 0xBC);
    c = func_00283180(*(f32 *)(ctl + 0xB0));
    b = b * a;
    c = c * b;
    buf[0] = c;
    d = func_00283198(*(f32 *)(ctl + 0xB0));
    d = d * b;
    buf[1] = d;
    buf[2] = 0.0f;
    func_00283098(buf, buf, obj + 0xC0);
    buf[3] = 0.0f;
    func_00282CC0(obj + 0xE0, buf, ctl);
    r = func_00283B98(*(f32 *)(ctl + 0xC4), *(f32 *)(ctl + 0xC0));
    g = ref_L22_003F5580_00;
    *(f32 *)(ctl + 0xC4) = r;
    func_002AA2E8((f32 *)(ctl + 0xBC), 0.0f, g * (1.0f / 60.0f));
}

typedef unsigned short u16;

extern void func_002AFFD8(void *, void *, int);

extern f32 func_00282DB0(void *, void *);

extern f32 func_00283CE0(int);

extern f32 func_002AA140(f32, f32, f32);

/* The resident object the point is derived from, the word that picks one of two threshold
   pairs, the four threshold words (read through $gp) and, for the two id lists, the table of
   lists and the base of the 32-byte records each one indexes (read bare: `lui`/`lw` in one
   register). */
extern u8 D_L22_001B9C80[];

extern int D_L22_001A8F00;

extern f32 D_L22_001B0A08 __attribute__((sda));

extern f32 D_L22_001B0A04 __attribute__((sda));

extern f32 D_L22_001B0A00 __attribute__((sda));

extern f32 D_L22_001B09FC __attribute__((sda));

extern u16 **D_L22_001A9014;

extern u16 **D_L22_001A901C;

extern u8 *D_L22_001B2794;

extern u8 *D_L22_001B2678;

/* Switches the object on (+0xCC set) when its position (+0xE0) comes within a threshold of the
   point func_002AFFD8 derives from D_L22_001B9C80 (func_00282DB0 measures it), and off when it
   moves beyond it; the threshold is interpolated between two words by the index at +0x14
   (divided by 14), the pair of words depending on D_L22_001A8F00 == 4. Only a change does
   anything more: switching off clears bit 2 of byte +0x1E of every record of the id list at
   +0x10 and bit 0 of byte +0x16 of every record of the list at +0x16; switching on sets those
   bits (a list index below zero means no list). */
void func_L22_003F58F8(u8 *self) {
    u8 *state = *(u8 **)(self + 0x68);
    f32 v[4];
    f32 dist;
    f32 thr;
    f32 f;
    u16 *p;
    u8 *e;

    func_002AFFD8(D_L22_001B9C80, v, 1);
    dist = func_00282DB0(self + 0xE0, v);
    f = func_00283CE0(*(s16 *)(state + 0x14)) * 0.071428575f;
    if (D_L22_001A8F00 == 4)
        thr = func_002AA140(D_L22_001B0A08, D_L22_001B0A04, f);
    else
        thr = func_002AA140(D_L22_001B0A00, D_L22_001B09FC, f);
    if (*(int *)(state + 0xCC) != 0) {
        if (thr < dist)
            *(int *)(state + 0xCC) = 0;
        if (*(int *)(state + 0xCC) != 0)
            return;
        if (*(int *)(state + 0x10) >= 0) {
            for (p = D_L22_001A9014[*(int *)(state + 0x10)];
                 p != 0 && ((s16)p[-1] >= 0 || p == D_L22_001A9014[*(int *)(state + 0x10)]);
                 p++) {
                e = D_L22_001B2794 + ((*p & 0x7FFF) << 5);
                if (e == 0)
                    break;
                e[0x1E] &= 0xFB;
            }
        }
        if (*(s16 *)(state + 0x16) >= 0) {
            for (p = D_L22_001A901C[*(s16 *)(state + 0x16)];
                 p != 0 && ((s16)p[-1] >= 0 || p == D_L22_001A901C[*(s16 *)(state + 0x16)]);
                 p++) {
                e = D_L22_001B2678 + ((*p & 0x7FFF) << 5);
                if (e == 0)
                    break;
                e[0x16] &= 0xFE;
            }
        }
    } else {
        if (dist < thr)
            *(int *)(state + 0xCC) = 1;
        if (*(int *)(state + 0xCC) == 0)
            return;
        if (*(int *)(state + 0x10) >= 0) {
            for (p = D_L22_001A9014[*(int *)(state + 0x10)];
                 p != 0 && ((s16)p[-1] >= 0 || p == D_L22_001A9014[*(int *)(state + 0x10)]);
                 p++) {
                e = D_L22_001B2794 + ((*p & 0x7FFF) << 5);
                if (e == 0)
                    break;
                e[0x1E] |= 4;
            }
        }
        if (*(s16 *)(state + 0x16) >= 0) {
            for (p = D_L22_001A901C[*(s16 *)(state + 0x16)];
                 p != 0 && ((s16)p[-1] >= 0 || p == D_L22_001A901C[*(s16 *)(state + 0x16)]);
                 p++) {
                e = D_L22_001B2678 + ((*p & 0x7FFF) << 5);
                if (e == 0)
                    break;
                e[0x16] |= 1;
            }
        }
    }
}
