/* NON_MATCHING func_L16_002E6D40 -- src/overlays/shared/vendor_002A1B58.c
 * Best so far: BYTES 12/1012 (98.8% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   p9 BYTES28/1012: typed private yaw/phase/cardinal-angle fields compile identically.
 *   p10 BYTES33/1012: delaying color-base cursor changes saved-register allocation despite fixing its setup order;
 *   p11 SIZE1004/1012: explicit final array-pointer transform alias coalesces matrix register and removes retail s
 *   p12 BYTES18/1012: indexed cardinal-angle loop fixes saved-angle base and final transform setup through inducti
 *   p13 BYTES18/1012: row-typed source UV/position cursors normalize identically; retained as clean array-based ca
 *   p14 BYTES14/1012: initialize position before color and UV cursors without changing declaration order; fixes fi
 *   p15 SIZE1020/1012: indexed UV source arrays introduce extra cursor-base setup and rearrange GS initialization;
 *   Final: complete plain C renderer, all operations/registers/addresses match except two independent setup permut
 */
typedef struct {
    float position[4][4];
    int color[4];
    float uv[4][2];
    unsigned long flags, texture, giftag, primitive;
} RingQuad;
typedef struct { char pad00[8]; float yaw, phase; float cardinal_angles[4]; } RingData;
extern long func_001F4868(int);
extern void func_001F9C30(void *, void *, float);
extern void func_001FA1F8(void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern float D_L16_001D9A50[4][2], D_L16_001D9A70[4][2];
extern float D_L16_001D9A90[4][4], D_L16_001D9AD0[4][4];
extern short D_L16_00161ED8, D_L16_00161EDC, D_L16_00161EE0;
extern short D_L16_00161EE8, D_L16_00161EEC, D_L16_00161EF0, D_L16_00161EF4;
extern short D_L16_00161EF8, D_L16_00161EFC, D_L16_00161F00;

/* Draw the animated ring quads and four rotating accents around a moby. */
void func_L16_002E6D40(char *m) {
    RingQuad quad;
    float transform[4][4], rotation[4];
    RingData *data = *(RingData **)(m + 0x78);
    float scale = *(float *)(m + 0x2C) / *(float *)(*(char **)(m + 0x24) + 0x24);
    float *u, *v;
    float *angles, *position;
    int *color_base;
    float phase, angle_step, phase_step;
    int i;
    qcopy(rotation, m + 0x40);
    qcopy(transform[3], m + 0x10);
    quad.texture = func_001F4868(*(int *)&D_L16_00161EF8);
    u = &quad.uv[0][0];
    v = &quad.uv[0][1];
    quad.giftag = 0xFF9000000260UL;
    quad.primitive = (unsigned long)*(int *)&D_L16_00161EE8
        | ((unsigned long)*(int *)&D_L16_00161EEC << 2)
        | ((unsigned long)*(int *)&D_L16_00161EF0 << 4)
        | ((unsigned long)*(int *)&D_L16_00161EF4 << 6)
        | 0x8000000000UL;
    quad.flags = 0;
    color_base = quad.color;
    {
        float (*source_uv)[2];
        float (*source_position)[4];
        int *color;
        float *next_v, *next_u;
        int count;
        source_position = D_L16_001D9A90;
        source_uv = D_L16_001D9A50;
        position = quad.position[0];
        color = color_base;
        next_v = v;
        next_u = u;
        for (count = 3; count >= 0; --count) {
            *next_u = source_uv[0][0];
            *next_v = source_uv[0][1];
            *color = *(int *)&D_L16_00161EDC;
            qcopy(position, source_position[0]);
            func_001F9C30(position, position, scale);
            ++source_position;
            position += 4;
            ++color;
            next_v += 2;
            next_u += 2;
            ++source_uv;
        }
    }
    angles = data->cardinal_angles;
    angle_step = 6.2831855f / func_001FA888(*(int *)&D_L16_00161F00);
    phase_step = 3.0f / func_001FA888(*(int *)&D_L16_00161F00);
    phase = data->phase;
    while (phase < 0.0f) phase += 1.0f;
    while (phase > 1.0f) phase -= 1.0f;
    for (i = 0; i < *(int *)&D_L16_00161F00;) {
        float blend;
        int color, j;
        rotation[0] = func_001FA748(data->yaw, (float)i * angle_step);
        func_001FA1F8(transform, rotation);
        if (phase > 0.5f) blend = (-phase + 1.0f) + (-phase + 1.0f);
        else blend = phase + phase;
        ++i;
        color = func_001FA8A8(*(int *)&D_L16_00161ED8, *(int *)&D_L16_00161EDC, blend);
        for (j = 1; j >= 0; --j) color_base[j] = color;
        phase += phase_step;
        if (phase > 1.0f) phase -= 1.0f;
        func_L00_001FD1D8(&quad, transform, 0);
    }
    quad.texture = func_001F4868(*(int *)&D_L16_00161EFC);
    {
        float *source_uv = D_L16_001D9A70[0];
        float *source_position = D_L16_001D9AD0[0];
        int *color = color_base;
        float *next_position = quad.position[0];
        float *next_v = v, *next_u = u;
        int count;
        int solid_color = *(int *)&D_L16_00161EE0;
        for (count = 3; count >= 0; --count) {
            *next_u = source_uv[0];
            *next_v = source_uv[1];
            *color = solid_color;
            qcopy(next_position, source_position);
            source_position += 4;
            next_position += 4;
            ++color;
            next_v += 2;
            next_u += 2;
            source_uv += 2;
        }
    }
    {
        int count;
        for (count = 3; count >= 0; --count) {
            rotation[0] = angles[3 - count];
            func_001FA1F8(transform, rotation);
            func_L00_001FD1D8(&quad, transform, 0);
        }
    }
}
