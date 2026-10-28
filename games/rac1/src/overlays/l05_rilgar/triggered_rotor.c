/* Starts at 0x00317B68 (was vendor_00317B68.c). */
/* Kept in a separate translation unit: this rotor reads the pool pointer with full addressing,
 * while vendor_0030EB68.c also has small-data declarations of that symbol. */
#include "common.h"
#include "moby.h"
extern float FastSubRots(float, float);
extern float FastAddRots(float, float);

typedef struct {
    char pad[0xBC];
    unsigned char flag;
    char pad2[0x43];
} RotorPoolMoby17B68;
extern RotorPoolMoby17B68 *moby_pool_4f60_pool __asm__("D_L05_00160098") MACRO_ADDR;
extern float g_frame_time_sq MACRO_ADDR;
extern float g_frame_time MACRO_ADDR;

typedef struct {
    int flag;
    float velocity;
    int target;
    float angle;
} RotorData17B68;

extern int Moby_PlaySound_i(int, int, void *) __asm__("func_0022ED80");
extern float ease_rotation_317B68(float *, float, float *, float, float, float) __asm__("func_L00_0025CE58");
/* Updates a triggered rotor: initialize its angle, wait for the target, then ease its rotation. */
void update_triggered_moby_yaw(Moby *moby) {
    RotorData17B68 *p = (RotorData17B68 *)moby->pvars;
    float f;
    switch (moby->state) {
    case 0:
        if (p->flag != 0) {
            moby->flags |= 0x8000;
        }
        moby->state = 1;
        p->angle = moby->rot.z;
        if (p->flag != 0) {
            moby->rot.z = FastSubRots(moby->rot.z, 1.5707964f);
        } else {
            moby->rot.z = FastAddRots(moby->rot.z, 1.5707964f);
        }
        /* fallthrough */
    case 1:
        if (moby_pool_4f60_pool[p->target].flag != 0) {
            Moby_PlaySound_i(0, 0, moby);
            moby->state = 2;
        }
        break;
    case 2:
        if (p->flag != 0) {
            f = FastSubRots(p->angle, 0.6981317f);
        } else {
            f = FastAddRots(p->angle, 0.6981317f);
        }
        ease_rotation_317B68(&moby->rot.z, f, &p->velocity, g_frame_time_sq * 12.566371f, g_frame_time_sq * 12.566371f, g_frame_time * 12.566371f);
        break;
    }
}
