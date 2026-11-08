/* Generated from sdk/bindings/rac2.json by sdk/tools/gen_binding.py. Do not edit. */

/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The script mods' side of the game's hook points, their names and its fields (docs/port/MODS.md,
 * sections 4 and 11; openrac/script_binding.h). */
#include <stddef.h>
#include <stdint.h>

#include "openrac/script_binding.h"

static const openrac_script_point k_points[7];

/* Shape S0: void(i32) (amount). */
typedef void (*S0_fn)(int32_t);
static const uint8_t k_S0_kinds[] = {OPENRAC_SV_I32};
static const char* const k_S0_names[] = {"amount"};
static void S0_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S0_fn)original)((int32_t)v[0].i);
}
#define S0_THUNK(N, I, K) static void N##_##K(int32_t amount) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    v[0].i = (int64_t)amount; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S0_fn)k_##N##_links[K].original)(amount); \
        return; \
    } \
}
#define S0_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S0_THUNK(N, I, 0) S0_THUNK(N, I, 1) S0_THUNK(N, I, 2) S0_THUNK(N, I, 3) S0_THUNK(N, I, 4) S0_THUNK(N, I, 5) S0_THUNK(N, I, 6) S0_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S1: u32(u32,u32,i32) (moby, mask, keep). */
typedef uint32_t (*S1_fn)(uint32_t, uint32_t, int32_t);
static const uint8_t k_S1_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_U32, OPENRAC_SV_I32};
static const char* const k_S1_names[] = {"moby", "mask", "keep"};
static void S1_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S1_fn)original)((uint32_t)v[0].i, (uint32_t)v[1].i, (int32_t)v[2].i);
}
#define S1_THUNK(N, I, K) static uint32_t N##_##K(uint32_t moby, uint32_t mask, int32_t keep) { \
    openrac_script_value v[3] = {{0, 0.0}}; \
    v[0].i = (int64_t)moby; \
    v[1].i = (int64_t)mask; \
    v[2].i = (int64_t)keep; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S1_fn)k_##N##_links[K].original)(moby, mask, keep); \
    } \
    return (uint32_t)r.i; \
}
#define S1_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S1_THUNK(N, I, 0) S1_THUNK(N, I, 1) S1_THUNK(N, I, 2) S1_THUNK(N, I, 3) S1_THUNK(N, I, 4) S1_THUNK(N, I, 5) S1_THUNK(N, I, 6) S1_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S2: i32(i32,i32) (item, amount). */
typedef int32_t (*S2_fn)(int32_t, int32_t);
static const uint8_t k_S2_kinds[] = {OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S2_names[] = {"item", "amount"};
static void S2_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S2_fn)original)((int32_t)v[0].i, (int32_t)v[1].i);
}
#define S2_THUNK(N, I, K) static int32_t N##_##K(int32_t item, int32_t amount) { \
    openrac_script_value v[2] = {{0, 0.0}}; \
    v[0].i = (int64_t)item; \
    v[1].i = (int64_t)amount; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S2_fn)k_##N##_links[K].original)(item, amount); \
    } \
    return (int32_t)r.i; \
}
#define S2_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S2_THUNK(N, I, 0) S2_THUNK(N, I, 1) S2_THUNK(N, I, 2) S2_THUNK(N, I, 3) S2_THUNK(N, I, 4) S2_THUNK(N, I, 5) S2_THUNK(N, I, 6) S2_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S3: void(u32,i32,i32) (moby, flags, arg). */
typedef void (*S3_fn)(uint32_t, int32_t, int32_t);
static const uint8_t k_S3_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S3_names[] = {"moby", "flags", "arg"};
static void S3_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S3_fn)original)((uint32_t)v[0].i, (int32_t)v[1].i, (int32_t)v[2].i);
}
#define S3_THUNK(N, I, K) static void N##_##K(uint32_t moby, int32_t flags, int32_t arg) { \
    openrac_script_value v[3] = {{0, 0.0}}; \
    v[0].i = (int64_t)moby; \
    v[1].i = (int64_t)flags; \
    v[2].i = (int64_t)arg; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S3_fn)k_##N##_links[K].original)(moby, flags, arg); \
        return; \
    } \
}
#define S3_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S3_THUNK(N, I, 0) S3_THUNK(N, I, 1) S3_THUNK(N, I, 2) S3_THUNK(N, I, 3) S3_THUNK(N, I, 4) S3_THUNK(N, I, 5) S3_THUNK(N, I, 6) S3_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S4: i32(i32,i32) (load, resume). */
typedef int32_t (*S4_fn)(int32_t, int32_t);
static const uint8_t k_S4_kinds[] = {OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S4_names[] = {"load", "resume"};
static void S4_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S4_fn)original)((int32_t)v[0].i, (int32_t)v[1].i);
}
#define S4_THUNK(N, I, K) static int32_t N##_##K(int32_t load, int32_t resume) { \
    openrac_script_value v[2] = {{0, 0.0}}; \
    v[0].i = (int64_t)load; \
    v[1].i = (int64_t)resume; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S4_fn)k_##N##_links[K].original)(load, resume); \
    } \
    return (int32_t)r.i; \
}
#define S4_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S4_THUNK(N, I, 0) S4_THUNK(N, I, 1) S4_THUNK(N, I, 2) S4_THUNK(N, I, 3) S4_THUNK(N, I, 4) S4_THUNK(N, I, 5) S4_THUNK(N, I, 6) S4_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S5: void(f32,f32) (up, down). */
typedef void (*S5_fn)(float, float);
static const uint8_t k_S5_kinds[] = {OPENRAC_SV_F32, OPENRAC_SV_F32};
static const char* const k_S5_names[] = {"up", "down"};
static void S5_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S5_fn)original)((float)v[0].f, (float)v[1].f);
}
#define S5_THUNK(N, I, K) static void N##_##K(float up, float down) { \
    openrac_script_value v[2] = {{0, 0.0}}; \
    v[0].f = (double)up; \
    v[1].f = (double)down; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S5_fn)k_##N##_links[K].original)(up, down); \
        return; \
    } \
}
#define S5_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S5_THUNK(N, I, 0) S5_THUNK(N, I, 1) S5_THUNK(N, I, 2) S5_THUNK(N, I, 3) S5_THUNK(N, I, 4) S5_THUNK(N, I, 5) S5_THUNK(N, I, 6) S5_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S6: i32(u32,u32,u32) (a0, a1, count). */
typedef int32_t (*S6_fn)(uint32_t, uint32_t, uint32_t);
static const uint8_t k_S6_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_U32, OPENRAC_SV_U32};
static const char* const k_S6_names[] = {"a0", "a1", "count"};
static void S6_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S6_fn)original)((uint32_t)v[0].i, (uint32_t)v[1].i, (uint32_t)v[2].i);
}
#define S6_THUNK(N, I, K) static int32_t N##_##K(uint32_t a0, uint32_t a1, uint32_t count) { \
    openrac_script_value v[3] = {{0, 0.0}}; \
    v[0].i = (int64_t)a0; \
    v[1].i = (int64_t)a1; \
    v[2].i = (int64_t)count; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S6_fn)k_##N##_links[K].original)(a0, a1, count); \
    } \
    return (int32_t)r.i; \
}
#define S6_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S6_THUNK(N, I, 0) S6_THUNK(N, I, 1) S6_THUNK(N, I, 2) S6_THUNK(N, I, 3) S6_THUNK(N, I, 4) S6_THUNK(N, I, 5) S6_THUNK(N, I, 6) S6_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

S0_POINT(Hero_TakeDamage, 0) /* rac2.player.take_damage */
S1_POINT(Moby_GetHitMessage, 1) /* rac2.combat.get_hit */
S2_POINT(Weapon_TakeAmmo, 2) /* rac2.weapon.take_ammo */
S3_POINT(Moby_RecordDeath, 3) /* rac2.moby.record_death */
S4_POINT(Level_Load, 4) /* rac2.level.load */
S5_POINT(Hero_ApproachSpeed, 5) /* rac2.player.approach_speed */
S6_POINT(Hero_NanotechSteps, 6) /* rac2.player.nanotech_steps */

static const openrac_script_point k_points[7] = {
    {"rac2.player.take_damage", "void(i32)", 1, k_S0_kinds, k_S0_names, OPENRAC_SV_VOID, k_Hero_TakeDamage_links, k_Hero_TakeDamage_thunks, S0_call},
    {"rac2.combat.get_hit", "u32(u32,u32,i32)", 3, k_S1_kinds, k_S1_names, OPENRAC_SV_U32, k_Moby_GetHitMessage_links, k_Moby_GetHitMessage_thunks, S1_call},
    {"rac2.weapon.take_ammo", "i32(i32,i32)", 2, k_S2_kinds, k_S2_names, OPENRAC_SV_I32, k_Weapon_TakeAmmo_links, k_Weapon_TakeAmmo_thunks, S2_call},
    {"rac2.moby.record_death", "void(u32,i32,i32)", 3, k_S3_kinds, k_S3_names, OPENRAC_SV_VOID, k_Moby_RecordDeath_links, k_Moby_RecordDeath_thunks, S3_call},
    {"rac2.level.load", "i32(i32,i32)", 2, k_S4_kinds, k_S4_names, OPENRAC_SV_I32, k_Level_Load_links, k_Level_Load_thunks, S4_call},
    {"rac2.player.approach_speed", "void(f32,f32)", 2, k_S5_kinds, k_S5_names, OPENRAC_SV_VOID, k_Hero_ApproachSpeed_links, k_Hero_ApproachSpeed_thunks, S5_call},
    {"rac2.player.nanotech_steps", "i32(u32,u32,u32)", 3, k_S6_kinds, k_S6_names, OPENRAC_SV_I32, k_Hero_NanotechSteps_links, k_Hero_NanotechSteps_thunks, S6_call},
};

static const openrac_script_alias k_aliases[27] = {
    {"Hero_ApproachSpeed", "rac2.player.approach_speed"},
    {"Hero_NanotechSteps", "rac2.player.nanotech_steps"},
    {"Hero_TakeDamage", "rac2.player.take_damage"},
    {"Level_Load", "rac2.level.load"},
    {"Moby_GetHitMessage", "rac2.combat.get_hit"},
    {"Moby_RecordDeath", "rac2.moby.record_death"},
    {"Weapon_TakeAmmo", "rac2.weapon.take_ammo"},
    {"func_002A6C58", "rac2.moby.record_death"},
    {"func_002A7650", "rac2.moby.record_death"},
    {"func_002A7AA8", "rac2.moby.record_death"},
    {"func_002A9640", "rac2.combat.get_hit"},
    {"func_002AA0A8", "rac2.combat.get_hit"},
    {"func_002AA4A8", "rac2.combat.get_hit"},
    {"func_L00_002A4D58", "rac2.player.take_damage"},
    {"func_L00_002A4EF8", "rac2.player.take_damage"},
    {"func_L00_002B40E0", "rac2.player.approach_speed"},
    {"func_L00_002B41B8", "rac2.player.approach_speed"},
    {"func_L00_002B42C0", "rac2.player.approach_speed"},
    {"func_L00_002D3F30", "rac2.weapon.take_ammo"},
    {"func_L00_002D42D0", "rac2.weapon.take_ammo"},
    {"func_L00_002D4458", "rac2.weapon.take_ammo"},
    {"func_L00_002D5E30", "rac2.player.nanotech_steps"},
    {"func_L00_002D61D0", "rac2.player.nanotech_steps"},
    {"func_L00_002D6380", "rac2.player.nanotech_steps"},
    {"func_L00_00303990", "rac2.level.load"},
    {"func_L00_00304660", "rac2.level.load"},
    {"func_L00_00305570", "rac2.level.load"},
};

static const char* const k_discs[] = {"ntsc", "pal", "ntsc2"};
static const openrac_script_field k_fields[1] = {
    {NULL, 0, 0, NULL},
};

static const openrac_script_binding k_binding = {
    "rac2", 3, k_discs, 7, k_points, 0, k_fields, NULL,
    27, k_aliases,
};

const openrac_script_binding* openrac_game_script_binding(void) {
    return &k_binding;
}
