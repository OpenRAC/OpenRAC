/* GPT-6.1 Sol high, 29 October 2026; retained near miss.
 * USA v1.01: 1104/1104 bytes, 0/27 places; nine checker runs plus regalloc.
 * Profile: -O2 -G8 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier
 *          -fno-force-mem -fno-rerun-loop-opt.
 * Initializing all three fragment constants immediately after the direction
 * snapshot and disabling the second loop pass retains retail's 2.0f/f23,
 * 0.05f/f21 and 0.5f/f20 hoists without also hoisting the epsilon threshold.
 * The baseline had no 0.5f hoist. First difference is now +0xE4: the spill,
 * constant setup and vector-pointer setup interleave differently. Fragment/tail
 * GPR allocation, store order, branch setup and second-phase hero-base formation
 * also differ. Both loops and all RNG calls, seven complete vectors, and the
 * unconditional drift clear are preserved. Jitter W is not newly initialized.
 * Separating the phase angles changed allocation without improving it; explicit
 * constants with both loop passes enabled grew to 1108 bytes and hoisted epsilon.
 * Next: actual fragment/tail pointer and hero-base lifetimes. All declarations
 * are below. This task checked only USA v1.01; earlier release claims are not
 * measurements of this revised source. No linked build was run for this miss.
 */
#include "common.h"
typedef int HeroQ __attribute__((mode(TI)));
typedef union { unsigned char b[0x2600]; signed char c[0x2600]; short h[0x1300]; unsigned short uh[0x1300]; int i[0x980]; unsigned int u[0x980]; float f[0x980]; char *ptr[0x980]; HeroQ q[0x260]; } Hero;
extern unsigned char D_00189E20[];
#define H ((Hero *)D_00189E20)
typedef int VectorBits __attribute__((mode(TI)));
typedef float Vector[4] __attribute__((aligned(16)));


extern float D_L00_001AF62C __attribute__((sda));
extern float D_L00_001AF630 __attribute__((sda));
extern int D_L00_001AF634 __attribute__((sda));
extern int D_L00_001AF638 __attribute__((sda));
extern unsigned int D_L00_001AF63C __attribute__((sda));
extern float D_L00_001AF640 __attribute__((sda));
extern float D_L00_001AF644 __attribute__((sda));
extern float D_L00_001AF648 __attribute__((sda));
extern float D_L00_001AF64C __attribute__((sda));
extern float D_L00_001AF650 __attribute__((sda));
extern float D_L00_001AF654 __attribute__((sda));
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_002ACEC0(void *, void *, void *, float);
extern float func_002A7878(float, float);
extern float func_00283B98(float, float);
extern void func_00282D30(void *, void *, float);
extern char *func_L00_00333F90(void *, void *);
extern float func_00282C48(float);
extern void func_002AB668(void *, int, float);
extern void func_002ACDE8(void *, void *, void *);
extern float func_002A78D8(float, float);
extern int func_00283CF0(float);
extern void func_00282CF0(void *, void *, void *);
extern char *func_002C16C0(void *, void *, unsigned int, int, int, int, float, float, float, float, float);

/* Emit a rotating fan of fragments followed by smoke particles. */
void func_L00_003A4480(char *moby)
{
    Vector rotation, direction, base, origin, velocity, jitter, drift;
    float angle = 0.0f;
    int count;
    float fragmentDuration, fragmentRate, fragmentScale;
    *(VectorBits *)base = 0;
    base[0] = 0.3f;
    base[1] = -1.0f;
    base[3] = 1.0f;
    func_00283098(base, base, moby + 0xC0);
    func_00282CC0(origin, moby + 0x10, base);
    *(unsigned char *)(moby + 0xBC) = 6;
    func_002ACEC0(base, (&H->f[460]), (&H->f[464]), 0.34906590f);
    qcopy(direction, base);
    fragmentDuration = 2.0f;
    fragmentRate = 0.05f;
    fragmentScale = 0.5f;
    for (count = 19; count >= 0; count--) {
        char *fragment;
        angle = func_00283B98(angle, func_002A7878(0.0f, 0.6283186078f));
        func_00282D30(velocity, direction, func_002A7878(0.1000000089f, 0.3f));
        fragment = func_L00_00333F90(origin, velocity);
        if (fragment) {
            char *tail = fragment + 0x30;
            func_00282D30(velocity, direction, 0.75f);
            func_00282CC0(fragment + 0x10, origin, velocity);
            qcopy((fragment + 0x20), origin);
            *(float *)(tail + 0xC) = fragmentDuration;
            *(int *)(fragment + 0xC) = 0x307F7F7F;
            *(float *)(fragment + 0x1C) = fragmentRate;
            *(float *)(fragment + 0x2C) = fragmentScale;
            *(int *)(fragment + 4) = 0x307F7F7F;
        }
        if (func_00282C48(angle) > 0.00001f) {
            func_002AB668(rotation, (int)(&H->f[460]), angle);
            func_002ACDE8(direction, base, rotation);
        }
    }
    angle = 0.0f;
    func_002ACEC0(base, (&H->f[460]), (&H->f[464]), D_L00_001AF62C * 0.017453294f);
    qcopy(direction, base);
    qzero(drift);
    if (H->i[208])
        func_00282D30(drift, (&H->f[68]), 1.7f);
    for (count = 19; count >= 0; count--) {
        unsigned int color;
        int spin;
        float size;
        angle = func_00283B98(angle, func_002A7878(0.0f, 0.6283186078f));
        func_00282D30(velocity, direction, func_002A7878(D_L00_001AF644 * (1.0f / 60.0f),
                                                       D_L00_001AF648 * (1.0f / 60.0f)));
        func_00282CC0(velocity, velocity, drift);
        color = D_L00_001AF63C;
        spin = func_00283CF0(func_002A78D8(1.0f, 2.0f));
        jitter[0] = origin[0] + func_002A78D8(0.0f, D_L00_001AF630);
        jitter[1] = origin[1] + func_002A78D8(0.0f, D_L00_001AF630);
        jitter[2] = origin[2] + func_002A78D8(0.0f, D_L00_001AF630);
        size = func_002A7878(D_L00_001AF64C, D_L00_001AF650) * 210000.0f;
        func_00282CF0(jitter, jitter, velocity);
        func_002C16C0(jitter, velocity, color, D_L00_001AF634, D_L00_001AF638, spin,
                      size, D_L00_001AF654, D_L00_001AF640, 0.0f, 0.0f);
        if (func_00282C48(angle) > 0.00001f) {
            func_002AB668(rotation, (int)(&H->f[460]), angle);
            func_002ACDE8(direction, base, rotation);
        }
    }
}
