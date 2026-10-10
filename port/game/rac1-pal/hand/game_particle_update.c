/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native-only PAL handwritten 00218A80..00218B08, including the loop at
 * 00218AA8. gp=166D00: -6B54 pool, -6B4C high, -6B30 cursor, -6B2C end.
 * C keeps its return address on the host stack; particle state remains in
 * the retail globals. No PS2 matching claim. */
typedef struct NativeParticleUpdate {
    signed char type, active;
    unsigned char payload[62];
} NativeParticleUpdate;
extern NativeParticleUpdate* pu_pool __asm__("D_001601AC");
extern int pu_high __asm__("D_001601B4");
extern NativeParticleUpdate* pu_cursor __asm__("D_001601D0");
extern NativeParticleUpdate* pu_end __asm__("D_001601D4");
extern void (*pu_callbacks[])(NativeParticleUpdate*) __asm__("D_001CE100");

void func_00218A80(void) {
    NativeParticleUpdate* next = pu_pool;
    NativeParticleUpdate* end = next + (pu_high + 1);
    pu_end = end;
    while (next != end) {
        NativeParticleUpdate* particle = next++;
        if (particle->active < 0) continue;
        pu_cursor = next;
        pu_callbacks[particle->type](particle);
        /* Retail reloads both after dispatch. The callbacks may remove,
         * add or redirect work through these globals. Do not hoist them. */
        next = pu_cursor;
        end = pu_end;
    }
}
