/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/level_crate_init.c"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(offsetof(CrateInitMoby, vars) == 0x78, "EE moby vars");
_Static_assert(offsetof(CrateInitMoby, oclass) == 0xA6, "EE moby class");
_Static_assert(offsetof(CrateInitMoby, up) == 0xE0, "EE moby up");
_Static_assert(offsetof(CrateInitVars, flags) == 0xAC, "EE crate flags");
_Static_assert(offsetof(CrateInitVars, platform) == 0xF0, "EE platform link");
_Static_assert(offsetof(CrateInitHit, z) == 0x28, "EE collision height");
#endif

CrateInitHit crate_hit;
unsigned char crate_save[0xE000];
int* crate_lists[2];
static CrateInitMoby mobys[5], *members[5];
static CrateInitVars vars[5];
static int list[32], member_count, random_value, random_calls, destroyed, attachments, distances;

static struct {
    int hit;
    CrateInitMoby* moby;
    float z, from_z;
} rays[12];

static int ray_count, ray_index;

int crate_random(int limit) {
    CHECK(limit == 2);
    ++random_calls;
    return random_value;
}

float crate_angle(float a, float b) {
    return a + b;
}

void crate_destroy(CrateInitMoby* m) {
    CHECK(m == &mobys[0]);
    ++destroyed;
    m->oclass = 0;
}

int crate_is_crate(CrateInitMoby* m) {
    return m && (m->oclass == 0x1F4 || m->oclass == 0x1F5);
}

int crate_is_platform(CrateInitMoby* m) {
    return m && m->oclass == 0x555;
}

int crate_raycast(float* from, float* to, int mode, CrateInitMoby* ignore, int flags) {
    CHECK(mode == 2 && ignore == NULL && flags == 0);
    CHECK(ray_index < ray_count && to[2] == 0.1f);
    CHECK(from[2] == rays[ray_index].from_z);
    CHECK(from[0] == to[0] && from[1] == to[1] && from[3] == to[3]);
    crate_hit.moby = rays[ray_index].moby;
    crate_hit.z = rays[ray_index].z;
    return rays[ray_index++].hit;
}

int crate_relative(
    int unused, CrateInitMoby* platform, float* p, float* r, float* outp, float* outr
) {
    int i;
    CHECK(unused == 0 && crate_is_platform(platform));
    ++attachments;
    for (i = 0; i < 4; ++i) {
        outp[i] = p[i] - platform->position[i];
        outr[i] = r[i];
    }
    return 1;
}

int crate_first(CrateInitMoby** out, int group, int a, int b) {
    CHECK(group == 7 && a == 0 && b == 0);
    *out = member_count ? members[0] : NULL;
    return *out != NULL;
}

int crate_next(CrateInitMoby** out, CrateInitMoby* m, int a, int b) {
    int i;
    CHECK(a == 0 && b == 0);
    for (i = 0; i < member_count; ++i) {
        if (members[i] == m) {
            break;
        }
    }
    CHECK(i < member_count);
    *out = i + 1 < member_count ? members[i + 1] : NULL;
    return *out != NULL;
}

void crate_add(float* out, float* a, float* b) {
    int i;
    for (i = 0; i < 4; ++i) {
        out[i] = a[i] + b[i];
    }
}

float crate_distance(float* a, float* b) {
    CHECK(a != b);
    ++distances;
    return 0;
}

void crate_trap(void) {
    CHECK(0 && "unexpected retail assertion");
}

static void reset(void) {
    int i;
    memset(mobys, 0, sizeof(mobys));
    memset(vars, 0, sizeof(vars));
    memset(rays, 0, sizeof(rays));
    memset(list, 0, sizeof(list));
    memset(crate_save, 0, sizeof(crate_save));
    member_count = random_value = random_calls = destroyed = attachments = distances = 0;
    ray_count = ray_index = 0;
    crate_lists[0] = list;
    for (i = 0; i < 5; ++i) {
        mobys[i].vars = &vars[i];
        mobys[i].group = 255;
        mobys[i].oclass = 0x1F4;
        mobys[i].saved_index = 255;
        mobys[i].stamp = (unsigned short)(10 + i);
        mobys[i].position[2] = 10.0f;
        mobys[i].position[3] = 1.0f;
        mobys[i].up[2] = 1.0f;
        vars[i].list = -1;
    }
}

static void ray(int hit, CrateInitMoby* m, float z, float from_z) {
    rays[ray_count].hit = hit;
    rays[ray_count].moby = m;
    rays[ray_count].z = z;
    rays[ray_count++].from_z = from_z;
}

static void run(void) {
    func_L00_002D1168(&mobys[0]);
    CHECK(ray_index == ray_count);
}

static void single(void) {
    reset();
    vars[0].below = &mobys[1];
    ray(0, NULL, 0, 10.5f);
    run();
    CHECK(vars[0].flags == 5 && vars[0].stamp == 10 && vars[0].height == 1);
    CHECK(!vars[0].below && mobys[0].position[2] == 10 && random_calls == 1);
    reset();
    random_value = 1;
    ray(1, NULL, 3, 10.5f);
    run();
    CHECK(mobys[0].position[2] == 3 && mobys[0].rotation[2] == 1.5707963705062866f);
    reset();
    random_value = 1;
    mobys[0].rotation[1] = 0.2f;
    vars[0].below = &mobys[1];
    ray(1, &mobys[1], 3, 10.5f);
    run();
    CHECK(vars[0].below == &mobys[1] && mobys[0].rotation[2] == 0);
    reset();
    mobys[1].oclass = 0x555;
    ray(1, &mobys[1], 3, 10.5f);
    run();
    CHECK(attachments == 1 && vars[0].platform == &mobys[1]);
    CHECK(vars[0].flags == 13 && mobys[0].distance == 0x7F80 && vars[0].platform_height == 0);
    reset();
    mobys[1].oclass = 0x444;
    ray(1, &mobys[1], 3, 10.5f);
    run();
    CHECK(attachments == 0 && vars[0].flags == 13 && !vars[0].below);
    reset();
    mobys[0].oclass = 0x1F5;
    mobys[0].saved_index = 2;
    vars[0].saved_threshold = 1;
    run();
    CHECK(destroyed == 1 && vars[0].flags == 4 && random_calls == 0);
    reset();
    mobys[0].oclass = 0x1F5;
    mobys[0].saved_index = 2;
    vars[0].saved_threshold = 1;
    ((int*)(crate_save + 0xD875))[2] = 1;
    ray(0, NULL, 0, 10.5f);
    run();
    CHECK(destroyed == 0 && vars[0].flags == 5);
}

static void stack(void) {
    int i;
    reset();
    member_count = 4;
    for (i = 0; i < 4; ++i) {
        members[i] = &mobys[i];
        mobys[i].group = 7;
    }
    mobys[3].oclass = 0;
    mobys[3].vars = NULL; /* group includes a non-crate */
    mobys[4].oclass = 0x555;
    vars[0].inherited = 42;
    ray(1, &mobys[4], 3, 10.5f);
    ray(1, &mobys[0], 3, 10.5f);
    ray(1, &mobys[1], 3, 10.5f);
    ray(1, &mobys[4], 3, 10.5f);
    run();
    CHECK(vars[0].above == &mobys[1] && !vars[0].below);
    CHECK(vars[1].above == &mobys[2] && vars[1].below == &mobys[0]);
    CHECK(vars[2].below == &mobys[1] && !vars[2].above);
    CHECK(mobys[1].position[2] == 11 && mobys[2].position[2] == 12);
    CHECK(attachments == 1 && distances == 3);
    for (i = 0; i < 3; ++i) {
        CHECK(vars[i].platform == &mobys[4] && vars[i].platform_height == (float)i);
        CHECK(vars[i].inherited == 42 && mobys[i].collision == 255);
        CHECK(vars[i].platform_position[2] == 0 && vars[i].stamp == 10 + i);
    }
    CHECK(mobys[0].distance == 0); /* retail tests the exhausted first-pass cursor */
}

static void groups_and_lists(void) {
    int kind, state;
    for (kind = 0; kind < 4; ++kind) {
        reset();
        member_count = 1;
        members[0] = &mobys[0];
        mobys[0].group = 7;
        vars[0].below = &mobys[1];
        vars[0].inherited = 99;
        mobys[1].oclass = kind == 0 ? 0x128 : kind == 1 ? 0x129 : 0x1F4;
        mobys[1].group = kind == 2 ? 8 : 7;
        if (kind == 3) {
            mobys[1].vars = NULL;
        }
        ray(1, &mobys[1], 4, 10.5f);
        ray(0, NULL, 0, kind < 2 ? 4.5f : 10.5f);
        run();
        CHECK(!vars[0].below && vars[0].inherited == (kind < 2 ? 0u : 99u));
    }
    /* Stack-linked crates consume their list slots. Terminal/full lists detach,
     * active lists advance, and fresh lists clear every entry's state. */
    for (state = 0; state < 8; ++state) {
        int states[] = {0, 1, 3, 0x111, 0x11, 0x12, 0x110, 2};
        reset();
        member_count = 2;
        members[0] = &mobys[0];
        members[1] = &mobys[1];
        mobys[0].group = mobys[1].group = 7;
        vars[0].trigger = 1;
        vars[0].list = 0;
        list[0] = state == 7 ? 0 : 2;
        list[7] = states[state];
        list[11] = 77;
        ray(1, NULL, 3, 10.5f);
        ray(1, &mobys[0], 3, 10.5f);
        ray(0, NULL, 0, 3.5f);
        run();
        if (state == 0) {
            CHECK(vars[0].list == 0 && list[7] == 0 && list[11] == 0);
        } else if (state == 4) {
            CHECK(vars[0].list == 0 && list[7] == 0x12 && list[11] == 77);
        } else {
            CHECK(vars[0].list == -1 && list[11] == 77);
        }
    }
    reset();
    member_count = 1;
    members[0] = &mobys[0];
    mobys[0].group = 7;
    mobys[0].oclass = 0x1F5;
    mobys[0].saved_index = 0;
    vars[0].saved_threshold = 1;
    run();
    CHECK(destroyed == 1 && ray_index == 0);
    reset();
    mobys[0].group = 7;
    run();
    CHECK(vars[0].flags == 4); /* empty group */
}

int main(void) {
    single();
    stack();
    groups_and_lists();
    puts("crate initialization tests passed");
    return 0;
}
