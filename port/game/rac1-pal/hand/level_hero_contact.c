/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native PAL nearby surface lookup, recovered from the candidate for
 * func_L00_0020D3A0 and its complete 0x24C retail body. Not a PS2 match.
 * Pointer fields/results stay pointers so hostgen emits guest addresses
 * and the retail 32-byte table stride. Filters retain the caller's integer
 * ABI and compare the 32-bit address bits. */
typedef struct {
    float position[3];
    float radius;
    int *surface;
    int index;
    int reserved[2];
} HeroContactEntry;
extern HeroContactEntry *D_L00_0015F7EC MACRO_ADDR;
extern int D_L00_0015F7F0 MACRO_ADDR;
extern unsigned char D_0013F450[];
extern float func_001F9D10(void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_001F9B88(float);
extern int func_L00_0025EFC0(void *, void *, void *, void *, void *, int,
                            float, float, float);

int func_L00_0020D3A0(void *position_, void *surface_, void *point_,
                     void *segment_, void *fraction_, void *index_,
                     int exclude, int require) {
    float *position = position_, *point = point_, *out_fraction = fraction_;
    int **out_surface = surface_;
    int *out_segment = segment_, *out_index = index_;
    float origin[4];
    int segment;
    float fraction;
    for (int j = 0; j < 4; ++j) origin[j] = position[j];
    for (int i = 0; i < D_L00_0015F7F0; ++i) {
        unsigned address = (unsigned)(unsigned long long)D_L00_0015F7EC[i].surface;
        if (exclude && address == (unsigned)exclude) continue;
        if (require && address != (unsigned)require) continue;
        if (*D_L00_0015F7EC[i].surface == 0) continue;
        if (func_001F9D10(&D_L00_0015F7EC[i], origin) > D_L00_0015F7EC[i].radius)
            continue;
        if (func_L00_0025EFC0(D_L00_0015F7EC[i].surface, origin, point,
                              &segment, &fraction, D_L00_0015F7EC[i].index,
                              12.0f, 10.0f, 0.0f)) {
            float planar_limit = 0.9f;
            float height_limit = 1.5f;
            if (*(unsigned int *)(D_0013F450 + 0x208C) < 2)
                planar_limit = 0.3f;
            if (*(short *)(D_0013F450 + 0x1CA) != 0) {
                height_limit = 10.0f;
                planar_limit += 0.5f;
            }
            float planar = func_001F9D48(origin, point);
            float height = func_001F9B88(origin[2] - point[2]);
            if (planar < planar_limit && height < height_limit) {
                *out_surface = D_L00_0015F7EC[i].surface;
                *out_segment = segment;
                *out_fraction = fraction;
                *out_index = D_L00_0015F7EC[i].index;
                return 1;
            }
        }
    }
    return 0;
}
