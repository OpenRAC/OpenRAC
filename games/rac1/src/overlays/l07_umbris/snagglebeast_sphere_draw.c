/* Starts at 0x00313D28 (was effect_00313D28.c). */
/* Level 07 code: func_L07_00313D28 in a file of its own, built with
 * -mno-split-addresses (config/file_cflags.txt). The functions of
 * vendor_00313D28.c do not build with it (11 change), so it was another
 * object in retail (docs/BUILD_FIDELITY.md, rule 2). */
#include "common.h"
#include "callees.h"
#include "vector.h"
#include "moby.h"

/* Snagglebeast sphere effect: only the draw state is read here. */
struct SnaggleSphereVars {
    u8 pad_00[0x1C6];
    u8 brightness;                  /* 0x1C6 */
};

extern void VU1_addGSregister(int, u64);
extern u64 GetEffectTex(int);
extern float IntToFloat(int);
extern int truncate_float_to_s32(float);
extern void Moby_GetJointWorldPos(void *, int, void *);
extern float split_float_fraction(void *, float);
extern float normalize_angle(float);
extern void BuildSphericalOffset(void *, float, float, float);
extern float FastAddRots(float, float);
extern void FastVecAdd(void *, void *, void *);
extern void draw_world_quad(void *, int, int);
extern s32 depth_buffer_address;
extern s32 umbris_frame_count;
typedef struct {
    float pos[4][4];
    u32 colors[4];
    float uv[4][2];
    u64 reserved;
    u64 texture;
    u64 texture_state;
    u64 primitive;
} FlareQuad_h6 __attribute__((aligned(16)));

/* Returns the color with each RGB channel given at least bit 0x40 set. */
static inline int fx_tint(int col) {
    int k = col | 0x404040;
    return k;
}

// Umbris effect: builds GS packets for a flare strip from the moby's data and draws its segments
void draw_snagglebeast_sphere_quad_strips(char *moby, int arg2, float fa, float fb, float fc, float fd, float fe) {
    struct SnaggleSphereVars *data = (struct SnaggleSphereVars *)((Moby *)moby)->pvars;
    int c = data->brightness;
    int col = (c << 24) | (c << 16) | (c << 8) | c;
    u64 ts = 0xFF9000000260L;
    u64 prim = 0xA000000029L;
    FlareQuad_h6 quad;
    float v90[4] __attribute__((aligned(16)));
    float vA0[4] __attribute__((aligned(16)));
    float vB0;
    u64 texture;
    int color;
    float da, alpha, spin, step, ustep, angle, next, wrapped, theta, ph;

    VU1_addGSregister(0x4E, (u64)(depth_buffer_address >> 13) | 0x1000000L | (0x8000L << 17));
    texture = GetEffectTex(0x3C);
    ph = fd;
    color = truncate_float_to_s32(IntToFloat(0) * ph) << 24;
    color |= fx_tint(col);
    quad.colors[3] = color;
    quad.colors[2] = color;
    quad.colors[1] = color;
    quad.colors[0] = color;
    quad.texture = texture;
    quad.texture_state = ts;
    quad.primitive = prim;
    quad.reserved = 0;
    spin = IntToFloat(umbris_frame_count + arg2) * fa;
    Moby_GetJointWorldPos(moby, 9, v90);
    qcopy(vA0, v90);
    vA0[2] = vA0[2] - (fb - 0.8f);
    quad.uv[0][0] = split_float_fraction(&vB0, spin * -0.01f);
    quad.uv[2][0] = quad.uv[0][0];
    quad.uv[1][0] = quad.uv[0][0] + 1.0f;
    quad.uv[3][0] = quad.uv[1][0];
    quad.uv[0][1] = split_float_fraction(&vB0, spin * -0.02f);
    quad.uv[1][1] = quad.uv[0][1];
    quad.uv[2][1] = quad.uv[0][1] + 1.0f;
    quad.uv[3][1] = quad.uv[2][1];
    step = (fe - ph) * 0.0625f;
    ustep = fc * 0.0625f;
    da = 0.3926991f;
    for (angle = -2.7488937f; angle < 3.1415927f; angle = next) {
        qcopy(&quad.pos[2], vA0);
        qcopy(&quad.pos[3], vA0);
        next = angle + da;
        alpha = IntToFloat(0);
        ph = fd;
        color = truncate_float_to_s32(alpha * ph) << 24;
        color |= fx_tint(col);
        quad.colors[3] = color;
        quad.colors[2] = color;
        for (theta = ustep + -1.5707964f; theta <= fc - 1.5707964f; theta += ustep) {
            qcopy(&quad.pos[0], &quad.pos[2]);
            qcopy(&quad.pos[1], &quad.pos[3]);
            ph += step;
            quad.colors[0] = quad.colors[2];
            quad.colors[1] = quad.colors[3];
            color = truncate_float_to_s32(IntToFloat(0) * ph) << 24;
            color |= fx_tint(col);
            quad.colors[3] = color;
            quad.colors[2] = color;
            wrapped = normalize_angle(angle + spin * 0.02f);
            BuildSphericalOffset(&quad.pos[2], fb, wrapped, theta);
            BuildSphericalOffset(&quad.pos[3], fb, FastAddRots(wrapped, da), theta);
            if (0.0f < theta) {
                quad.pos[2][2] = quad.pos[2][2] + 0.6f;
                quad.pos[3][2] = quad.pos[3][2] + 0.6f;
            } else {
                quad.pos[2][2] = quad.pos[2][2] - 0.375f;
                quad.pos[3][2] = quad.pos[3][2] - 0.375f;
            }
            FastVecAdd(&quad.pos[2], &quad.pos[2], v90);
            FastVecAdd(&quad.pos[3], &quad.pos[3], v90);
            draw_world_quad(&quad, 0, 1);
        }
    }
    VU1_addGSregister(0x4E, (u64)(depth_buffer_address >> 13) | 0x1000000L);
}
