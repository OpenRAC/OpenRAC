/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_hero_attach.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(HeroAttachSlot) == 0x50, "EE slot stride");
_Static_assert(sizeof(HeroAttachDefinition) == 0x4C, "EE item stride");
_Static_assert(offsetof(HeroAttachSlot, detached) == 0x2A, "EE detached");
_Static_assert(offsetof(HeroAttachSlot, item) == 0x48, "EE item");
_Static_assert(offsetof(HeroAttachState, matrices) == 0xAC0, "EE matrices");
_Static_assert(offsetof(HeroAttachState, slots) == 0x1070, "EE slots");
_Static_assert(offsetof(HeroAttachState, slots[3].auxiliary) == 0x118C, "EE auxiliary");
_Static_assert(offsetof(HeroAttachState, auxiliary_point) == 0x1D80, "EE auxiliary point");
_Static_assert(offsetof(HeroAttachState, hero) == 0x2080, "EE hero");
_Static_assert(offsetof(HeroAttachMoby, model) == 0x24, "EE model");
_Static_assert(offsetof(HeroAttachMoby, flags) == 0x34, "EE flags");
_Static_assert(offsetof(HeroAttachMoby, lighting) == 0x38, "EE lighting");
_Static_assert(offsetof(HeroAttachMoby, joints) == 0x68, "EE joints");
_Static_assert(offsetof(HeroAttachMoby, color) == 0x90, "EE color");
_Static_assert(offsetof(HeroAttachMoby, class_id) == 0xA6, "EE class");
_Static_assert(offsetof(HeroAttachMoby, basis) == 0xC0, "EE basis");
#endif
unsigned char ha_state[0x5000], ha_settings[4];
int ha_mode, ha_euler, ha_clock;
HeroAttachDefinition ha_definitions[8];
unsigned char ha_glove[16], ha_glove_out[16], ha_head[16], ha_head_other[16], ha_head_out[16];
float ha_boot_save[48], ha_boot_scale[240];
unsigned char ha_boot_left[16], ha_boot_right[16], ha_boot_left_out[16], ha_boot_right_out[16];
static HeroAttachState* g;
static HeroAttachMoby hero, models[7][2], glow;
static int advanced, updated, normalized, bounded, eulers, weapons, crossed, poses, spawned, joints, auxiliary;
static int glove_slot, head_slot, boot_slot, fail_spawn, delete_glow, disable_glow, clear_euler;
static int ticks, trapped;
static float wave_value, phase_value;
static void *pose_source[16], *pose_output[16];
static int pose_start[16];

void ha_angles(void* from, void* to) { ++eulers; memcpy(to, from, 16); }
void ha_advance(void* p) { CHECK(p); ++advanced; if (clear_euler) ha_euler = 0; }
void ha_update(void* p) {
    CHECK(p); ++updated;
    if (p == &glow) {
        if (delete_glow) g->slots[3].auxiliary = NULL;
        if (disable_glow) glow.flags |= 1;
    }
}
int ha_is_glove(int i) { return i == glove_slot; }
int ha_is_head(int i) { return i == head_slot; }
int ha_is_boot(int i) { return i == boot_slot; }
void ha_copy_basis(void* to, void* from) { memcpy(to, from, 48); }
void ha_normalize(void* p) { CHECK(p); ++normalized; }
void ha_rotate(void* out, void* in, void* matrix) {
    float* v = in;
    CHECK(out == in && matrix && v[0] == 1 && v[1] == 0 && v[2] == 0);
    v[0] = 3; v[1] = 4; v[2] = 12;
}
float ha_length(void* p) { CHECK(((float*)p)[2] == 12); return 5; }
float ha_angle(float x, float y) { return x + 2 * y; }
void ha_bounds(void* p) { CHECK(p); ++bounded; }
void ha_pose(void* from, void* out, void* model, int first, int zero) {
    CHECK(model == &hero && zero == 0 && poses < 16);
    pose_source[poses] = from; pose_output[poses] = out; pose_start[poses++] = first;
    if (from == ha_boot_left || from == ha_boot_right) {
        CHECK(ha_boot_scale[0x398 / 4] == 1 && ha_boot_scale[0x2E0 / 4] == 1);
        CHECK(ha_boot_scale[0x2E4 / 4] == 1 && ha_boot_scale[0x2E8 / 4] == 1);
        CHECK(ha_boot_scale[0x390 / 4] == 1 && ha_boot_scale[0x394 / 4] == 1);
        memset(ha_boot_save, 0, 16); memset(ha_boot_save + 0xB0 / 4, 0, 16);
    }
}
void ha_weapon(void* m, void* matrix) { CHECK(m && matrix); ++weapons; }
void ha_cross(void* z, void* x, void* y) {
    CHECK((float*)y == (float*)x + 4 && (float*)z == (float*)x + 8); ++crossed;
}
HeroAttachMoby* ha_spawn(int id) { CHECK(id == 0x4B4); ++spawned; return fail_spawn ? NULL : &glow; }
void ha_joint(void* m, int index, void* matrix) {
    int k;
    CHECK(m == &models[3][1] && index == 6); ++joints;
    /* Writes the entire matrix, including the last row the old candidate overflowed. */
    for (k = 0; k < 16; ++k) ((float*)matrix)[k] = (float)(100 + k);
}
void ha_auxiliary(void) { CHECK(g->auxiliary_point[0] == 112 && g->auxiliary_point[3] == 115); ++auxiliary; }
int ha_ticks(int n) { CHECK(n == 120); return ticks; }
float ha_float(int n) { CHECK(n == ticks); return (float)n; }
float ha_sin(float x) { phase_value = x; return wave_value; }
int ha_truncate(float f) { return (int)f; }
void ha_trap(void) { ++trapped; }

static void reset(void) {
    int i, j, k;
    memset(ha_state, 0, sizeof(ha_state)); memset(models, 0, sizeof(models));
    memset(&glow, 0, sizeof(glow)); memset(ha_definitions, 0, sizeof(ha_definitions));
    memset(ha_boot_save, 0, sizeof(ha_boot_save)); memset(ha_boot_scale, 0, sizeof(ha_boot_scale));
    g = (HeroAttachState*)(ha_state + 0xE1D); g->hero = &hero;
    hero.lighting[0] = 0x12345678; hero.lighting[1] = 0xABCDEF01;
    for (i = 0; i < 7; ++i) {
        g->slots[i].item = i; ha_definitions[i].matrix = i;
        for (j = 0; j < 2; ++j) {
            models[i][j].model = &hero; models[i][j].flags = 0x100;
            models[i][j].frame = 5; models[i][j].next_frame = 6;
        }
        for (j = 0; j < 4; ++j) for (k = 0; k < 4; ++k)
            g->matrices[i][j][k] = (float)(i * 100 + j * 10 + k);
    }
    glow.scale = 2;
    for (k = 0; k < 4; ++k) { g->position[k] = (float)k; g->angles[k] = (float)(k + 10); }
    ha_mode = ha_euler = ha_clock = ha_settings[1] = 0;
    advanced = updated = normalized = bounded = eulers = weapons = crossed = poses = spawned = joints = auxiliary = 0;
    glove_slot = head_slot = boot_slot = -1;
    fail_spawn = delete_glow = disable_glow = clear_euler = trapped = 0;
    ticks = 120; wave_value = phase_value = 0;
}
static void ordinary_and_modes(void) {
    int mode, i;
    for (mode = 0; mode < 3; ++mode) {
        reset(); ha_mode = mode == 1 ? 2 : mode == 2 ? 6 : 0;
        for (i = 0; i < 7; ++i) g->slots[i].primary = &models[i][0];
        func_L00_0020FC18();
        CHECK(advanced == (mode ? 2 : 7) && normalized == advanced && bounded == advanced);
        for (i = 0; i < 7; ++i) {
            if (mode && i != 4 && i != 5) { CHECK(models[i][0].flags == 0x100); continue; }
            CHECK(models[i][0].flags == 0x106 && models[i][0].lighting[1] == hero.lighting[1]);
            CHECK(models[i][0].position[3] == g->matrices[i][3][3]);
            CHECK(models[i][0].angles[1] == -29 && models[i][0].angles[2] == 11);
        }
    }
    reset(); g->slots[0].secondary = &models[0][1];
    g->slots[1].secondary = &models[1][1];
    func_L00_0020FC18(); CHECK(advanced == 1 && models[1][1].position[0] == 330);
}
static void detached_and_euler(void) {
    reset(); g->slots[2].primary = &models[2][0]; g->slots[2].detached = 1;
    models[2][0].flags = 0x107;
    func_L00_0020FC18();
    CHECK(g->slots[2].point[3] == 233 && g->slots[2].angles[0] == 200);
    CHECK(models[2][0].position[0] == 0 && models[2][0].flags == 0x103);
    CHECK(advanced == 1 && updated == 1 && bounded == 0 && eulers == 1);
    reset(); g->slots[0].primary = &models[0][0]; ha_euler = ha_definitions[0].euler = 1;
    ha_settings[1] = 1; func_L00_0020FC18();
    CHECK(weapons == 1 && crossed == 1 && normalized == 0 && models[0][0].flags == 0x106);
    clear_euler = 1; ha_euler = 1; func_L00_0020FC18(); CHECK(eulers == 1 && weapons == 1);
}
static void special_items(void) {
    int k;
    reset(); g->slots[0].primary = &models[0][0]; glove_slot = head_slot = boot_slot = 0;
    func_L00_0020FC18();
    CHECK(advanced == 0 && normalized == 0 && pose_source[0] == ha_glove && pose_start[0] == 0);
    CHECK(models[0][0].frame == 0 && models[0][0].next_joints == ha_glove_out);
    reset(); g->slots[1].primary = &models[1][0]; head_slot = 1;
    models[1][0].class_id = 0x1B1; func_L00_0020FC18();
    CHECK(pose_source[0] == ha_head && pose_start[0] == 6);
    models[1][0].class_id = 0; func_L00_0020FC18();
    CHECK(pose_source[1] == ha_head_other && pose_start[1] == 0);
    reset(); boot_slot = 4; g->slots[4].primary = &models[4][0]; g->slots[4].secondary = &models[4][1];
    for (k = 0; k < 48; ++k) ha_boot_save[k] = (float)(k + 1);
    func_L00_0020FC18();
    CHECK(poses == 2 && pose_source[0] == ha_boot_left && pose_source[1] == ha_boot_right);
    CHECK(models[4][0].joints == ha_boot_left_out && models[4][1].joints == ha_boot_right_out);
    for (k = 0; k < 48; ++k) CHECK(ha_boot_save[k] == (float)(k + 1));
}
static void glow_model(void) {
    int k;
    reset(); g->slots[3].secondary = &models[3][1]; fail_spawn = 1;
    func_L00_0020FC18(); CHECK(spawned == 1 && !g->slots[3].auxiliary && joints == 0);
    reset(); g->slots[3].secondary = &models[3][1]; delete_glow = 1;
    func_L00_0020FC18(); CHECK(!g->slots[3].auxiliary && joints == 0);
    reset(); g->slots[3].secondary = &models[3][1]; disable_glow = 1;
    func_L00_0020FC18(); CHECK(joints == 0 && glow.position[3] == 3 && glow.angles[3] == 13);
    reset(); g->slots[3].secondary = &models[3][1];
    memset(g->pad12A0, 0x5A, sizeof(g->pad12A0)); memset(g->pad1D90, 0xA5, sizeof(g->pad1D90));
    ha_clock = 60; wave_value = 1; func_L00_0020FC18();
    CHECK(spawned == 1 && joints == 1 && auxiliary == 1 && phase_value == 0);
    CHECK(glow.scale == 2.6f && glow.distance == 0x40 && glow.draw == 1 && glow.state == 0);
    CHECK(glow.color == 0x801E64FFu && glow.flags == 0x10 && glow.lighting[1] == hero.lighting[1]);
    for (k = 0; k < 4; ++k) CHECK(glow.position[k] == (float)(112 + k));
    CHECK(glow.basis[2][3] == 111);
    for (k = 0; k < (int)sizeof(g->pad12A0); ++k) CHECK(g->pad12A0[k] == 0x5A);
    for (k = 0; k < (int)sizeof(g->pad1D90); ++k) CHECK(g->pad1D90[k] == 0xA5);
    wave_value = -1; func_L00_0020FC18(); CHECK(spawned == 1 && glow.color == 0x800A007Du);
    ticks = 0; func_L00_0020FC18(); CHECK(trapped == 1);
}
int main(void) {
    ordinary_and_modes(); detached_and_euler(); special_items(); glow_model();
    puts("hero attachment: OK"); return 0;
}
