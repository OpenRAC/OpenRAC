/* GPT-6.1 Sol (low), task v0585; NOT MATCHING.
   Current pinned compiler, own -g8.c unit: 968/980 bytes, 0/7 places.
   Frame 0x9F0 versus retail 0xA00. Explicit matrix-translation pointer closes
   half the old frame gap. Argument spills, matrix pointer lifetime, UV-loop
   strength reduction and register allocation still differ. Retail uses separate
   address views at D_001A89E0 (GP) and D_001A89E4 (absolute), stride eight.
   SDATA keeps the unknown table extent truthful; declarations are below.
   Reopen with independent lifetime evidence; retain all four matrix rows.
*/

#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 bits; float f[4]; } UiVector;
typedef struct { UiVector vertices[4]; unsigned int colors[4]; float uv[4][2];
                 u64 primitive, texture, state, flags; } UiDrawPacket;
extern float func_00283CE0(int);
extern unsigned int func_00283D38(int, int, float);
extern u64 func_0027C878(int);
extern void func_002839D0(void *, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00280B90(void *, int, int);
extern int D_001A89D0 __attribute__((sda));
extern int D_001A89D4 __attribute__((sda));
extern int D_001A89D8 __attribute__((sda));
extern float D_001A89E0[] SDATA(D_001A89E0);
extern float D_001A89E4[][2];

void func_00281A08(UiVector *vertices, int vertex_count, UiVector *positions,
                   UiVector *rotations, int phase, int steps,
                   int first_color, int last_color, float blend)
{
    UiDrawPacket packets[16];
    UiVector matrix[4];
    UiVector *translation = &matrix[3];
    int ring, i, step;
    float increment;
    unsigned int color;
    if (steps < 2)
        return;
    ring = 2;
    increment = 1.0f / func_00283CE0(steps - 1);
    blend = increment * blend;
    color = func_00283D38(first_color, last_color, increment);
    for (i = 0; i < (vertex_count >> 1); i++) {
        int j;
        packets[i].texture = func_0027C878(D_001A89D0);
        packets[i].colors[1] = color;
        packets[i].state = 0xFF9000000260UL;
        packets[i].primitive = 5;
        packets[i].flags = ((long)D_001A89D4 << 2) | ((long)D_001A89D8 << 6) | 0x8000000000UL;
        packets[i].colors[0] = color;
        for (j = 3; j >= 0; j--) {
            int index = 3 - j;
            packets[i].uv[index][0] = ((float *)D_001A89E0)[index * 2];
            packets[i].uv[index][1] = D_001A89E4[index][0];
        }
    }
    func_002839D0(&rotations[phase], matrix);
    translation->bits = positions[phase].bits;
    translation->f[3] = 1.0f;
    for (i = 0; i < vertex_count; i++)
        func_002830C0(&packets[i >> 1].vertices[i & 1], vertices + i, matrix);
    for (step = 1; step < steps; step++) {
        unsigned int next_color;
        int next_ring;
        phase += steps;
        phase--;
        phase %= steps;
        func_002839D0(&rotations[phase], matrix);
        translation->bits = positions[phase].bits;
        translation->f[3] = 1.0f;
        next_color = func_00283D38(first_color, last_color, blend);
        blend += increment;
        if (blend > 1.0f)
            blend = 1.0f;
        next_ring = ring + 2;
        for (i = 0; i < vertex_count; i++) {
            int slot = ring + (i & 1);
            func_002830C0(&packets[i >> 1].vertices[slot], vertices + i, matrix);
            packets[i >> 1].colors[slot] = next_color;
        }
        ring = next_ring % 4;
        for (i = 0; i < (vertex_count >> 1); i++)
            func_00280B90(&packets[i], 0, 1);
    }
}
