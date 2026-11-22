/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's enemy copies (port/game/rac2/host/enemy_copies.c) on a level of the test's own,
 * laid out as LoadLevelData leaves one: which placed mobys may be copied (a free enemy; one of a
 * group, one the level names or that names another, one with a script slot; a spawner, a boss and
 * Tabora's crystal creatures of the port's table; a moby of another kind; one with no reward), the
 * copies (their spot on the floor, in the open and apart, what they keep of their slot and take of
 * their original, their pvars and the pointers into them, the moby cells and their squad's list),
 * the pool's reserve, a stale file, a new load, and a copy's death, once, leaving the original's
 * record as it was. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#include "../game/rac2/host/enemy_copies.c"
#include "../game/rac2/host/moby_pool.c"

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = 0x02000000u;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;
gaddr openrac_guest_sp = 0x01F00000u;

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

gaddr openrac_guest_frame(uint32_t size) {
    openrac_guest_sp = (openrac_guest_sp - size) & ~0xFu;
    return openrac_guest_sp;
}

static gaddr statics = 0x01C00000u;

gaddr openrac_guest_static(uint32_t size) {
    const gaddr at = statics;
    statics += (size + 15u) & ~15u;
    return at;
}

static const char* pool_env;

const char* openrac_env_string(const char* name) {
    (void)name;
    return pool_env;
}

static uint32_t program_loads = 1;

uint32_t rac2_program_loads(void) {
    return program_loads;
}

/* ---- The level ---- */

#define FILE_AT 0x00800000u
#define BASE 0x00400000u
#define PLACED 12u
#define DYNAMIC_SLOTS 200u
#define DYNAMIC (BASE + PLACED * 0x100u)
#define END (DYNAMIC + DYNAMIC_SLOTS * 0x100u)
#define CLASS_ENEMY 0x00500000u
#define CLASS_THING 0x00500100u
#define CLASS_LISTED 0x00500200u
#define ENEMY 2000
#define THING 500
#define SPAWNER 3098
#define DUNE_MONSTER 4098
#define CRYSTAL_SHELL 2410
#define BLOBS_AT 0x00700000u
#define SLOT_PVARS_AT 0x00600000u
#define WALL_X 13.0f
#define MAP 3
#define CAMERA_BLOB 12

/* The test's placed mobys, in slot order: their class. */
static int16_t class_at(uint32_t i) {
    switch (i) {
        case 3:
            return SPAWNER;
        case 4:
            return THING;
        case 10:
            return DUNE_MONSTER;
        case 11:
            return CRYSTAL_SHELL;
        default:
            return ENEMY;
    }
}

static void put32(gaddr a, uint32_t v) {
    memcpy(G(a), &v, 4);
}

static void put16(gaddr a, uint16_t v) {
    memcpy(G(a), &v, 2);
}

static void putf(gaddr a, float v) {
    memcpy(G(a), &v, 4);
}

static gaddr blob_at(int k) {
    return BLOBS_AT + 0x400u * (uint32_t)k;
}

static uint32_t blob_size(int k) {
    return k == 7 ? 0x200u : 0x40u;
}

static gaddr class_of(int16_t oclass) {
    switch (oclass) {
        case ENEMY:
            return CLASS_ENEMY;
        case THING:
            return CLASS_THING;
        case SPAWNER:
        case DUNE_MONSTER:
        case CRYSTAL_SHELL:
            return CLASS_LISTED;
        default:
            return 0;
    }
}

/* Each level of the test a program of its own (rac2_program_loads). */
static void make_level(void) {
    memset(openrac_guest_base, 0, openrac_guest_ram_size);
    ++program_loads;
    put32(0x001BF140u + 0x0Cu, FILE_AT);
    put32(RAC2_g_mobyTableBase, BASE);
    put32(RAC2_g_mobyTableBase + 4u, DYNAMIC);
    put32(RAC2_g_mobyTableEnd, END);
    put32(RAC2_g_playerProgress, MAP);
    put16(CLASS_ENEMY + 0x46u, 5);
    put16(CLASS_THING + 0x46u, 3);
    put16(CLASS_LISTED + 0x46u, 5);
    put32(FILE_AT + 0x4Cu, 0x1000u);
    put32(FILE_AT + 0x50u, 0x2000u);
    put32(FILE_AT + 0x58u, 0x3000u);
    put32(FILE_AT + 0x5Cu, 0x4000u);
    put32(FILE_AT + 0x64u, 0x5000u);
    /* The records and their mobys, one each in order. */
    put32(FILE_AT + 0x1000u, PLACED);
    for (uint32_t i = 0; i < PLACED; ++i) {
        const gaddr r = FILE_AT + 0x1010u + 0x88u * i;
        const int pv = i == 9 ? -1 : (int)i;
        put32(r, 0x88u);
        put32(r + 0x10u, 10u + i);
        put32(r + 0x68u, (uint32_t)pv);
        put32(r + 0x84u, i == 5 ? 3u : 0xFFFFFFFFu);
        const gaddr m = BASE + 0x100u * i;
        const int16_t oclass = class_at(i);
        put16(m + 0xAAu, (uint16_t)oclass);
        put32(m + 0x24u, class_of(oclass));
        put16(m + 0xB2u, (uint16_t)(10u + i));
        put16(m + 0xB4u, (uint16_t)(20u + i));
        GREF(uint8_t, m + 0xB1u) = i == 8 ? 0x80u : 0x01u;
        put32(m + 0x68u, pv >= 0 ? blob_at(pv) : 0);
        putf(m + 0x10u, 10.0f);
        putf(m + 0x14u, 10.0f + 10.0f * (float)i);
        putf(m + 0x18u, 1.0f);
        put32(m + 0xA4u, 0x11223344u);
        put32(m + 0xACu, i << 16);
    }
    /* The pvar blocks: the mobys' and a camera's; block 7's +0x10 points into itself. */
    for (int k = 0; k <= CAMERA_BLOB; ++k) {
        put32(FILE_AT + 0x4000u + 8u * (uint32_t)k, blob_at(k));
        put32(FILE_AT + 0x4000u + 8u * (uint32_t)k + 4u, blob_size(k));
        for (uint32_t b = 0; b < blob_size(k); b += 4) {
            put32(blob_at(k) + b, 0xA0000000u + (uint32_t)k * 0x1000u + b);
        }
    }
    put32(blob_at(7) + 0x10u, blob_at(7) + 0x100u);
    put32(blob_at(7) + 0x14u, 0x00123456u); /* a pointer elsewhere, not listed as inner */
    /* Two groups: a set piece (records 1 and 4, an enemy and a moby of another kind) and a squad
     * (records 0 and 9, two enemies of one class). */
    put32(FILE_AT + 0x2000u, 2);
    put32(FILE_AT + 0x2010u, 0);
    put32(FILE_AT + 0x2014u, 4);
    put16(FILE_AT + 0x2018u, 0x0001u);
    put16(FILE_AT + 0x201Au, 0x8004u);
    put16(FILE_AT + 0x201Cu, 0x0000u);
    put16(FILE_AT + 0x201Eu, 0x8009u);
    /* The groups as the load lays them out: lists of pool indices, the squad's group 1. */
    put32(0x001B1B78u, 2);
    put32(0x001EE5A0u + 4u, 0x00880000u);
    put16(0x00880000u, 0x0000u);
    put16(0x00880002u, 0x8009u);
    for (uint32_t i = 0; i < PLACED; ++i) {
        GREF(uint8_t, BASE + 0x100u * i + 0x21u) = i == 0 || i == 9 ? 1u : 0xFFu;
    }
    /* Fields naming a moby: the camera's names moby 2, block 6 names moby 4, block 0's names none. */
    const uint32_t refs[] = {CAMERA_BLOB, 0x8, 6, 0x4, 0, 0xC, 0xFFFFFFFFu, 0};
    for (unsigned i = 0; i < 8; ++i) {
        put32(FILE_AT + 0x3000u + 4u * i, refs[i]);
    }
    put32(blob_at(CAMERA_BLOB) + 0x8u, 2);
    put32(blob_at(6) + 0x4u, 4);
    put32(blob_at(0) + 0xCu, 0xFFFFFFFFu);
    /* The pointers into a block: block 7's +0x10. */
    put32(FILE_AT + 0x5000u, 7);
    put32(FILE_AT + 0x5004u, 0x10u);
    put32(FILE_AT + 0x5008u, 0xFFFFFFFFu);
    /* The pool past the placed mobys: free. */
    for (gaddr m = DYNAMIC; m < END; m += 0x100u) {
        GREF(uint8_t, m + 0x20u) = 0xFEu;
    }
}

/* ---- The game's calls ---- */

gaddr func_0029EFD0(int oclass) {
    for (gaddr m = DYNAMIC; m < END; m += 0x100u) {
        if (GREF(uint8_t, m + 0x20u) >= 0xFEu) {
            const uint32_t index = (m - DYNAMIC) / 0x100u;
            /* The never-used mark moves up past the slot taken. */
            if (GREF(uint8_t, m + 0x20u) == 0xFFu && m + 0x100u < END) {
                GREF(uint8_t, m + 0x120u) = 0xFFu;
            }
            memset(G(m), 0, 0x100u);
            put16(m + 0xAAu, (uint16_t)oclass);
            put32(m + 0x24u, class_of((int16_t)oclass));
            put32(m + 0x68u, SLOT_PVARS_AT + 0x80u * index);
            memset(G(SLOT_PVARS_AT + 0x80u * index), 0, 0x80u);
            GREF(uint8_t, m + 0x6Cu) = 0xFFu;
            GREF(uint8_t, m + 0x6Du) = 0xFFu;
            put32(m + 0xA4u, 0x80807F7Fu);
            put32(m + 0xACu, (PLACED + index) << 16);
            return m;
        }
    }
    return 0;
}

void func_0029F338(gaddr m) {
    GREF(uint8_t, m + 0x20u) = 0xFEu;
}

/* The floor is at 1 everywhere west of x = 150; a wall stands at x = WALL_X. */
float func_002A89A8(gaddr point, int mode, float over) {
    (void)mode;
    (void)over;
    return GREF(float, point) < 150.0f ? 1.0f : 0.0f;
}

int func_002761F8(gaddr from, gaddr to, int mask, gaddr ignore, gaddr out) {
    (void)mask;
    (void)ignore;
    (void)out;
    return GREF(float, from) >= WALL_X || GREF(float, to) >= WALL_X;
}

static int deaths;
static int gameplay_loads;
static gaddr celled;

/* MobyUpdateCell: the moby put in the cells at its place. */
void func_002A0F48(gaddr m) {
    celled = m;
}

/* LoadLevelData: the level laid out (make_level did), the heap's cursor returned. */
gaddr func_L00_00300618__game(int reload) {
    (void)reload;
    ++gameplay_loads;
    return 0x00900000u;
}

/* SetDeathBits's marks for the moby's save id: its kill count and both collection bytes. */
void func_002A6C58__hooked(gaddr moby, int flags, int arg) {
    (void)flags;
    (void)arg;
    const int16_t uid = GREF(int16_t, moby + 0xB2u);
    GREF(uint8_t, 0x0019B4A8u + (uint32_t)(uid >> 1) + 0x400u * MAP) += 1;
    GREF(uint8_t, 0x00210D40u + 0x464u + (uint32_t)uid) = 2;
    GREF(uint8_t, 0x002119B0u + 0x464u + (uint32_t)uid) = 2;
    ++deaths;
}

/* ---- The checks ---- */

static const rac2_moby_table table = {BASE, DYNAMIC, END};

static gaddr placed(uint32_t i) {
    return BASE + 0x100u * i;
}

static void answers(void) {
    make_level();
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_OK);
    CHECK(rac2_copy_check(&table, placed(1)) == RAC2_COPY_SCRIPTED); /* a set piece's */
    CHECK(rac2_copy_check(&table, placed(2)) == RAC2_COPY_SCRIPTED); /* the camera names it */
    CHECK(rac2_copy_check(&table, placed(3)) == RAC2_COPY_LISTED);   /* a spawner */
    CHECK(rac2_copy_check(&table, placed(4)) == RAC2_COPY_NOT_ENEMY);
    CHECK(rac2_copy_check(&table, placed(5)) == RAC2_COPY_SCRIPTED); /* a script slot */
    CHECK(rac2_copy_check(&table, placed(6)) == RAC2_COPY_SCRIPTED); /* names moby 4 */
    CHECK(rac2_copy_detail(&table, placed(1)) == RAC2_COPY_WHY_GROUP);
    CHECK(rac2_copy_detail(&table, placed(2)) == RAC2_COPY_WHY_NAMED);
    CHECK(rac2_copy_detail(&table, placed(5)) == RAC2_COPY_WHY_SCRIPT);
    CHECK(rac2_copy_detail(&table, placed(6)) == RAC2_COPY_WHY_NAMES);
    CHECK(rac2_copy_detail(&table, placed(0)) == 0);
    CHECK(rac2_copy_check(&table, placed(7)) == RAC2_COPY_OK);
    CHECK(rac2_copy_check(&table, placed(8)) == RAC2_COPY_NOT_PLACED); /* no reward */
    CHECK(rac2_copy_check(&table, placed(9)) == RAC2_COPY_OK);         /* no pvars */
    CHECK(rac2_copy_check(&table, placed(10)) == RAC2_COPY_LISTED);    /* Tabora's crystals */
    CHECK(rac2_copy_check(&table, placed(11)) == RAC2_COPY_LISTED);
    CHECK(rac2_copy_check(&table, DYNAMIC) == RAC2_COPY_NOT_PLACED);
    CHECK(rac2_copy_check(&table, BASE + 0x80u) == RAC2_COPY_UNKNOWN);
    /* The table: the bosses NG++ names among them, every class named and listed once. */
    const int16_t bosses[] =
        {2644,
         2701,
         2702,
         2552,
         2628,
         3210,
         3679,
         3973,
         4785,
         3961,
         2874,
         3402,
         3145,
         3806,
         3967,
         4623,
         4576};
    for (unsigned i = 0; i < sizeof bosses / sizeof bosses[0]; ++i) {
        CHECK(listed(bosses[i]));
    }
    /* Grelbin's moonstone droppers, as Tabora's crystal creatures (slots 10 and 11). */
    CHECK(listed(3927) && listed(4551));
    CHECK(listed(4304)); /* the yetis their spawner lets out */
    for (unsigned i = 0; i < rac2_copy_never_count; ++i) {
        CHECK(rac2_copy_never[i].what != NULL);
        for (unsigned j = i + 1; j < rac2_copy_never_count; ++j) {
            CHECK(rac2_copy_never[i].oclass != rac2_copy_never[j].oclass);
        }
    }
}

static void copies(void) {
    make_level();
    const gaddr orig = placed(0);
    gaddr made_copies[3];
    for (int k = 0; k < 3; ++k) {
        made_copies[k] = rac2_copy_make(&table, orig, k);
        CHECK(made_copies[k] != 0);
        const gaddr c = made_copies[k];
        const float x = GREF(float, c + 0x10u), y = GREF(float, c + 0x14u),
                    z = GREF(float, c + 0x18u);
        const float d = sqrtf((x - 10.0f) * (x - 10.0f) + (y - 10.0f) * (y - 10.0f));
        CHECK(d >= 2.5f - 1e-4f && d <= 4.0f + 1e-4f); /* beside it */
        CHECK(x < WALL_X);                             /* the wall's side it stands on */
        CHECK(z == 1.0f);                              /* its height over the floor */
        for (int j = 0; j < k; ++j) {
            const float dx = x - GREF(float, made_copies[j] + 0x10u);
            const float dy = y - GREF(float, made_copies[j] + 0x14u);
            CHECK(dx * dx + dy * dy >= 2.0f * 2.0f); /* apart */
        }
        /* The original's record, but what is its slot's own. */
        CHECK(GREF(int16_t, c + 0xAAu) == ENEMY && GREF(gaddr, c + 0x24u) == CLASS_ENEMY);
        CHECK(GREF(int16_t, c + 0xB2u) == 10 && GREF(int16_t, c + 0xB4u) == 20);
        CHECK(GREF(uint8_t, c + 0xB1u) == 0x01u);
        CHECK(GREF(uint32_t, c + 0xA4u) == 0x80807F7Fu);
        CHECK(GREF(uint8_t, c + 0x6Cu) == 0xFFu && GREF(uint8_t, c + 0x6Du) == 0xFFu);
        CHECK(GREF(uint32_t, c + 0xACu) == (PLACED + (uint32_t)k) << 16);
        /* Its pvars: the original's, in its slot's own 0x80 bytes. */
        const gaddr pv = GREF(gaddr, c + 0x68u);
        CHECK(pv == SLOT_PVARS_AT + 0x80u * (uint32_t)k);
        CHECK(memcmp(G(pv), G(blob_at(0)), 0x40u) == 0);
        CHECK(rac2_copy_is(c));
        CHECK(celled == c); /* in the moby cells */
        /* On its squad's list, after the members and the copies before it. */
        const gaddr list = GREF(gaddr, 0x001EE5A0u + 4u);
        CHECK(list != 0x00880000u);
        CHECK(GREF(uint16_t, list) == 0x0000u && GREF(uint16_t, list + 2u) == 0x0009u);
        for (int j = 0; j <= k; ++j) {
            const uint16_t want =
                (uint16_t)((made_copies[j] - BASE) / 0x100u) | (j == k ? 0x8000u : 0u);
            CHECK(GREF(uint16_t, list + 4u + 2u * (uint32_t)j) == want);
        }
    }
    CHECK(rac2_copy_count() == 3);
    CHECK(!rac2_copy_is(orig));
    /* Pvars larger than a slot's: in the port's room, the pointer into them moved with them. */
    const gaddr big = rac2_copy_make(&table, placed(7), 0);
    CHECK(big != 0);
    const gaddr pv = GREF(gaddr, big + 0x68u);
    CHECK(pv >= 0x01C00000u && pv < 0x01C00000u + ROOM_BYTES);
    CHECK(GREF(uint32_t, pv + 0x10u) == pv + 0x100u);
    CHECK(GREF(uint32_t, pv + 0x14u) == 0x00123456u);
    CHECK(GREF(uint32_t, pv + 0x18u) == GREF(uint32_t, blob_at(7) + 0x18u));
    /* None of what may not be copied is. */
    for (uint32_t i = 1; i < PLACED; ++i) {
        if (i != 7 && i != 9) {
            CHECK(rac2_copy_make(&table, placed(i), 0) == 0);
        }
    }
    /* No pvars: none in the copy either (its slot's own, zeroed). */
    CHECK(rac2_copy_make(&table, placed(9), 0) != 0);
}

static void no_spot(void) {
    make_level();
    /* An original with no ground under it: no copy. */
    putf(placed(0) + 0x10u, 200.0f);
    CHECK(rac2_copy_make(&table, placed(0), 0) == 0);
    /* A pack around it out to 4 units: a spot in a ring farther out. */
    make_level();
    for (uint32_t i = 0; i < 40; ++i) {
        const gaddr m = func_0029EFD0(ENEMY);
        const float a = 0.157f * (float)i;
        putf(m + 0x10u, 10.0f + 3.25f * cosf(a));
        putf(m + 0x14u, 10.0f + 3.25f * sinf(a));
        putf(m + 0x18u, 1.0f);
    }
    const gaddr far = rac2_copy_make(&table, placed(0), 0);
    CHECK(far != 0);
    const float fx = GREF(float, far + 0x10u) - 10.0f, fy = GREF(float, far + 0x14u) - 10.0f;
    CHECK(fx * fx + fy * fy > 4.0f * 4.0f && fx * fx + fy * fy <= 8.5f * 8.5f + 1e-3f);
    /* A pack all around it, 2.5 units apart: a spot still, a unit from every one of them. */
    make_level();
    for (float x = -7.5f; x <= 7.5f; x += 2.5f) {
        for (float y = -7.5f; y <= 7.5f; y += 2.5f) {
            const gaddr m = func_0029EFD0(ENEMY);
            CHECK(m != 0);
            putf(m + 0x10u, 10.0f + x);
            putf(m + 0x14u, 10.0f + y);
            putf(m + 0x18u, 1.0f);
        }
    }
    const gaddr close = rac2_copy_make(&table, placed(0), 0);
    CHECK(close != 0);
    for (gaddr m = DYNAMIC; m < END; m += 0x100u) {
        if (m == close || GREF(uint8_t, m + 0x20u) >= 0x80u) {
            continue;
        }
        const float dx = GREF(float, close + 0x10u) - GREF(float, m + 0x10u);
        const float dy = GREF(float, close + 0x14u) - GREF(float, m + 0x14u);
        CHECK(dx * dx + dy * dy >= 1.0f);
    }
}

/* A moby that names others: a placed one (named alike by the copy), its own spawned parts (copied
 * with it), or a spawned moby another one names too (no copy). */
static void parts(void) {
    make_level();
    put32(blob_at(0) + 0x20u, placed(4));
    put32(blob_at(0) + 0x24u, placed(0));
    gaddr c = rac2_copy_make(&table, placed(0), 0);
    CHECK(c != 0);
    CHECK(GREF(gaddr, GREF(gaddr, c + 0x68u) + 0x20u) == placed(4));
    CHECK(GREF(gaddr, GREF(gaddr, c + 0x68u) + 0x24u) == c); /* itself: the copy */
    /* A torso it spawned, naming it back, and an arm the torso names. */
    make_level();
    const gaddr torso = func_0029EFD0(THING);
    const gaddr arm = func_0029EFD0(THING);
    GREF(uint8_t, torso + 0x20u) = 1;
    GREF(uint8_t, arm + 0x20u) = 1;
    putf(torso + 0x10u, 10.0f);
    putf(torso + 0x14u, 10.0f);
    putf(torso + 0x18u, 2.0f);
    put32(blob_at(0) + 0x28u, torso);
    put32(GREF(gaddr, torso + 0x68u) + 0x70u, placed(0));
    put32(GREF(gaddr, torso + 0x68u) + 0x10u, arm);
    put32(GREF(gaddr, arm + 0x68u) + 0x8u, 0x12345678u);
    /* Lists in their pvars (a joint modifier's node, named by the record's +0x64). */
    put32(placed(0) + 0x64u, blob_at(0) + 0x30u);
    put32(torso + 0x64u, GREF(gaddr, torso + 0x68u) + 0x40u);
    put32(GREF(gaddr, torso + 0x68u) + 0x48u, GREF(gaddr, torso + 0x68u) + 0x40u);
    c = rac2_copy_make(&table, placed(0), 0);
    CHECK(c != 0);
    const gaddr torso2 = GREF(gaddr, GREF(gaddr, c + 0x68u) + 0x28u);
    CHECK(torso2 != torso && torso2 >= DYNAMIC && torso2 < END);
    CHECK(GREF(int16_t, torso2 + 0xAAu) == THING && GREF(uint8_t, torso2 + 0x20u) == 1);
    const gaddr tv = GREF(gaddr, torso2 + 0x68u);
    CHECK(tv != GREF(gaddr, torso + 0x68u));
    CHECK(GREF(gaddr, tv + 0x70u) == c); /* its copy of the legs */
    CHECK(GREF(gaddr, c + 0x64u) == GREF(gaddr, c + 0x68u) + 0x30u);
    CHECK(GREF(gaddr, torso2 + 0x64u) == tv + 0x40u && GREF(gaddr, tv + 0x48u) == tv + 0x40u);
    const gaddr arm2 = GREF(gaddr, tv + 0x10u);
    CHECK(arm2 != arm && arm2 >= DYNAMIC && GREF(int16_t, arm2 + 0xAAu) == THING);
    CHECK(GREF(uint32_t, GREF(gaddr, arm2 + 0x68u) + 0x8u) == 0x12345678u);
    /* Moved with the copy. */
    CHECK(GREF(float, torso2 + 0x10u) == GREF(float, c + 0x10u));
    CHECK(GREF(float, torso2 + 0x18u) == 2.0f);
    /* The original's parts are left as they were. */
    CHECK(GREF(gaddr, GREF(gaddr, torso + 0x68u) + 0x70u) == placed(0));
    CHECK(GREF(gaddr, GREF(gaddr, torso + 0x68u) + 0x10u) == arm);
    CHECK(GREF(gaddr, blob_at(0) + 0x28u) == torso);
    /* Another moby names the torso too: no copy. */
    const gaddr shared = func_0029EFD0(THING);
    GREF(uint8_t, shared + 0x20u) = 1;
    put32(GREF(gaddr, shared + 0x68u) + 0x4u, torso);
    CHECK(rac2_copy_make(&table, placed(0), 1) == 0);
}

static void reserve(void) {
    make_level();
    /* The pool as the game leaves it: given-back slots, then the never-used mark and zeros past. */
    for (gaddr m = DYNAMIC + 100u * 0x100u; m < END; m += 0x100u) {
        GREF(uint8_t, m + 0x20u) = 0;
    }
    GREF(uint8_t, DYNAMIC + 100u * 0x100u + 0x20u) = 0xFFu;
    /* The pool keeps RESERVE slots free for the level's own spawns. */
    const uint32_t take = DYNAMIC_SLOTS - RESERVE - 1u;
    for (uint32_t i = 0; i < take; ++i) {
        CHECK(func_0029EFD0(THING) != 0);
    }
    CHECK(rac2_copy_make(&table, placed(0), 0) != 0);
    CHECK(rac2_copy_make(&table, placed(0), 1) == 0);
}

static void stale_and_new_load(void) {
    make_level();
    const gaddr copy = rac2_copy_make(&table, placed(0), 0);
    CHECK(copy != 0 && rac2_copy_count() == 1);
    /* The level loaded again after a death: the pool laid out again, the copies gone and
     * forgotten; the answers kept from the first scan, as the level is laid out alike. */
    memset(G(copy), 0, 0x100u);
    GREF(uint8_t, copy + 0x20u) = 0xFFu;
    CHECK(rac2_copy_count() == 0);
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_OK);
    CHECK(rac2_copy_make(&table, placed(0), 0) != 0 && rac2_copy_count() == 1);
    /* The file's memory reused after the scan: the copies keep what the scan read. */
    memset(G(FILE_AT), 0, 0x6000u);
    CHECK(rac2_copy_make(&table, placed(7), 0) != 0);
    /* A file no longer the level's at the first ask: nothing may be copied, and it is read again
     * a tick later. */
    make_level();
    put32(FILE_AT + 0x4000u + 8u * 5u, 0x00777000u);
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_UNKNOWN);
    CHECK(rac2_copy_make(&table, placed(0), 0) == 0);
    put32(FILE_AT + 0x4000u + 8u * 5u, blob_at(5));
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_UNKNOWN);
    put32(0x001B19C8u, 1);
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_OK);
    /* The file read as LoadLevelData returns: what the rest of the load does to its memory after
     * changes nothing (Tabora's reuses it). */
    make_level();
    CHECK(func_L00_00300618(0) == 0x00900000u && gameplay_loads > 0);
    memset(G(FILE_AT), 0x5A, 0x6000u);
    put32(0x001B19C8u, 7);
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_OK);
    CHECK(rac2_copy_check(&table, placed(1)) == RAC2_COPY_SCRIPTED);
    CHECK(rac2_copy_make(&table, placed(7), 0) != 0);
    /* No file at all. */
    make_level();
    put32(0x001BF140u + 0x0Cu, 0);
    CHECK(rac2_copy_check(&table, placed(0)) == RAC2_COPY_UNKNOWN);
}

static void death(void) {
    make_level();
    const gaddr copy = rac2_copy_make(&table, placed(0), 0);
    CHECK(copy != 0);
    const gaddr nibble = 0x0019B4A8u + (10u >> 1) + 0x400u * MAP;
    const gaddr collected = 0x00210D40u + 0x464u + 10u;
    const gaddr checkpoint = 0x002119B0u + 0x464u + 10u;
    GREF(uint8_t, nibble) = 0x31u;
    /* A copy's death: the game's own (its drop, its count), its marks on the save id put back. */
    deaths = 0;
    func_002A6C58(copy, 0, 0);
    CHECK(deaths == 1);
    /* Its death again (a class that runs it twice): nothing more. */
    func_002A6C58(copy, 0, 0);
    CHECK(deaths == 1);
    CHECK(
        GREF(uint8_t, nibble) == 0x31u && GREF(uint8_t, collected) == 0
        && GREF(uint8_t, checkpoint) == 0
    );
    /* The original's death: the game's marks stay. */
    func_002A6C58(placed(0), 0, 0);
    CHECK(deaths == 2);
    CHECK(
        GREF(uint8_t, nibble) == 0x32u && GREF(uint8_t, collected) == 2
        && GREF(uint8_t, checkpoint) == 2
    );
    /* A copy freed, its slot another moby's: that one's death is the game's. */
    func_0029F338(copy);
    CHECK(!rac2_copy_is(copy));
}

/* The moby pool: a request adds spare slots to the gameplay file a running loader named, once and
 * only to that file; the status counts the level's pool. */
static void pool(void) {
    make_level();
    const gaddr file = 0x00A00000u;
    put32(file + 0x4Cu, 0x100u);
    put32(file + 0x100u, 12);
    put32(file + 0x104u, 200);
    put32(LOAD_CONTEXT + CONTEXT_FILE, file);
    /* No request: the file as it is. */
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    rac2_pool_loaded(0);
    CHECK(GREF(int32_t, file + 0x104u) == 200);
    /* A request of 100, 1,000 is cut to the most; only the loader's file is raised. */
    CHECK(rac2_pool_raise(100) == 100);
    rac2_pool_loading();
    rac2_wad_decompressed(file + 0x1000u);
    CHECK(GREF(int32_t, file + 0x104u) == 200);
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 300);
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 300);
    rac2_pool_loaded(0);
    CHECK(rac2_pool_raise(5000) == RAC2_POOL_EXTRA_MAX);
    put32(file + 0x104u, 200);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + RAC2_POOL_EXTRA_MAX);
    rac2_pool_loaded(0);
    /* The indices stay under 15 bits: placed 12 and spare 0x7FF0 leave room for 3. */
    put32(file + 0x104u, 0x7FF0);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 0x7FF3);
    rac2_pool_loaded(0);
    /* The status of the level's table: 200 spare, the copies' and the level's own alive. */
    int32_t out[5];
    CHECK(rac2_pool_status(out) == 1);
    CHECK(
        out[0] == (int32_t)DYNAMIC_SLOTS && out[1] + out[2] <= out[0]
        && out[4] == RAC2_POOL_EXTRA_MAX
    );
}

/* The heap's ceiling: a level's first load runs without the raise and measures the room below the
 * classes above it; the memory is put back and the load runs again with the raise that fits, and
 * the level's next loads fit at once. */
static void pool_ceiling(void) {
    make_level();
    const gaddr file = 0x00A00000u;
    const gaddr start = 0x00300000u;
    put32(file + 0x4Cu, 0x100u);
    put32(file + 0x100u, 12);
    put32(file + 0x104u, 200);
    put32(LOAD_CONTEXT + CONTEXT_FILE, file);
    put32(LOAD_CONTEXT + 0x48u, start);
    CHECK(rac2_pool_raise(RAC2_POOL_EXTRA_MAX) == RAC2_POOL_EXTRA_MAX);
    /* Room for all below the lowest class above the heap (0x500000): measured, then all of it. */
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200);
    CHECK(rac2_pool_loaded(0x00480000u) == 1);
    rac2_pool_restore();
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + RAC2_POOL_EXTRA_MAX);
    CHECK(rac2_pool_loaded(0x00480000u + RAC2_POOL_EXTRA_MAX * 0x180u) == 0);
    /* Another level's file (its own counts): room for 160 below 0x500000 less the gap; what the
     * measure's load wrote is put back. */
    put32(file + 0x100u, 13);
    put32(file + 0x104u, 200);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200);
    GREF(uint8_t, 0x00520000u) = 0x5Au;
    CHECK(rac2_pool_loaded(0x004F0000u) == 1);
    rac2_pool_restore();
    CHECK(GREF(uint8_t, 0x00520000u) == 0);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + 160);
    CHECK(rac2_pool_loaded(0x004F0000u + 160u * 0x180u) == 0);
    /* Its load again (a death): what fits at once. */
    put32(file + 0x104u, 200);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + 160);
    CHECK(rac2_pool_loaded(0x004F0000u + 160u * 0x180u) == 0);
    /* A level with no room: measured, and no load again; its next load adds none. */
    put32(file + 0x100u, 14);
    put32(file + 0x104u, 200);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(rac2_pool_loaded(0x004FF800u) == 0);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200);
    CHECK(rac2_pool_loaded(0x004FF800u) == 0);
    /* A raise that crossed the ceiling all the same (a level grown since): laid out again. */
    put32(file + 0x100u, 13);
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + 160);
    CHECK(rac2_pool_loaded(0x004F4000u + 160u * 0x180u) == 1);
    rac2_pool_restore();
    rac2_pool_loading();
    rac2_wad_decompressed(file);
    CHECK(GREF(int32_t, file + 0x104u) == 200 + 117);
    CHECK(rac2_pool_loaded(0x004F4000u + 117u * 0x180u) == 0);
}

/* A dead copy's slot given out again: its index comes off its squad's list, the rest stay. */
static void slot_taken(void) {
    make_level();
    const gaddr a = rac2_copy_make(&table, placed(0), 0);
    const gaddr b = rac2_copy_make(&table, placed(0), 1);
    CHECK(a != 0 && b != 0);
    const gaddr list = GREF(gaddr, 0x001EE5A0u + 4u);
    const uint16_t ia = (uint16_t)((a - BASE) / 0x100u), ib = (uint16_t)((b - BASE) / 0x100u);
    CHECK(GREF(uint16_t, list + 4u) == ia && GREF(uint16_t, list + 6u) == (ib | 0x8000u));
    /* A live moby's slot that no list names: the lists are left alone. */
    const gaddr other = func_0029EFD0(THING);
    rac2_copy_slot_taken(other);
    CHECK(GREF(uint16_t, list + 4u) == ia && GREF(uint16_t, list + 6u) == (ib | 0x8000u));
    /* The last copy dies and a bolt takes its slot: the one before it ends the list. */
    func_0029F338(b);
    CHECK(func_0029EFD0(THING) == b);
    rac2_copy_slot_taken(b);
    CHECK(GREF(uint16_t, list) == 0x0000u && GREF(uint16_t, list + 2u) == 0x0009u);
    CHECK(GREF(uint16_t, list + 4u) == (ia | 0x8000u));
    /* The first dies too: only the members are left, the last with its mark. */
    func_0029F338(a);
    CHECK(func_0029EFD0(THING) == a);
    rac2_copy_slot_taken(a);
    CHECK(GREF(uint16_t, list) == 0x0000u && GREF(uint16_t, list + 2u) == 0x8009u);
}

int main(void) {
    openrac_guest_base = calloc(1, openrac_guest_ram_size);
    CHECK(openrac_guest_base != NULL);
    answers();
    copies();
    no_spot();
    parts();
    reserve();
    stale_and_new_load();
    slot_taken();
    death();
    pool();
    pool_ceiling();
    puts("RAC2 enemy copies: OK");
    return 0;
}
