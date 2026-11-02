/* Opus-1 (v1496), 29 October 2026: GNU game -O2 -G0 -ffunction-sections.
 * US, US2 and PAL: 1016/1008 bytes, 0/2 places each.
 * The `for (i = 0; i < count; i++)` form (i otherwise unused) gives retail's loop:
 * the guard tests the incoming $t1, loop.c's reversed counter lives in $s6 and is
 * decremented in the final call's delay slot, and a/b/c land in $s5/$s4/$s3. The older
 * do/while draft put count in $s3 and shifted every saved register.
 * One cause remains (two instructions): `flags` (spilled to 0x4C($sp)) is stored
 * twice. sched1's split_block_insns splits `flags = 0x1003C` into lui/ori on the
 * pseudo, so reload stores after the lui (+0x8) and after the ori; the in-out reload
 * of the ori names flags twice, so delete_output_reload keeps the first store. Retail
 * has lui, ori and one `sw $v0,0x4C($sp)` (the blez delay slot). The extra word then
 * misaligns the loop head and its .p2align 3 adds the nop before the first jal.
 * Passing 0x1003C directly is rematerialized in the loop (980 bytes); `flags = 0x3C;
 * flags |= 0x10000`, a short helper local and an inline helper returning the constant
 * all fold back to the same split. Reopen with a source form that keeps flags' set
 * from being a large-constant move at sched1 (retail evidently copied it from a
 * register); func_L05_00398A38 shows retail's split-with-two-stores form for comparison.
 */
#include "common.h"

extern float func_002A7878(float, float);
extern void func_00283098(void *, void *, void *);
extern void func_L00_00324F18(float, float, float, void *, void *);
extern int func_00283CF0(float);
extern int func_00283D38(float, int, int);
extern void *func_002BAA28(void *, void *, void *, int, int, int, int, int, int);
extern void func_00282C88(void *);
extern void func_00282CC0(void *, void *, void *);

typedef struct { float x, y, z, w; } SparkVector;
void func_L03_002ECF08(unsigned char *object, float *minimumPosition, float *maximumPosition,
                      float *minimumVelocity, float *maximumVelocity, int count, unsigned char style) {
    SparkVector velocity, acceleration, position;
    int flags = 0x1003C;
    float scale;
    float gravity = 0.0013888889f;
    int a, b, c, startColor, endColor;
    int i;

    for (i = 0; i < count; i++) {
        velocity.x = func_002A7878(minimumVelocity[0], maximumVelocity[0]) * 0.016666668f;
        velocity.y = func_002A7878(minimumVelocity[1], maximumVelocity[1]) * 0.016666668f;
        velocity.z = func_002A7878(minimumVelocity[2], maximumVelocity[2]) * 0.016666668f;
        position.x = func_002A7878(minimumPosition[0], maximumPosition[0]);
        position.y = func_002A7878(minimumPosition[1], maximumPosition[1]);
        position.z = func_002A7878(minimumPosition[2], maximumPosition[2]);
        func_00283098(&velocity, &velocity, object + 0xC0);
        func_L00_00324F18(position.x, position.y, position.z, object, &position);
        scale = func_002A7878(0.5f, 1.5f);
        a = func_00283CF0(scale * 10.0f);
        b = func_00283CF0(scale * 35.0f);
        c = func_00283CF0(scale * 7.0f);
        velocity.w = func_002A7878(0.1f, 0.4f);
        func_00282C88(&acceleration);
        acceleration.z = -gravity * (float)(a + b + c);
        func_00282CC0(&acceleration, &acceleration, &velocity);
        acceleration.w = 0.01f;
        switch (style) {
        case 1:
            startColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x80102040, 0x801010A0);
            endColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x102040, 0x1010A0);
            break;
        case 2:
            startColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x80104010, 0x8020A020);
            endColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x104010, 0x20A020);
            break;
        case 3:
            startColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x80201608, 0x80402510);
            endColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x201608, 0x402510);
            break;
        case 0:
        default:
            startColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x80004080, 0x80408080);
            endColor = func_00283D38(func_002A7878(0.0f, 1.0f), 0x4080, 0x408080);
            break;
        }
        func_002BAA28(&position, &velocity, &acceleration, startColor, endColor, a, b, c, flags);
    }
}
