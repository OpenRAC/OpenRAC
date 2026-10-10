/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/level_hero_items.c"
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(HeroCreateSlot) == 0x50, "EE slot size");
_Static_assert(sizeof(HeroItemDefinition) == 0x4C, "EE definition size");
_Static_assert(offsetof(HeroItems, slots) == 0x1090, "EE slots");
_Static_assert(offsetof(HeroItems, scratch) == 0x18F0, "EE scratch");
_Static_assert(offsetof(HeroItems, hero) == 0x2080, "EE hero pointer");
_Static_assert(offsetof(HeroItems, selector) == 0x20AB, "EE selector");
_Static_assert(offsetof(HeroItems, state) == 0x20B8, "EE state");
_Static_assert(offsetof(HeroItems, primary_override) == 0x20D4, "EE override");
_Static_assert(offsetof(HeroItems, hidden) == 0x22D8, "EE hidden flag");
_Static_assert(offsetof(HeroItemMoby, distance) == 0x32, "EE distance");
_Static_assert(offsetof(HeroItemMoby, lighting) == 0x38, "EE lighting");
_Static_assert(offsetof(HeroItemMoby, joints) == 0x73, "EE joints");
_Static_assert(offsetof(HeroItemMoby, auxiliary) == 0x74, "EE auxiliary");
#endif

unsigned char hero_items_state[0x5000], hero_items_options[16], hero_items_saved[0x60];
unsigned char hero_items_owned[3];
int hero_items_clock;
HeroItemDefinition hero_items_definitions[36];
static HeroItems* g;
static HeroItemMoby hero, pool[32], occupied;
static unsigned char model[8];
static int spawned, requested, cleared, animated, back_parts, requested_id;
static int spawn_ids[32];
static unsigned fail_mask;

void hero_items_request(int id, int flag) {
    CHECK(flag == -1);
    ++requested;
    requested_id = id;
}

void hero_items_clear(void* p, int value, int count) {
    CHECK(cleared < 3 && p == g->scratch[cleared]);
    CHECK(value == 0 && count == 0xB0);
    memset(p, value, (size_t)count);
    ++cleared;
}

HeroItemMoby* hero_items_spawn(int id) {
    HeroItemMoby* m;
    CHECK(spawned < 32);
    spawn_ids[spawned] = id;
    m = &pool[spawned++];
    if (fail_mask & (1u << (spawned - 1))) {
        return NULL;
    }
    memset(m, 0x55, sizeof(*m));
    m->model = model;
    /* The last two special models do not inspect a class/joint field. */
    if (id == 1034 || id == 1035) {
        m->model = NULL;
    }
    m->flags = 0x100;
    return m;
}

void hero_items_animate(void* p, int sequence, int a, int b) {
    CHECK(p == g->slots[0].primary && sequence == 1 && a == 0 && b == 1);
    ++animated;
}

void* hero_items_back_part(int i) {
    CHECK(i == back_parts);
    ++back_parts;
    return NULL;
}

static void reset(void) {
    int i;
    memset(hero_items_state, 0, sizeof(hero_items_state));
    memset(hero_items_options, 0, sizeof(hero_items_options));
    memset(hero_items_saved, 0, sizeof(hero_items_saved));
    memset(hero_items_owned, 0, sizeof(hero_items_owned));
    memset(hero_items_definitions, 0, sizeof(hero_items_definitions));
    g = (HeroItems*)(hero_items_state + 0xE1D);
    g->hero = &hero;
    hero.lighting[0] = 0x12345678;
    hero.lighting[1] = 0xABCD9876;
    memset(g->scratch, 0x33, sizeof(g->scratch));
    memset(g->pad1270, 0x44, sizeof(g->pad1270));
    memset(g->pad1B00, 0x66, sizeof(g->pad1B00));
    model[6] = 1;
    hero_items_clock = 1;
    spawned = requested = cleared = animated = back_parts = 0;
    fail_mask = 0;
    for (i = 0; i < 36; ++i) {
        hero_items_definitions[i].primary = 1000 + i;
        hero_items_definitions[i].secondary = 2000 + i;
        hero_items_definitions[i].selector = (unsigned char)(i + 80);
    }
}

static void check_model(HeroItemMoby* m, int joints) {
    CHECK(m && m->distance == 0x20 && m->draw == 1);
    CHECK(memcmp(m->lighting, hero.lighting, 8) == 0);
    CHECK(m->joints == (joints ? 0x18 : 0x55));
}

static void defaults_and_guards(void) {
    int i, j;
    reset();
    func_L00_0020F118();
    CHECK(spawned == 3 && requested == 1 && requested_id == 1008 && animated == 1);
    CHECK(spawn_ids[0] == 1008 && spawn_ids[1] == 1002 && spawn_ids[2] == 1001);
    CHECK(g->slots[0].primary == &pool[0] && g->slots[0].status == 2);
    CHECK(g->slots[0].flags == 0x80 && g->slots[0].item == 8 && g->selector == 88);
    CHECK(g->slots[3].primary == &pool[1] && g->slots[3].secondary == &pool[2]);
    CHECK(g->slots[3].status == 2 && g->slots[3].item == 2);
    for (i = 0; i < 3; ++i) {
        check_model(&pool[i], 1);
        for (j = 0; j < 0xB0; ++j) {
            CHECK(g->scratch[i][j] == (j == 0xA0 || j == 0xA1 ? 255 : 0));
        }
    }
    CHECK(cleared == 3 && g->pad1270[sizeof(g->pad1270) - 1] == 0x44 && g->pad1B00[0] == 0x66);
    func_L00_0020F118();
    CHECK(spawned == 3 && requested == 1); /* existing slots are untouched */
    for (i = 0; i < 3; ++i) {
        reset();
        if (i == 0) {
            g->slots[0].primary = &occupied;
        }
        if (i == 1) {
            g->slots[0].available = hero_items_clock;
        }
        if (i == 2) {
            g->state = 1;
        }
        func_L00_0020F118();
        CHECK(requested == 0 && cleared == 0 && spawned == 2);
    }
    reset();
    g->state = 0x24;
    model[6] = 0;
    func_L00_0020F118();
    CHECK(requested == 1);
    check_model(&pool[0], 0);
}

static void choices(void) {
    int i;
    reset();
    *(int*)(hero_items_saved + 0x45) = 6;
    func_L00_0020F118();
    CHECK(requested_id == 1006 && spawn_ids[0] == 1006 && animated == 0);
    CHECK(g->slots[0].flags == 0x20 && g->selector == 86);
    reset();
    *(int*)(hero_items_saved + 0x45) = 6;
    *(int*)(hero_items_options + 8) = 1;
    func_L00_0020F118();
    CHECK(requested_id == 1008);
    reset();
    g->primary_override = 7;
    *(int*)(hero_items_saved + 0x45) = 6;
    *(int*)(hero_items_options + 8) = 1;
    func_L00_0020F118();
    CHECK(requested_id == 1007);
    reset();
    g->primary_override = 6;
    hero_items_definitions[6].slot = 1;
    g->slots[1].item = 7;
    func_L00_0020F118();
    CHECK(requested_id == 1006 && spawn_ids[0] == 1007);
    CHECK(g->slots[0].primary == NULL && g->slots[1].primary == &pool[0]);
    CHECK(g->slots[1].flags == 0x20 && g->selector == 87);
    for (i = 0; i < 4; ++i) {
        reset();
        *(int*)(hero_items_options + 0xC) = 1;
        if (i >= 1) {
            *(int*)(hero_items_saved + 0x51) = 4;
        }
        if (i >= 2) {
            g->back_override = 3;
        }
        if (i == 3) {
            g->hidden = 1;
        }
        func_L00_0020F118();
        CHECK(spawn_ids[1] == (i == 1 ? 1004 : 1003));
        CHECK(back_parts == (i == 0 || i == 2 ? 2 : 0));
        CHECK(pool[1].flags == (i == 3 ? 0x141 : 0x100));
        CHECK(pool[2].flags == (i == 3 ? 0x141 : 0x100));
    }
}

static void all_items(void) {
    *(int*)(hero_items_saved + 0x4D) = 5;
    *(int*)(hero_items_saved + 0x49) = 9;
    memset(hero_items_owned, 1, sizeof(hero_items_owned));
}

static void optional_and_failure(void) {
    int i;
    reset();
    all_items();
    g->optional_override = 6;
    func_L00_0020F118();
    CHECK(spawned == 9 && spawn_ids[3] == 1006);
    CHECK(pool[3].flags == 0x902 && pool[3].auxiliary == 0);
    CHECK(spawn_ids[4] == 1009 && spawn_ids[5] == 2009);
    CHECK(spawn_ids[6] == 1033 && spawn_ids[7] == 1034 && spawn_ids[8] == 1035);
    CHECK(g->slots[4].item == 34 && g->slots[5].item == 35);
    for (i = 3; i < 9; ++i) {
        check_model(&pool[i], i < 7);
    }
    reset();
    all_items();
    fail_mask = ~0u;
    func_L00_0020F118();
    CHECK(spawned == 9 && spawn_ids[3] == 1005 && animated == 0 && back_parts == 0);
    for (i = 0; i < 6; ++i) {
        CHECK(g->slots[i].status == 0 && !g->slots[i].primary && !g->slots[i].secondary);
    }
    CHECK(g->selector == 0 && g->slots[4].item == 34 && g->slots[5].item == 35);
    fail_mask = 0;
    cleared = 0;
    func_L00_0020F118();
    CHECK(spawned == 18 && requested == 2 && g->slots[0].primary == &pool[9]);
    reset();
    all_items();
    fail_mask = 1u << 4;
    g->slots[1].secondary = &occupied;
    func_L00_0020F118();
    CHECK(!g->slots[1].primary && g->slots[1].secondary == &pool[5]);
    CHECK(g->slots[1].status == 2); /* secondary still attempted after failure */
    reset();
    all_items();
    g->slots[1].primary = &occupied;
    func_L00_0020F118();
    CHECK(spawned == 7 && g->slots[1].secondary == NULL); /* retail primary guard */
}

int main(void) {
    defaults_and_guards();
    choices();
    optional_and_failure();
    puts("hero item creation tests passed");
    return 0;
}
