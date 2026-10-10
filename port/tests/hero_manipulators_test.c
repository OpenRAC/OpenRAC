/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/level_hero_manipulators.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(HeroPoseNode) == 0x40, "EE node");
_Static_assert(offsetof(HeroPoseNode, pose) == 0x18, "EE pose");
_Static_assert(offsetof(HeroPoseNode, frame) == 0x20, "EE frames");
_Static_assert(offsetof(HeroPoseNode, transition) == 0x30, "EE transition");
_Static_assert(offsetof(HeroPoseNode, buffers) == 0x38, "EE buffers");
_Static_assert(offsetof(HeroPoseState, nodes) == 0xD00, "EE nodes");
_Static_assert(offsetof(HeroPoseState, extra) == 0xD08, "EE extra");
_Static_assert(offsetof(HeroPoseState, allow_extra) == 0xD14, "EE extra override");
_Static_assert(offsetof(HeroPoseState, hero) == 0x2080, "EE hero");
_Static_assert(offsetof(HeroPoseState, selector) == 0x20AB, "EE selector");
_Static_assert(offsetof(HeroPoseState, force_fade) == 0x20AF, "EE force fade");
_Static_assert(offsetof(HeroPoseMoby, sequence) == 0x52, "EE sequence");
#endif
unsigned char hp_state[0x5000], hp_pose_index[8], hp_buffers[0x1000];
float hp_step, hp_scale, hp_pose_data[0x700 / 4];
int hp_ids[2];
void* hp_poses[8];
static HeroPoseState* g;
static HeroPoseMoby hero;
static HeroPoseNode nodes[2];
static int created, removed, updated, trapped, fail_create, special_mode, ticks_called;
static int category_calls, remaps;
static float special_factor;
static float targets[20];
static int approaches;
float hp_approach(float* v, float target, float step) {
    CHECK(approaches < 20 && step == hp_step * 0.1f);
    targets[approaches++] = target;
    if (*v < target) { *v += step; if (*v > target) *v = target; }
    else if (*v > target) { *v -= step; if (*v < target) *v = target; }
    return *v;
}
HeroPoseNode* hp_create(void* p, int id) {
    CHECK(p == &hero && (id == 20 || id == 21)); ++created;
    return fail_create ? NULL : &nodes[id - 20];
}
void hp_remove(void* p, HeroPoseNode** slot) { CHECK(p == &hero && *slot); ++removed; *slot = NULL; }
void hp_update(void* p, void* n) { CHECK(p == &hero && (n == &nodes[0] || n == &nodes[1])); ++updated; }
int hp_category(int id) { ++category_calls; return id / 10; }
int hp_special(int id, float* factor) { CHECK(id == hero.sequence[1]); *factor = special_factor; return special_mode; }
int hp_sequence(void* p, int id) { CHECK(p == &hero); ++remaps; return id + 1; }
int hp_ticks(int n) { CHECK(n == 15); ++ticks_called; return 12; }
void hp_trap(void) { ++trapped; }
static void reset(void) {
    memset(hp_state, 0, sizeof(hp_state)); memset(nodes, 0, sizeof(nodes));
    memset(&hero, 0, sizeof(hero)); memset(hp_pose_data, 0, sizeof(hp_pose_data));
    g = (HeroPoseState*)(hp_state + 0xE1D); g->hero = &hero;
    hp_ids[0] = 20; hp_ids[1] = 21; hp_pose_index[1] = 3; hp_pose_index[2] = 4;
    hp_poses[3] = &nodes[0]; hp_poses[4] = &nodes[1];
    hp_step = 1; hp_scale = 2; hero.sequence[0] = hero.sequence[1] = 4;
    hero.frame[0] = 7; hero.frame[1] = 8; hero.blend = 0.25f;
    created = removed = updated = trapped = fail_create = special_mode = ticks_called = 0;
    category_calls = remaps = approaches = 0; special_factor = 0;
}
static void allocation_and_fade(void) {
    int selector;
    reset(); func_L00_0020E3B8(); CHECK(created == 0 && updated == 0);
    for (selector = 1; selector <= 3; ++selector) {
        reset(); g->selector = (unsigned char)selector; func_L00_0020E3B8();
        CHECK(created == (selector == 2 ? 2 : 1) && updated == created);
        CHECK(nodes[0].pose == hp_poses[3] && nodes[0].weight == 0.1f);
        CHECK(nodes[0].frame[0] == 7 && nodes[0].frame[1] == 8 && nodes[0].blend == 0.25f);
        CHECK(nodes[0].sequence[0] == 5 && nodes[0].sequence[1] == 5);
        CHECK(nodes[0].buffers[0] == hp_buffers && nodes[0].buffers[1] == hp_buffers + 0x400);
        if (selector == 2) CHECK(nodes[1].buffers[1] == hp_buffers + 0xC00 && nodes[1].pose == hp_poses[4]);
    }
    reset(); g->selector = 2; fail_create = 1; func_L00_0020E3B8();
    CHECK(trapped == 1 && created == 1 && updated == 0 && !g->nodes[0]);
    reset(); g->nodes[0] = &nodes[0]; nodes[0].weight = 0.05f; nodes[0].fading = 1;
    g->selector = 2; func_L00_0020E3B8(); CHECK(removed == 1 && !g->nodes[0] && created == 1);
    reset(); g->nodes[0] = &nodes[0]; nodes[0].weight = 0.5f; g->selector = 1; g->force_fade = 1;
    func_L00_0020E3B8(); CHECK(nodes[0].fading == 1 && nodes[0].weight == 0.4f && approaches == 1);
    reset(); g->nodes[0] = &nodes[0]; nodes[0].weight = 0.5f; g->selector = 1; g->extra = &hero;
    func_L00_0020E3B8(); CHECK(targets[0] == 0 && nodes[0].weight == 0.4f);
    g->allow_extra = 1; approaches = 0; func_L00_0020E3B8(); CHECK(targets[0] == 1);
}
static void ordinary_transitions(void) {
    reset(); g->selector = 1; g->nodes[0] = &nodes[0]; hero.sequence[1] = 8;
    nodes[0].sequence[0] = 40; nodes[0].buffers[0] = &hero; nodes[0].frame[0] = 99;
    func_L00_0020E3B8(); CHECK(nodes[0].sequence[0] == 40 && nodes[0].sequence[1] == 9 && remaps == 1);
    CHECK(nodes[0].frame[0] == 99 && !nodes[0].use_buffer[0] && nodes[0].buffers[0] == &hero);
    reset(); g->selector = 1; g->nodes[0] = &nodes[0]; g->compare_categories = 1;
    hero.sequence[1] = 30; nodes[0].sequence[0] = 10; nodes[0].sequence[1] = 20;
    nodes[0].frame[0] = 2; nodes[0].frame[1] = 3; nodes[0].blend = 0.9f;
    func_L00_0020E3B8(); CHECK(category_calls == 2 && remaps == 0 && nodes[0].transition == 0);
    CHECK(nodes[0].sequence[0] == 20 && nodes[0].frame[0] == 3 && nodes[0].blend > 1);
    nodes[0].sequence[0] = 10; nodes[0].blend = 0.2f; approaches = 0;
    func_L00_0020E3B8(); CHECK(nodes[0].transition == 1 && nodes[0].blend > 0.34f && nodes[0].blend < 0.36f);
}
static void special_transitions(void) {
    reset(); g->selector = 1; g->nodes[0] = &nodes[0]; special_mode = 1; special_factor = 0.7f;
    nodes[0].sequence[0] = 2; nodes[0].sequence[1] = 3; nodes[0].frame[1] = 5; nodes[0].blend = 0.6f;
    func_L00_0020E3B8();
    CHECK(nodes[0].special == 1 && nodes[0].sequence[0] == 3 && nodes[0].frame[0] == 5);
    CHECK(nodes[0].sequence[1] == 0 && nodes[0].blend == 0 && nodes[0].speed == 1 && nodes[0].rate == 1.0f / 12);
    CHECK(hp_pose_data[0x5E4 / 4] == 0.7f && hp_pose_data[0x694 / 4] == 0.7f);
    CHECK(hp_pose_data[0x624 / 4] == 0.1f && hp_pose_data[0x6D4 / 4] == 0.1f);
    CHECK(hp_pose_data[0x628 / 4] == 0.6f && hp_pose_data[0x6D8 / 4] == 0.6f);
    approaches = 0; func_L00_0020E3B8(); CHECK(ticks_called == 1);
    special_mode = 0; approaches = 0; func_L00_0020E3B8();
    CHECK(nodes[0].releasing == 1 && nodes[0].sequence[1] == 5 && nodes[0].speed == 0 && nodes[0].blend == 0);
    nodes[0].blend = 0.95f; approaches = 0; func_L00_0020E3B8();
    CHECK(nodes[0].blend == 1 && nodes[0].special == 0);
    reset(); g->selector = 1; g->nodes[0] = &nodes[0]; special_mode = 1;
    nodes[0].sequence[0] = 2; nodes[0].sequence[1] = 3; nodes[0].frame[0] = 6; nodes[0].frame[1] = 7;
    nodes[0].blend = 0.5f; func_L00_0020E3B8();
    CHECK(nodes[0].sequence[0] == 2 && nodes[0].frame[0] == 6);
}
int main(void) {
    allocation_and_fade(); ordinary_transitions(); special_transitions();
    puts("hero manipulators: OK"); return 0;
}
