/* GPT-6.1 Sol high, task v1118, 2026-10-29: NOT MATCHED.
 * Current pinned compiler: -O2 -G8 -Wa,-G32 -ffunction-sections.
 * USA v1.01 check.py: 1424/1420 bytes, 0/1 places; frame 0x2B0.
 * Independent field bases for the two real 144-byte packets reproduce the
 * retail outer-invariant bases (+0x40, +0x50, +0x54). qcopy_nc on the collision
 * endpoint transfer restores the branch and instruction order through +0x1BC;
 * intervening opaque calls separate the following reads of that local vector.
 * Remaining: spill slot order, inner cursor/index register lifetimes, GP U vs
 * absolute V sources, packet store order and particle-loop allocation.
 * Retained complete bone/packet/request/UV storage and existing call contracts.
 * Explicit layer-color/next-index/second-packet variables regressed allocation;
 * an indexed packet form added a position-W stream and grew the frame to 0x2C0.
 * -G32 instead of -G8 did not repair the mixed UV forms. No flag sweep or linked
 * build was run. Reopen with independent source/field-lifetime evidence.
 * Declarations required are included below.
 */
#include "common.h"
typedef int BeamQuadword __attribute__((mode(TI)));
typedef struct { int word[4]; } BeamDamageData __attribute__((aligned(16)));
typedef struct {
    float position[4][4];
    u32 color[4];
    float uv[4][2];
    unsigned long packet[4];
} BeamBatch __attribute__((aligned(16)));
extern float D_L16_001B16D0[4] __attribute__((sda));
extern float D_L16_001B17F8[2] __attribute__((sda));
extern int D_L16_001B17E8[2] __attribute__((sda));
extern float D_L16_001B1800[4][2] MACRO_ADDR;
extern u32 D_L16_001B1820[2] __attribute__((sda));
extern int D_L16_001B1828 __attribute__((sda));
extern u32 D_L16_001B182C __attribute__((sda));
extern float D_L16_001B1830 __attribute__((sda));
extern float D_L16_001B1834, D_L16_001B1838, D_L16_001B183C;
extern int D_L16_001B1840 __attribute__((sda));
extern int D_L16_001B1844;
extern float D_L16_001B1848 __attribute__((sda));
extern float D_L16_001B184C;
extern float D_L16_001B9B00[4], D_L16_001BFF60[4];
extern BeamDamageData D_L16_001A8F20;
extern int D_L16_001B2648;
extern void func_0029FC20(void *, int, void *), func_002830C0(void *, void *, void *);
extern float func_L16_00416EB8(char *);
extern void func_00282CF0(void *, void *, void *), func_00282CC0(void *, void *, void *);
extern void func_00282F70(void *, void *, float), func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *), func_00282D30(void *, void *, float);
extern void func_002A8C00(char *, int, int, float, BeamDamageData *);
extern int func_002761F8(void *, void *, int, void *, void *);
extern u32 func_00283D38(u32, u32, float);
extern unsigned long func_0027C878(int);
extern void func_00280B90(void *, int, int);
extern int func_002A7820(int, int), func_00283CF0(float);
extern float func_002A78D8(float, float), func_002A7878(float, float);
extern void func_00282D08(void *, void *, void *, float);
/* Particle calls consume six/three GPR inputs and five/one FP inputs.
 * Both return their allocated pointer or null; this caller ignores those results. */
extern void *func_002C16C0(void *, void *, int, int, int, int, float, float, float, float, float);
extern void *func_002C6DD8(void *, void *, int, float);
extern void func_002A7A00(void *, float, float);

void func_L16_00412560(u8 *moby)
{
    BeamBatch batch[2];
    float bone[16] __attribute__((aligned(16)));
    float side[4] __attribute__((aligned(16)));
    float camera[4] __attribute__((aligned(16)));
    float ray[4] __attribute__((aligned(16)));
    float start[4] __attribute__((aligned(16)));
    float end[4] __attribute__((aligned(16)));
    float target[4] __attribute__((aligned(16)));
    char effect[48] __attribute__((aligned(16)));
    float delta[4] __attribute__((aligned(16)));
    float velocity[4] __attribute__((aligned(16)));
    float sample[4] __attribute__((aligned(16)));
    u8 *state = *(u8 **)(moby + 0x68);
    u8 *object;
    int i, j, texture, seed;
    float size, speed;
    func_0029FC20(*(void **)(state + 0x1f0), 2, bone);
    func_002830C0(start, D_L16_001B16D0, bone);
    object = *(u8 **)(state + 0x24c);
    if (object) {
        qcopy(target, (object + 0x10));
        target[2] = func_L16_00416EB8((char *)object);
    } else qcopy(target, (state + 0x210));
    func_00282CF0(end, moby + 0x10, target);
    end[2] = -0.1f;
    func_00282F70(end, end, *(float *)(state + 0x250));
    func_00282CC0(end, end, target);
    func_002A8C00(effect, (int)moby, 0x90001, (float)state[0x36], &D_L16_001A8F20);
    if (func_002761F8(start, end, 0, moby, effect))
        qcopy_nc(end, D_L16_001BFF60);
    func_00282CF0(camera, D_L16_001B9B00, start);
    i = 0;
    func_00282CF0(ray, end, start);
    func_00282DD8(side, ray, camera);
    func_00282F20(side, side, 1.0f);
    {
        u32 *color_base = batch[0].color;
        float *u_base = &batch[0].uv[0][0];
        float *v_base = &batch[0].uv[0][1];
        for (; i < 2; i++) {
            float *u = (float *)(i * sizeof(BeamBatch) + (u8 *)u_base);
            float *v = (float *)(i * sizeof(BeamBatch) + (u8 *)v_base);
            float *source_u = &D_L16_001B1800[0][0];
            float *source_v = &D_L16_001B1800[0][1];
            u32 *color = (u32 *)(i * sizeof(BeamBatch) + (u8 *)color_base);
            float (*vertex)[4] = batch[i].position;
            for (j = 0; j < 4; j++) {
                *color = func_00283D38(D_L16_001B1820[i] & 0xff000000,
                    D_L16_001B1820[i], *(float *)(state + 0x254));
                *u = *source_u;
                *v = *source_v;
                if (j & 1)
                    func_00282D30(*vertex, side, D_L16_001B17F8[i]);
                else func_00282D30(*vertex, side, -D_L16_001B17F8[i]);
                if (j < 2) func_00282CC0(*vertex, *vertex, start);
                else func_00282CC0(*vertex, *vertex, end);
                (*vertex)[3] = 1.0f;
                ++vertex; ++color; u += 2; v += 2; source_u += 2; source_v += 2;
            }
            batch[i].packet[1] = func_0027C878(D_L16_001B17E8[i]);
            batch[i].packet[3] = 0x8000000048UL;
            batch[i].packet[2] = 0xff9000000260UL;
            batch[i].packet[0] = 0;
        }
    }
    func_00280B90(&batch[0], 0, 1);
    func_00280B90(&batch[1], 0, 1);
    if (*(int *)(state + 0x258) != D_L16_001B2648) {
        *(int *)(state + 0x258) = D_L16_001B2648;
        func_00282CF0(delta, end, start);
        for (i = 0; i < D_L16_001B1828; i++) {
            texture = func_002A7820(D_L16_001B1840, D_L16_001B1844);
            seed = func_00283CF0(func_002A78D8(1.0f, 2.0f));
            size = func_002A7878(D_L16_001B1848, D_L16_001B184C);
            speed = func_002A7878(D_L16_001B1838, D_L16_001B183C);
            func_00282F20(velocity, delta, speed * 0.016666668f);
            func_00282D08(sample, end, start, func_002A7878(0.0f, 1.0f));
            func_002C16C0(sample, velocity, texture, (int)D_L16_001B182C >> 24,
                D_L16_001B182C & 0xffffff, seed, size * 210000.0f,
                500.0f, 1.0f, 0.0f, 1.0f);
            size = func_002A7878(10.0f, 20.0f);
            func_002C6DD8(sample, velocity, texture, size * 0.00027777778f);
            func_002A7A00(velocity, D_L16_001B1830 * 0.016666668f,
                D_L16_001B1834 * 0.016666668f);
            velocity[2] += func_002A7878(D_L16_001B1838, D_L16_001B183C) * 0.016666668f;
            func_002C6DD8(end, velocity, texture, 0.0040555559f);
        }
    }
}
