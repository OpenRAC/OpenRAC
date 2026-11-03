/* GPT-6.1 Sol (high), USA v1.01, 2026-10-29: retained near miss.
 * tools/check.py: 884/884 bytes, 0 of 2 places match; first mismatch +0x74.
 * Profile: -O2 -G8 -ffunction-sections -mno-split-addresses.
 * The 0xA0 frame and five saved FPRs agree. Relative to the previous draft,
 * an inner-loop widthStep initializer recovers retail's 0.0009/gravity constant
 * order, and indexed points remove the premature cursor setup. Canonical
 * catalogue callee names are retained; declarations are included below.
 * Remaining: the floor-global base folds into an absolute lwc1 instead of
 * lui/addiu plus lwc1 +0x318; loop preheader/pointer-increment scheduling,
 * growth-condition branches, and callback argument setup also differ.
 * Plain/typed global views, sda/nosda, and a context local did not fix the
 * folded floor access or callback schedule. Explicit age/velocity cursors
 * grew this to 892 bytes. Reopen with a real address/lifetime hypothesis.
 */
#include "common.h"
/* Eight-sample trail records; the retail loop visits 140 entries of stride 0x150. */
typedef struct Trail416018 {
    float points[8][4];
    float velocities[8][4];
    float origin[4];
    float *offset, *impulse;
    short ages[8], landed[8];
    int count, lifetime;
    u32 colour;
    int active;
    char *owner;
    int tail;
} Trail416018;
extern Trail416018 D_L04_00254E10[];
extern unsigned char D_00189E20[];
extern int D_L04_001B08F0 __attribute__((sda));
extern void *D_L04_001B0914 __attribute__((sda));
extern void func_00282CC0(float *, float *, float *);
extern void func_00282D30(float *, float *, float);
extern void func_00283098(float *, float *, char *);
extern int func_00282948(int *);
extern u32 func_00283D38(float, u32, u32);
typedef void (*TrailDrawCallback416018)(void *);
extern void func_L04_00415E28(void *);
extern void func_L00_002E6CF0(TrailDrawCallback416018, void *);
void func_L04_00416018(void)
{
    float local[4] __attribute__((aligned(16)));
    float floorZ, scale;
    Trail416018 *trail;
    float *point, *velocity;
    float gravity;
    short *age;
    int i, j, count, last;
    char *owner;
    for (i = 0; i < 140; i++) {
        trail = &D_L04_00254E10[i];
        if (!trail->active)
            continue;
        owner = trail->owner;
        count = trail->count;
        if (*(short *)(owner + 0xaa) == 0xbbc)
            floorZ = *(float *)(D_00189E20 + 0x318) + 0.1f;
        else
            floorZ = *(float *)(owner + 0x18) - 0.25f;
        for (j = 0; j < count; j++) {
            float widthStep = 0.0009f;
            gravity = 0.0019444445f;
            point = trail->points[j];
            velocity = trail->velocities[j];
            age = &trail->ages[j];
            if ((*age)++ > 10)
                ((float *)((char *)point + 0x80))[2] -= gravity;
            scale = 1.0f;
            scale -= (*age - 5) * 0.01f;
            func_00282CC0(point, point, velocity);
            func_00282D30(local, trail->impulse, scale);
            func_00282CC0(point, point, local);
            if (point[2] < floorZ) {
                point[2] = floorZ;
                trail->landed[j] = 1;
            }
            point[3] = trail->ages[0] * widthStep + 0.01f + *age * widthStep;
        }
        last = count - 1;
        if (count < 8 &&
            ((*(short *)(trail->owner + 0xaa) == 0xbbc && D_L04_001B08F0) ||
             (*(short *)(trail->owner + 0xaa) == 0xb2d &&
              *(int *)(*(char **)(trail->owner + 0x68) + 0x244)))) {
            if (trail->ages[last] < 3)
                continue;
            point = trail->points[count];
            func_00283098(local, trail->origin, trail->owner + 0xc0);
            func_00282CC0(point, trail->offset, local);
            func_00282CC0(point, point, trail->impulse);
            qcopy(trail->velocities[count], local);
            point[3] = 0.005f;
            trail->ages[count] = 0;
            trail->count++;
        } else if (trail->landed[last] == 1) {
            if (func_00282948(&trail->lifetime)) {
                trail->active = 0;
            } else {
                trail->colour = func_00283D38(trail->lifetime * 0.033333335f,
                    0x00858585, 0x80858585);
            }
        }
    }
    func_L00_002E6CF0(func_L04_00415E28, D_L04_001B0914);
}
