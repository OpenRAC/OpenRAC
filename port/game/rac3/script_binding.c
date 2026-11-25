/* Generated from sdk/bindings/rac3.json by sdk/tools/gen_binding.py. Do not edit. */

/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The script mods' side of the game's hook points, their names and its fields (docs/port/MODS.md,
 * sections 4 and 11; openrac/script_binding.h). */
#include <stddef.h>
#include <stdint.h>

#include "openrac/script_binding.h"

static const openrac_script_point k_points[17];

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

/* Shape S1: void(i32) (id). */
typedef void (*S1_fn)(int32_t);
static const uint8_t k_S1_kinds[] = {OPENRAC_SV_I32};
static const char* const k_S1_names[] = {"id"};
static void S1_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S1_fn)original)((int32_t)v[0].i);
}
#define S1_THUNK(N, I, K) static void N##_##K(int32_t id) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    v[0].i = (int64_t)id; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S1_fn)k_##N##_links[K].original)(id); \
        return; \
    } \
}
#define S1_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S1_THUNK(N, I, 0) S1_THUNK(N, I, 1) S1_THUNK(N, I, 2) S1_THUNK(N, I, 3) S1_THUNK(N, I, 4) S1_THUNK(N, I, 5) S1_THUNK(N, I, 6) S1_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S2: i32(i32) (id). */
typedef int32_t (*S2_fn)(int32_t);
static const uint8_t k_S2_kinds[] = {OPENRAC_SV_I32};
static const char* const k_S2_names[] = {"id"};
static void S2_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S2_fn)original)((int32_t)v[0].i);
}
#define S2_THUNK(N, I, K) static int32_t N##_##K(int32_t id) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    v[0].i = (int64_t)id; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S2_fn)k_##N##_links[K].original)(id); \
    } \
    return (int32_t)r.i; \
}
#define S2_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S2_THUNK(N, I, 0) S2_THUNK(N, I, 1) S2_THUNK(N, I, 2) S2_THUNK(N, I, 3) S2_THUNK(N, I, 4) S2_THUNK(N, I, 5) S2_THUNK(N, I, 6) S2_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S3: i32(i32,i32) (item, amount). */
typedef int32_t (*S3_fn)(int32_t, int32_t);
static const uint8_t k_S3_kinds[] = {OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S3_names[] = {"item", "amount"};
static void S3_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S3_fn)original)((int32_t)v[0].i, (int32_t)v[1].i);
}
#define S3_THUNK(N, I, K) static int32_t N##_##K(int32_t item, int32_t amount) { \
    openrac_script_value v[2] = {{0, 0.0}}; \
    v[0].i = (int64_t)item; \
    v[1].i = (int64_t)amount; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S3_fn)k_##N##_links[K].original)(item, amount); \
    } \
    return (int32_t)r.i; \
}
#define S3_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S3_THUNK(N, I, 0) S3_THUNK(N, I, 1) S3_THUNK(N, I, 2) S3_THUNK(N, I, 3) S3_THUNK(N, I, 4) S3_THUNK(N, I, 5) S3_THUNK(N, I, 6) S3_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S4: i32() (no parameters). */
typedef int32_t (*S4_fn)(void);
static const uint8_t k_S4_kinds[] = {OPENRAC_SV_VOID};
static const char* const k_S4_names[] = {NULL};
static void S4_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)v;
    r->i = ((S4_fn)original)();
}
#define S4_THUNK(N, I, K) static int32_t N##_##K(void) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S4_fn)k_##N##_links[K].original)(); \
    } \
    return (int32_t)r.i; \
}
#define S4_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S4_THUNK(N, I, 0) S4_THUNK(N, I, 1) S4_THUNK(N, I, 2) S4_THUNK(N, I, 3) S4_THUNK(N, I, 4) S4_THUNK(N, I, 5) S4_THUNK(N, I, 6) S4_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S5: i32(i32) (mode). */
typedef int32_t (*S5_fn)(int32_t);
static const uint8_t k_S5_kinds[] = {OPENRAC_SV_I32};
static const char* const k_S5_names[] = {"mode"};
static void S5_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S5_fn)original)((int32_t)v[0].i);
}
#define S5_THUNK(N, I, K) static int32_t N##_##K(int32_t mode) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    v[0].i = (int64_t)mode; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S5_fn)k_##N##_links[K].original)(mode); \
    } \
    return (int32_t)r.i; \
}
#define S5_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S5_THUNK(N, I, 0) S5_THUNK(N, I, 1) S5_THUNK(N, I, 2) S5_THUNK(N, I, 3) S5_THUNK(N, I, 4) S5_THUNK(N, I, 5) S5_THUNK(N, I, 6) S5_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S6: void(u32,i32,i32) (moby, hit, killer). */
typedef void (*S6_fn)(uint32_t, int32_t, int32_t);
static const uint8_t k_S6_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S6_names[] = {"moby", "hit", "killer"};
static void S6_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S6_fn)original)((uint32_t)v[0].i, (int32_t)v[1].i, (int32_t)v[2].i);
}
#define S6_THUNK(N, I, K) static void N##_##K(uint32_t moby, int32_t hit, int32_t killer) { \
    openrac_script_value v[3] = {{0, 0.0}}; \
    v[0].i = (int64_t)moby; \
    v[1].i = (int64_t)hit; \
    v[2].i = (int64_t)killer; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S6_fn)k_##N##_links[K].original)(moby, hit, killer); \
        return; \
    } \
}
#define S6_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S6_THUNK(N, I, 0) S6_THUNK(N, I, 1) S6_THUNK(N, I, 2) S6_THUNK(N, I, 3) S6_THUNK(N, I, 4) S6_THUNK(N, I, 5) S6_THUNK(N, I, 6) S6_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S7: void(u32,u32) (victim, record). */
typedef void (*S7_fn)(uint32_t, uint32_t);
static const uint8_t k_S7_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_U32};
static const char* const k_S7_names[] = {"victim", "record"};
static void S7_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S7_fn)original)((uint32_t)v[0].i, (uint32_t)v[1].i);
}
#define S7_THUNK(N, I, K) static void N##_##K(uint32_t victim, uint32_t record) { \
    openrac_script_value v[2] = {{0, 0.0}}; \
    v[0].i = (int64_t)victim; \
    v[1].i = (int64_t)record; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S7_fn)k_##N##_links[K].original)(victim, record); \
        return; \
    } \
}
#define S7_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S7_THUNK(N, I, 0) S7_THUNK(N, I, 1) S7_THUNK(N, I, 2) S7_THUNK(N, I, 3) S7_THUNK(N, I, 4) S7_THUNK(N, I, 5) S7_THUNK(N, I, 6) S7_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S8: void(u32,i32,i32,i32,i32,i32,f32) (moby, low, high, flags, surface, maximum, height). */
typedef void (*S8_fn)(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, float);
static const uint8_t k_S8_kinds[] = {OPENRAC_SV_U32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_F32};
static const char* const k_S8_names[] = {"moby", "low", "high", "flags", "surface", "maximum", "height"};
static void S8_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)r;
    ((S8_fn)original)((uint32_t)v[0].i, (int32_t)v[1].i, (int32_t)v[2].i, (int32_t)v[3].i, (int32_t)v[4].i, (int32_t)v[5].i, (float)v[6].f);
}
#define S8_THUNK(N, I, K) static void N##_##K(uint32_t moby, int32_t low, int32_t high, int32_t flags, int32_t surface, int32_t maximum, float height) { \
    openrac_script_value v[7] = {{0, 0.0}}; \
    v[0].i = (int64_t)moby; \
    v[1].i = (int64_t)low; \
    v[2].i = (int64_t)high; \
    v[3].i = (int64_t)flags; \
    v[4].i = (int64_t)surface; \
    v[5].i = (int64_t)maximum; \
    v[6].f = (double)height; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S8_fn)k_##N##_links[K].original)(moby, low, high, flags, surface, maximum, height); \
        return; \
    } \
}
#define S8_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S8_THUNK(N, I, 0) S8_THUNK(N, I, 1) S8_THUNK(N, I, 2) S8_THUNK(N, I, 3) S8_THUNK(N, I, 4) S8_THUNK(N, I, 5) S8_THUNK(N, I, 6) S8_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S9: i32(i32,i32,i32,i32,i32,i32) (source, position, velocity, flags, value, counted). */
typedef int32_t (*S9_fn)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
static const uint8_t k_S9_kinds[] = {OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32, OPENRAC_SV_I32};
static const char* const k_S9_names[] = {"source", "position", "velocity", "flags", "value", "counted"};
static void S9_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S9_fn)original)((int32_t)v[0].i, (int32_t)v[1].i, (int32_t)v[2].i, (int32_t)v[3].i, (int32_t)v[4].i, (int32_t)v[5].i);
}
#define S9_THUNK(N, I, K) static int32_t N##_##K(int32_t source, int32_t position, int32_t velocity, int32_t flags, int32_t value, int32_t counted) { \
    openrac_script_value v[6] = {{0, 0.0}}; \
    v[0].i = (int64_t)source; \
    v[1].i = (int64_t)position; \
    v[2].i = (int64_t)velocity; \
    v[3].i = (int64_t)flags; \
    v[4].i = (int64_t)value; \
    v[5].i = (int64_t)counted; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S9_fn)k_##N##_links[K].original)(source, position, velocity, flags, value, counted); \
    } \
    return (int32_t)r.i; \
}
#define S9_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S9_THUNK(N, I, 0) S9_THUNK(N, I, 1) S9_THUNK(N, I, 2) S9_THUNK(N, I, 3) S9_THUNK(N, I, 4) S9_THUNK(N, I, 5) S9_THUNK(N, I, 6) S9_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S10: i32(i32) (moby). */
typedef int32_t (*S10_fn)(int32_t);
static const uint8_t k_S10_kinds[] = {OPENRAC_SV_I32};
static const char* const k_S10_names[] = {"moby"};
static void S10_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    r->i = ((S10_fn)original)((int32_t)v[0].i);
}
#define S10_THUNK(N, I, K) static int32_t N##_##K(int32_t moby) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    v[0].i = (int64_t)moby; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        return ((S10_fn)k_##N##_links[K].original)(moby); \
    } \
    return (int32_t)r.i; \
}
#define S10_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S10_THUNK(N, I, 0) S10_THUNK(N, I, 1) S10_THUNK(N, I, 2) S10_THUNK(N, I, 3) S10_THUNK(N, I, 4) S10_THUNK(N, I, 5) S10_THUNK(N, I, 6) S10_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

/* Shape S11: void() (no parameters). */
typedef void (*S11_fn)(void);
static const uint8_t k_S11_kinds[] = {OPENRAC_SV_VOID};
static const char* const k_S11_names[] = {NULL};
static void S11_call(void* original, const openrac_script_value* v, openrac_script_value* r) {
    (void)v;
    (void)r;
    ((S11_fn)original)();
}
#define S11_THUNK(N, I, K) static void N##_##K(void) { \
    openrac_script_value v[1] = {{0, 0.0}}; \
    openrac_script_value r = {0, 0.0}; \
    if (!openrac_script_dispatch(&k_points[I], &k_##N##_links[K], v, &r)) { \
        ((S11_fn)k_##N##_links[K].original)(); \
        return; \
    } \
}
#define S11_POINT(N, I) static openrac_script_link k_##N##_links[OPENRAC_SCRIPT_LINKS]; S11_THUNK(N, I, 0) S11_THUNK(N, I, 1) S11_THUNK(N, I, 2) S11_THUNK(N, I, 3) S11_THUNK(N, I, 4) S11_THUNK(N, I, 5) S11_THUNK(N, I, 6) S11_THUNK(N, I, 7) static void* const k_##N##_thunks[OPENRAC_SCRIPT_LINKS] = {(void*)N##_0, (void*)N##_1, (void*)N##_2, (void*)N##_3, (void*)N##_4, (void*)N##_5, (void*)N##_6, (void*)N##_7};

S0_POINT(Hero_TakeDamage, 0) /* rac3.player.take_damage */
S1_POINT(Item_Give, 1) /* rac3.item.give */
S2_POINT(Item_Upgrade, 2) /* rac3.item.upgrade */
S1_POINT(Weapon_LevelUp, 3) /* rac3.weapon.level_up */
S3_POINT(Weapon_TakeAmmo, 4) /* rac3.weapon.take_ammo */
S3_POINT(Weapon_AddAmmo, 5) /* rac3.weapon.add_ammo */
S4_POINT(Vendor_Price, 6) /* rac3.vendor.price */
S5_POINT(Vendor_BuildList, 7) /* rac3.vendor.build_list */
S2_POINT(Vendor_StockAdd, 8) /* rac3.vendor.stock_add */
S6_POINT(Moby_RecordDeath, 9) /* rac3.moby.record_death */
S7_POINT(Moby_PostHit, 10) /* rac3.combat.post_hit */
S8_POINT(Bolt_SpawnPickups, 11) /* rac3.bolt.spawn_pickups */
S9_POINT(Bolt_SpawnPickup, 12) /* rac3.bolt.spawn_pickup */
S10_POINT(Bolt_Collect, 13) /* rac3.bolt.collect */
S4_POINT(Level_Load, 14) /* rac3.level.load */
S11_POINT(Level_GiveItems, 15) /* rac3.level.give_items */
S11_POINT(Hero_DeathDone, 16) /* rac3.player.death_done */

static const openrac_script_point k_points[17] = {
    {"rac3.player.take_damage", "void(i32)", 1, k_S0_kinds, k_S0_names, OPENRAC_SV_VOID, k_Hero_TakeDamage_links, k_Hero_TakeDamage_thunks, S0_call},
    {"rac3.item.give", "void(i32)", 1, k_S1_kinds, k_S1_names, OPENRAC_SV_VOID, k_Item_Give_links, k_Item_Give_thunks, S1_call},
    {"rac3.item.upgrade", "i32(i32)", 1, k_S2_kinds, k_S2_names, OPENRAC_SV_I32, k_Item_Upgrade_links, k_Item_Upgrade_thunks, S2_call},
    {"rac3.weapon.level_up", "void(i32)", 1, k_S1_kinds, k_S1_names, OPENRAC_SV_VOID, k_Weapon_LevelUp_links, k_Weapon_LevelUp_thunks, S1_call},
    {"rac3.weapon.take_ammo", "i32(i32,i32)", 2, k_S3_kinds, k_S3_names, OPENRAC_SV_I32, k_Weapon_TakeAmmo_links, k_Weapon_TakeAmmo_thunks, S3_call},
    {"rac3.weapon.add_ammo", "i32(i32,i32)", 2, k_S3_kinds, k_S3_names, OPENRAC_SV_I32, k_Weapon_AddAmmo_links, k_Weapon_AddAmmo_thunks, S3_call},
    {"rac3.vendor.price", "i32()", 0, k_S4_kinds, k_S4_names, OPENRAC_SV_I32, k_Vendor_Price_links, k_Vendor_Price_thunks, S4_call},
    {"rac3.vendor.build_list", "i32(i32)", 1, k_S5_kinds, k_S5_names, OPENRAC_SV_I32, k_Vendor_BuildList_links, k_Vendor_BuildList_thunks, S5_call},
    {"rac3.vendor.stock_add", "i32(i32)", 1, k_S2_kinds, k_S2_names, OPENRAC_SV_I32, k_Vendor_StockAdd_links, k_Vendor_StockAdd_thunks, S2_call},
    {"rac3.moby.record_death", "void(u32,i32,i32)", 3, k_S6_kinds, k_S6_names, OPENRAC_SV_VOID, k_Moby_RecordDeath_links, k_Moby_RecordDeath_thunks, S6_call},
    {"rac3.combat.post_hit", "void(u32,u32)", 2, k_S7_kinds, k_S7_names, OPENRAC_SV_VOID, k_Moby_PostHit_links, k_Moby_PostHit_thunks, S7_call},
    {"rac3.bolt.spawn_pickups", "void(u32,i32,i32,i32,i32,i32,f32)", 7, k_S8_kinds, k_S8_names, OPENRAC_SV_VOID, k_Bolt_SpawnPickups_links, k_Bolt_SpawnPickups_thunks, S8_call},
    {"rac3.bolt.spawn_pickup", "i32(i32,i32,i32,i32,i32,i32)", 6, k_S9_kinds, k_S9_names, OPENRAC_SV_I32, k_Bolt_SpawnPickup_links, k_Bolt_SpawnPickup_thunks, S9_call},
    {"rac3.bolt.collect", "i32(i32)", 1, k_S10_kinds, k_S10_names, OPENRAC_SV_I32, k_Bolt_Collect_links, k_Bolt_Collect_thunks, S10_call},
    {"rac3.level.load", "i32()", 0, k_S4_kinds, k_S4_names, OPENRAC_SV_I32, k_Level_Load_links, k_Level_Load_thunks, S4_call},
    {"rac3.level.give_items", "void()", 0, k_S11_kinds, k_S11_names, OPENRAC_SV_VOID, k_Level_GiveItems_links, k_Level_GiveItems_thunks, S11_call},
    {"rac3.player.death_done", "void()", 0, k_S11_kinds, k_S11_names, OPENRAC_SV_VOID, k_Hero_DeathDone_links, k_Hero_DeathDone_thunks, S11_call},
};

static const openrac_script_alias k_aliases[51] = {
    {"Bolt_Collect", "rac3.bolt.collect"},
    {"Bolt_SpawnPickup", "rac3.bolt.spawn_pickup"},
    {"Bolt_SpawnPickups", "rac3.bolt.spawn_pickups"},
    {"Hero_DeathDone", "rac3.player.death_done"},
    {"Hero_TakeDamage", "rac3.player.take_damage"},
    {"Item_Give", "rac3.item.give"},
    {"Item_Upgrade", "rac3.item.upgrade"},
    {"Level_GiveItems", "rac3.level.give_items"},
    {"Level_Load", "rac3.level.load"},
    {"Moby_PostHit", "rac3.combat.post_hit"},
    {"Moby_RecordDeath", "rac3.moby.record_death"},
    {"Vendor_BuildList", "rac3.vendor.build_list"},
    {"Vendor_Price", "rac3.vendor.price"},
    {"Vendor_StockAdd", "rac3.vendor.stock_add"},
    {"Weapon_AddAmmo", "rac3.weapon.add_ammo"},
    {"Weapon_LevelUp", "rac3.weapon.level_up"},
    {"Weapon_TakeAmmo", "rac3.weapon.take_ammo"},
    {"func_L01_002F88E0", "rac3.bolt.collect"},
    {"func_L01_002F8918", "rac3.bolt.collect"},
    {"func_L01_002F8F20", "rac3.bolt.spawn_pickup"},
    {"func_L01_002F8F60", "rac3.bolt.spawn_pickup"},
    {"func_L01_00416640", "rac3.weapon.level_up"},
    {"func_L01_00416B38", "rac3.item.give"},
    {"func_L01_00416BF8", "rac3.vendor.stock_add"},
    {"func_L01_004176F8", "rac3.item.upgrade"},
    {"func_L01_00418C90", "rac3.weapon.level_up"},
    {"func_L01_00419188", "rac3.item.give"},
    {"func_L01_00419248", "rac3.vendor.stock_add"},
    {"func_L01_00419D48", "rac3.item.upgrade"},
    {"func_L01_004200A8", "rac3.level.give_items"},
    {"func_L01_004227D8", "rac3.level.give_items"},
    {"func_L01_004412D0", "rac3.moby.record_death"},
    {"func_L01_00443840", "rac3.combat.post_hit"},
    {"func_L01_004443E8", "rac3.moby.record_death"},
    {"func_L01_00446978", "rac3.combat.post_hit"},
    {"func_L01_0044A0E8", "rac3.bolt.spawn_pickups"},
    {"func_L01_0044D360", "rac3.bolt.spawn_pickups"},
    {"func_L01_004B6C98", "rac3.vendor.price"},
    {"func_L01_004B6E88", "rac3.vendor.build_list"},
    {"func_L01_004BA830", "rac3.vendor.price"},
    {"func_L01_004BAA20", "rac3.vendor.build_list"},
    {"func_L01_004D3998", "rac3.level.load"},
    {"func_L01_004D7658", "rac3.level.load"},
    {"func_L01_004DC1C0", "rac3.player.take_damage"},
    {"func_L01_004DFEA0", "rac3.player.take_damage"},
    {"func_L01_004EA040", "rac3.player.death_done"},
    {"func_L01_004EDE10", "rac3.player.death_done"},
    {"func_L01_0050D8F8", "rac3.weapon.take_ammo"},
    {"func_L01_0050D980", "rac3.weapon.add_ammo"},
    {"func_L01_005118D0", "rac3.weapon.take_ammo"},
    {"func_L01_00511958", "rac3.weapon.add_ammo"},
};

static const char* const k_discs[] = {"pal", "ntsc"};
static const openrac_script_field k_fields[1] = {
    {NULL, 0, 0, NULL},
};

static const openrac_script_binding k_binding = {
    "rac3", 2, k_discs, 17, k_points, 0, k_fields, NULL,
    51, k_aliases,
};

const openrac_script_binding* openrac_game_script_binding(void) {
    return &k_binding;
}
