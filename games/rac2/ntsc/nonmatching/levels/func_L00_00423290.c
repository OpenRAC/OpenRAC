/* func_L00_00423290 (360 bytes) - near miss (W1, 28 October 2026): 360/360 bytes, 4 lines off at us level 0
 * (was 356/360, 74 lines). Profile: -O2 -G0 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier
 * -mno-split-addresses.
 * What closed the size: the loaded object pointer is one variable that gets +0x60 added in place
 * (`v60 = *(char **)(a0 + 0x70); v160 = v60 + 0x160; obj = v60; v60 += 0x60;`). Retail keeps the
 * loaded pointer in $s1, copies it to $s2 for the later +0xB0/+0xC0/+0xF0 uses and adds 0x60 in
 * place, which a second variable for the same value never produced.
 * Left: the order of the four setup instructions before the first jal. Retail:
 * daddu $s2,$s1 ; addiu $s4,$s1,0x160 ; daddu $a1,$s0 ; addiu $s1,$s1,0x60 ; jal ; daddu $a0,$sp in the
 * slot. Here the in-place add ends in the slot and `daddu $a0,$sp` is before it. All orders of the
 * five head statements and the float position of func_00282D30 tie.
 */
#include "include_asm.h"
typedef int HeroQ __attribute__((mode(TI)));
typedef union { unsigned char b[0x2600]; signed char c[0x2600]; short h[0x1300]; unsigned short uh[0x1300]; int i[0x980]; unsigned int u[0x980]; float f[0x980]; char *ptr[0x980]; HeroQ q[0x260]; } Hero;
extern unsigned char D_00189E20[];
#define H ((Hero *)D_00189E20)


#include "include_asm.h"

#ifndef RAC2_T_V4_F0FBA48D3
#define RAC2_T_V4_F0FBA48D3
typedef int V4_F0fba48d3 __attribute__((mode(TI)));
#endif

extern void func_00282CF0_2(void *b, void *c, void *d);

extern void func_00282CC0(void *c, void *c2, void *s);

extern void func_00282D30(void *dst, void *src, float k);

extern float func_00282DB0_1(char *local, char *p);

extern void func_00282C88(void *a);





void func_L00_00423290(char *a0)
{
    char *obj;
    char *v60;
    char *vB0;
    char *vF0;
    char *v160;
    char *hero;
    char *pos;
    V4_F0fba48d3 *dst;
    char buf0[16];
    char buf1[16];

    if (*(short *)(a0 + 0x86) != 0)
        return;

    hero = (&H->c[704]);
    v60 = *(char **)(a0 + 0x70);
    v160 = v60 + 0x160;
    obj = v60;
    v60 += 0x60;

    func_00282D30(buf0, hero, -1.0f);
    /* The hero's position, 0x240 below the facing direction the call above uses. */
    hero = hero - 0x240;
    *(V4_F0fba48d3 *)v60 = *(V4_F0fba48d3 *)hero;
    dst = (V4_F0fba48d3 *)(obj + 0xC0);
    *dst = *(V4_F0fba48d3 *)v60;
    vB0 = obj + 0xB0;
    vF0 = obj + 0xF0;
    func_00282D30(obj + 0x70, buf0, func_00282DB0_1(v60, buf0));
    func_00282C88(obj + 0xD0);
    func_00282C88(obj + 0xE0);
    func_00282C88(obj + 0x100);
    func_00282D30(buf1, buf0, *(float *)(v160 + 0x30));
    func_00282CC0(vB0, buf1, v60);
    func_00282D30(buf1, buf0, *(float *)(v60 + 0xB0));
    func_00282CC0(vF0, buf1, v60);
    dst = (V4_F0fba48d3 *)(obj + 0xA0);
    *dst = *(V4_F0fba48d3 *)vF0;
    func_00282CF0_2(v160, a0 + 0x30, vB0);
    dst = (V4_F0fba48d3 *)(obj + 0x170);
    *dst = *(V4_F0fba48d3 *)v160;
    *(V4_F0fba48d3 *)obj = *(V4_F0fba48d3 *)(a0 + 0x30);
    func_00282CC0(obj + 0x210, vB0, v160);
    *(int *)(obj + 0x240) = 0;
}
