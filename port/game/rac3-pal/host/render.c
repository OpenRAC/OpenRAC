/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: the game's own renderers, which the native port does not run. As Going
 * Commando's (rac2/host/render.c) and Ratchet & Clank's (rac1/host/render_native.c): on the console
 * they are hand-written assembly that turns level data into VU1 and GS packets; the port draws the
 * same things with its own renderers from the extracted level (extractor/extract.py port, --levels).
 * The frame's draw pass (DrawFrame, Veldin's 0x00403440, Going Commando's func_0027BD70) still runs:
 * its passes end here, where the window is told which layers the game drew, with what camera, and
 * where the world goes in the display list.
 *
 * Every level's program carries its own copy of the engine. These are written with Veldin's copy
 * (the names rac3-decomp's catalogue gives the levels' places, hostgen.json "places"), and its
 * globals go through OPENRAC_LDATA(1, ...) to the loaded level's. The multiplayer maps (levels 39 to
 * 53) share most of them, but DrawTfrag, the tie and shrub passes and MobyProc have copies of their
 * own there, named after level 39's places, whose globals go through OPENRAC_LDATA(39, ...). Each
 * was paired with Going Commando's function instruction by instruction
 * (port/game/rac3-pal/IDENTIFIED.tsv): DrawFrame's calls line up one for one with Going Commando's.
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "game/common/lib/effect_texture.h"
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* Shared with the multiplayer maps' copies (mp_maps.c), which pass their own globals. */
int rac3_sky_draw_shell(int shell, gaddr matrix);
int rac3_part_proc(gaddr pool_word, gaddr highest_word, gaddr table);
void rac3_world_quad(gaddr record, gaddr matrix, gaddr uploads, gaddr records, gaddr list);

/* A global of Veldin's program, in the program loaded now. */
#define VELDIN(a) OPENRAC_LDATA(1, (gaddr)(a))

/* The globals a copy of the renderers reads, at the addresses of the level it is written with
 * (`from`), relocated to the loaded level's (OPENRAC_LDATA). */
typedef struct Copy {
    int from;
    /* The display list's write pointer (DrawTfrag reads it first; VU1_addGSregister appends
     * through it). */
    gaddr list_pointer;
    /* BuildViewMatrices's rotation rows (the profile's camera_rows): the view block 0x160 before
     * them, the eye 0x20 before them. */
    gaddr camera_rows;
    /* The view context: +0xB0 and +0xB4 the tangents of half the field of view. */
    gaddr view_context;
    /* MobyProc's inputs: the cap on a moby's draw distance ($s7 from $gp) and the group masks it
     * copies to the scratchpad (Going Commando's 0x001B1B74 and 0x001BADC0). */
    gaddr moby_max_distance;
    gaddr moby_groups;
} Copy;

/* Veldin's (the single-player levels'): MobyProc's cap is $gp + 0x37A0. */
static const Copy kVeldin =
    {1,
     RAC3_LADDR(1, 0x001E1068u),
     RAC3_LADDR(1, 0x001E29E0u),
     RAC3_LADDR(1, 0x001E7580u),
     RAC3_LADDR(1, 0x001E0050u),
     RAC3_LADDR(1, 0x001EA840u)};
/* The multiplayer menu's (the multiplayer maps'): MobyProc's cap is $gp + 0x3FD0, the maps' own
 * $gp 0x00247FC0 (config/gp.tsv; lw s7 at 0x005B2780), not the executable's 0x001DC8B0. */
static const Copy kMultiplayer =
    {39,
     RAC3_LADDR(39, 0x0024CAD0u),
     RAC3_LADDR(39, 0x0024DE20u),
     RAC3_LADDR(39, 0x0025D6C0u),
     RAC3_LADDR(39, 0x0024BF90u),
     RAC3_LADDR(39, 0x0025F580u)};

/* The boot program's (the loading screen between planets): its globals are no level's, not
 * relocated. MobyProc 0x003CB798 reads its cap at $gp - 0x23E0 (0x001DA4D0) and its group masks
 * from its own occlusion mask, D_00227480 (GameProfile::occlusion_mask). */
static const Copy kBoot =
    {-1,
     RAC3_ADDR(0x001DA050u),
     RAC3_ADDR(0x00222420u),
     RAC3_ADDR(0x00225900u),
     RAC3_ADDR(0x001DA4D0u),
     RAC3_ADDR(0x00227480u)};

static gaddr at_copy(const Copy* c, gaddr address) {
    return c->from < 0 ? address : OPENRAC_LDATA(c->from, address);
}

/* The sky: its header (+0x06 the shell count, +0x20 the shells) and the matrix the shell being
 * drawn takes (0x004A7E10 builds it from the shell's angles before each 0x004A8158). */
#define SKY_MATRIX RAC3_LADDR(1, 0x001E0E10u)

/* The frame recorder's place for what follows (openrac/frame.h, docs/port/FRAME_RECORDER.md): the
 * display list's write pointer, the word at `pointer`, where the console's packets would have gone.
 * Nothing is written into the list; the window finds that place among what the list still draws.
 * Any list pointer, the boot program's too. */
static void anchor_at(gaddr pointer) {
    const gaddr at = GREF(gaddr, pointer);
    openrac_frame_anchor(at < 0x02000000u ? at : 0);
}

static void anchor(const Copy* c) {
    anchor_at(at_copy(c, c->list_pointer));
}

/* DrawTfrag (Going Commando's func_002EF3B0, the same 208 bytes): the world's first native pass
 * after the sky, so the world's marker goes here. */
static void draw_tfrag(const Copy* c) {
    openrac_game_draw(OPENRAC_DRAW_TERRAIN);
    anchor(c);
    openrac_frame_world(OPENRAC_DRAW_TERRAIN);
}

int func_L01_004B1728(void) {
    draw_tfrag(&kVeldin);
    return 0;
}

int func_L39_005F4690(void) {
    draw_tfrag(&kMultiplayer);
    return 0;
}

/* The tie pass of DrawFrame (func_002F0EF8). */
int func_L01_004B3238(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
    return 0;
}

int func_L39_005F61A0(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
    return 0;
}

/* The shrub pass of DrawFrame (func_002E11C0). */
int func_L01_004A59F8(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
    return 0;
}

int func_L39_005EDE28(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
    return 0;
}

/* SkyDrawShell(shell) (func_002E3940): the window draws the shell with the matrix the sky code
 * built for it. The sky is the world's first pass, so its camera is the world's. */
int rac3_sky_draw_shell(int shell, gaddr matrix) {
    openrac_game_world_view();
    float m[16];
    memcpy(m, G(matrix), sizeof(m));
    openrac_game_sky_shell(shell, m);
    openrac_game_draw(OPENRAC_DRAW_SKY);
    return 0;
}

int func_L01_004A8158(int shell) {
    return rac3_sky_draw_shell(shell, VELDIN(SKY_MATRIX));
}

/* The boot program's SkyDrawShell (0x003CE798, frontbin's func_003CE3E0, the levels' 0x004A8158):
 * the loading screen's sky, the stars and the galaxy, three shells of the scene pack's sky
 * (0x003BBDC0 hands it to LoadSky, 0x00398A48; SkyDraw 0x003CE568 turns each into the matrix at
 * 0x001DA610 before drawing it). Its triangles went out as the game's GS packets, which the window
 * draws in the 4:3 middle of a wide window, while the ship and its trail filled it. Drawn natively
 * instead, from the scene's export (level 97's sky): around the camera with the window's own view,
 * so a wider window shows more of the sky, as it shows more of the levels' skies. Leaving a planet
 * the level's copy (0x004A8158 above) draws the same sky, which the level's LoadSky copy took. */
void func_003CE798(int shell) {
    if (shell < GREF(int16_t, GREF(gaddr, RAC3_ADDR(0x001DA5F0u)) + 6u)) {
        rac3_sky_draw_shell(shell, RAC3_ADDR(0x001DA610u));
    }
}

/*
 * MobyProc's drawn flag (+0x31), as the console sets it (Going Commando's mark_drawn_mobys,
 * rac2/host/render.c): group mask, mode bits 0x81, alpha below 8, draw distance (+0x32, capped)
 * and the game's 4:3 view. It is game logic, not only drawing (the moby loop updates a drawn moby,
 * the lock-on drops one not drawn), so it is the console's test whatever the window draws.
 */
static void mark_drawn_mobys(const Copy* c, gaddr first, int count) {
    float eye[3], block[12], rows[12], tx, ty;
    memcpy(eye, G(at_copy(c, c->camera_rows - 0x20u)), sizeof(eye));
    memcpy(block, G(at_copy(c, c->camera_rows - 0x160u)), sizeof(block));
    for (int c = 0; c < 3; ++c) {
        rows[c] = block[c * 4 + 2];
        rows[4 + c] = -block[c * 4 + 0];
        rows[8 + c] = -block[c * 4 + 1];
    }
    const gaddr view_context = at_copy(c, c->view_context);
    memcpy(&tx, G(view_context + 0xB0), 4);
    memcpy(&ty, G(view_context + 0xB4), 4);
    if (!(tx > 0.05f && tx < 10.0f)) {
        tx = 0.63f;
    }
    if (!(ty > 0.05f && ty < 10.0f)) {
        ty = tx * 0.756f;
    }
    /* The game fix fix_wide_moby_effects (differences.json) opens the view as the port's picture and
     * the player's Field of View open it, as Going Commando's: a moby at the sides of a picture wider
     * than 4:3 is drawn, and makes its effects, as on a wider console picture. */
    static const unsigned char* fix;
    if (!fix) {
        fix = openrac_game_fix("fix_wide_moby_effects");
    }
    const float opened = *fix ? openrac_game_view_opened() : 1.0f;
    tx *= *fix ? openrac_game_picture_wider() * opened : 1.0f;
    ty *= opened;
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    const gaddr end = first + (gaddr)count * 0x100u;
    const int max_distance = GREF(int, at_copy(c, c->moby_max_distance));
    const uint8_t* groups = G(at_copy(c, c->moby_groups));
    if (first == 0 || first >= 0x02000000u) {
        return;
    }
    for (gaddr m = first; (count < 0 || m != end) && m - first < 0x100u * 4096u; m += 0x100) {
        uint8_t* b = G(m);
        const int8_t state = (int8_t)b[0x20];
        if (state == -1) {
            break;
        }
        if (state < 0) {
            continue;
        }
        int drawn = (b[0x36] & groups[b[0x37]]) != 0 && (*(uint16_t*)(b + 0x34) & 0x81) == 0
                    && b[0x23] >= 8;
        if (drawn) {
            float sphere[4];
            memcpy(sphere, b, sizeof(sphere));
            const float r = sphere[3] / 1024.0f;
            const float d[3] =
                {sphere[0] / 1024.0f - eye[0],
                 sphere[1] / 1024.0f - eye[1],
                 sphere[2] / 1024.0f - eye[2]};
            int distance = *(int16_t*)(b + 0x32);
            if (distance > max_distance) {
                distance = max_distance;
            }
            const float z = d[0] * rows[0] + d[1] * rows[1] + d[2] * rows[2];
            const float x = d[0] * rows[4] + d[1] * rows[5] + d[2] * rows[6];
            const float y = d[0] * rows[8] + d[1] * rows[9] + d[2] * rows[10];
            drawn = z <= (float)distance && z + r > 32.0f / 1024.0f && fabsf(x) - r * kx <= tx * z
                    && fabsf(y) - r * ky <= ty * z;
        }
        b[0x31] = (uint8_t)(drawn ? 1 : 0);
    }
}

/* MobyProc (func_002A4E68, hand-written): the moby list `a0`, `a2` mobys of it (-1 the whole
 * list, the world's draw). */
static void moby_proc(const Copy* c, gaddr list, int count) {
    mark_drawn_mobys(c, list, count);
    anchor(c);
    if (count < 0) {
        openrac_frame_world_mobys();
    }
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    openrac_frame_mobys(list, count);
}

int func_L01_00441B78(int a0, int a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    moby_proc(&kVeldin, (gaddr)a0, a2);
    return 0;
}

/* The boot program's MobyProc (0x003CB798, hand-written; frontbin's 0x003CB3E0), run by the loading
 * screen between planets for the ship's scene (0x003C3B38, 0x003C3C18): the drawn flags as the
 * console sets them, as Going Commando's func_002A4E68 and the levels' copies above. Its draw side
 * (0x003CB6B8, below) puts the world's markers and draws them natively. Run as the game's, it built
 * VU1 packets whose vertices the draw side's jobs fill (0x003C81D8 from the scratchpad), which the
 * port does not run: their space kept what the list held two passes before, the window's reading
 * of the list lost its place in that stale data, and every other pass the planet's name after it
 * was dropped (the name flickered). It returns the job list's end (the caller keeps it at
 * 0x001DA498): no job queued, as it came. */
unsigned int func_003CB798(int a0, unsigned int a1, int a2, int a3) {
    (void)a3;
    mark_drawn_mobys(&kBoot, (gaddr)a0, a2);
    return a1;
}

/* The same routine as 0x003C3C18 calls it (the catalogue's place of its own): the whole table. */
int func_003CB798_003C3C18(int a0, int a1, int a2, int a3) {
    return (int)func_003CB798(a0, (unsigned int)a1, a2, a3);
}

/* frontbin's copy (0x003CB3E0), run by its own loading screen when a game starts from frontbin
 * (0x003C3780, and 0x003C3860 for the whole table): the boot program's routine, but for the
 * addresses of its packet buffers, which this does not build. The globals it reads are frontbin's at
 * the boot program's addresses on both discs (rac3-decomp's config/us/names.tsv: D_001DA4D0,
 * D_00222420, D_00225900, D_00227480). */
unsigned int func_003CB3E0__frontbin(int a0, unsigned int a1, int a2, int a3) {
    return func_003CB798(a0, a1, a2, a3);
}

int func_L39_005B25C8(int a0, int a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    moby_proc(&kMultiplayer, (gaddr)a0, a2);
    return 0;
}

/* The moby textures' DMA (func_002A4AD8) and MobyAnimProc's draw side (func_002A4D88; the boot
 * program's copy is 0x003CB6B8 below). */
int func_L01_004417E8(int a0) {
    (void)a0;
    return 0;
}

int func_L01_00441A98(int a0, int a1) {
    (void)a0;
    (void)a1;
    return 0;
}

/* moby_anim_eval_chain (func_002A4078): the pose of the joints marked in a1, into the scratchpad. */
int func_L01_00440090(int a0, int a1) {
    openrac_game_moby_chain((gaddr)a0, (gaddr)a1);
    return 0;
}

/* The boot program's copy (the loading screen's ship) and frontbin's: the same evaluator, where
 * retail streamed the joints into the scratchpad by a DMA tag chain. */
int func_003CA9C0(int a0, int a1) {
    openrac_game_moby_chain((gaddr)a0, (gaddr)a1);
    return 0;
}

int func_003CA608__frontbin(int a0, int a1) {
    openrac_game_moby_chain((gaddr)a0, (gaddr)a1);
    return 0;
}

/*
 * PartProc (func_002B9420, the same instructions): each live record of the pool (0x40 bytes, byte
 * 1 bit 7 a free one), slots 0 to the highest in use, to the window, as Going Commando's. Its
 * texture is entry +0x02 of the table PartProc sends to the scratchpad (Veldin's 0x00220A40, Going
 * Commando's 0x001F1F40): the low half the palette's address << 4, the high half the pixels'
 * address << 4 with the log2 of the side.
 */
#define PARTICLE_POOL RAC3_LADDR(1, 0x001E0278u)
#define PARTICLE_HIGHEST RAC3_LADDR(1, 0x001E0280u)
#define PARTICLE_TEXTURES RAC3_LADDR(1, 0x00220A40u)

int rac3_part_proc(gaddr pool_word, gaddr highest_word, gaddr table) {
    const gaddr pool = GREF(gaddr, pool_word);
    const int highest = GREF(int, highest_word);
    if (pool == 0 || pool >= 0x01FE0000u) {
        return 0;
    }
    for (int i = 0; i <= highest && i < 2048; ++i) {
        const gaddr at = pool + (gaddr)i * 0x40;
        const uint8_t* r = G(at);
        if ((r[1] & 0x80) != 0) {
            continue;
        }
        uint64_t entry;
        memcpy(&entry, G(table + (gaddr)r[2] * 8), 8);
        openrac_game_particle(
            r,
            (uint32_t)(entry >> 32) >> 4,
            (uint32_t)entry >> 4,
            (int)((entry >> 32) & 0xF),
            0x80000000u | ((at >> 6) & 0x7FFFFu) << 8 | r[0]
        );
    }
    return 0;
}

int func_L01_00461CB8(void) {
    return rac3_part_proc(
        VELDIN(PARTICLE_POOL), VELDIN(PARTICLE_HIGHEST), VELDIN(PARTICLE_TEXTURES)
    );
}

/*
 * The screen glow (Up Your Arsenal's own; Going Commando has none): DrawFrame, while the word
 * 0x001DBCA0 is set, appends two prebuilt GS packets by DMA reference. 0x004DE670 the first
 * (0x001D0A50, built by 0x004DDE98: the frame at block 0x1E00 copied into a 256 x 256 buffer at
 * FBP 0x198 and shrunk into FBP 0 and 0x1FD), 0x004DE6F0 the second (0x001D2A50, built by
 * 0x004DE5B0: blurred and added back over the frame, ALPHA 0x221, FIX 0x80). Replayed over the
 * native world they drew a ghost of the picture and a copy of it in its top left corner. The port
 * leaves them out: the enhanced renderer's bloom (enhanced/docs/bloom.md) is its glow. The word
 * each sets for the frame (0x6000 at $gp - 0xC0C, 0 again after) is kept.
 */
#define GLOW_WORD RAC3_LADDR(1, 0x001DBCA4u)

int func_L01_004DE670(void) {
    GREF(int32_t, VELDIN(GLOW_WORD)) = 0x6000;
    return 0;
}

int func_L01_004DE6F0(void) {
    GREF(int32_t, VELDIN(GLOW_WORD)) = 0;
    return 0;
}

/* The second screen effect (0x004DF310, while 0x001DBCBC is set; DrawFrame clears it after): the
 * same family, two more prebuilt packets, 0x001D3650 (0x004DF070: the frame copied and blurred
 * twice) and 0x001D3EF0 (0x004DF1B0: tinted and added back over the frame). Left out as the glow
 * is. */
int func_L01_004DF310(void) {
    return 0;
}

/* The draw callback running and its argument (wide_view.c), the owner of the quads it draws. */
extern gaddr rac3_draw_owner;
extern gaddr rac3_draw_callback;
extern unsigned rac3_draw_ordinal;

/* The trail record whose quad DrawWorldQuad draws now (func_003D98C0 below), 0 for none. */
static gaddr s_trail_record;

/* The effect textures' uploads this frame: their count and the records GetEffectTex (Veldin's
 * 0x00404040, Going Commando's func_0027C878) writes, 16 bytes each as Going Commando's. */
#define TEXTURE_UPLOADS RAC3_LADDR(1, 0x001DF97Cu)
#define TEXTURE_RECORDS RAC3_LADDR(1, 0x001E7880u)

/*
 * DrawWorldQuad (0x00407E28, Going Commando's func_00280B90, the same 608 bytes): the draw
 * callbacks' world quad. a0 a quad record as Going Commando's: four corners (+0x00), four RGBA words
 * (+0x40), four ST pairs (+0x50), then CLAMP_1, TEX0_1, TEX1_1 and ALPHA_1 (+0x70..+0x88); a1, when
 * set, a matrix the corners go through first. The window draws it in the world, with the TEST and
 * ZBUF in effect at its marker (rac2/host/render.c's world_quad).
 */
/* The loading flight's state (0x002D2008 in the boot program, the level's copy of it on a flight
 * that leaves a planet: loading.c), 0 outside a flight. Its +0 is the destination's picture, which
 * the transition's start (0x003C10B0) unpacked for the level asked for (0x003BC940), and +8 the
 * TEX0 its planet callback (0x003C1660) took for it (PictureTex0, 0x0039B088: the CLUT at +0x20,
 * the 8-bit pixels at +0x420 of the picture). */
gaddr rac3_loading_state(void); /* loading.c */

/* The planet's quad: drawn from its picture itself, not from whichever texture of the frame's
 * paging records names its blocks. */
static void flight_picture(openrac_game_quad* q) {
    const gaddr state = rac3_loading_state();
    if (state == 0 || q->tex0 == 0 || GREF(uint64_t, state + 8u) != q->tex0) {
        return;
    }
    const gaddr picture = GREF(gaddr, state);
    const uint32_t width = 1u << ((q->tex0 >> 26) & 0xFu), height = 1u << ((q->tex0 >> 30) & 0xFu);
    if (picture < 0x00100000u || picture + 0x420u + width * height > openrac_guest_ram_size) {
        return;
    }
    q->clut = picture + 0x20u;
    q->pixels = picture + 0x420u;
    q->source = 1;
}

void rac3_world_quad(gaddr record, gaddr matrix, gaddr uploads, gaddr records, gaddr list) {
    const uint8_t* r = G(record);
    openrac_game_quad q;
    memset(&q, 0, sizeof(q));
    memcpy(q.corner, r, sizeof(q.corner));
    if (matrix != 0) {
        float m[16];
        memcpy(m, G(matrix), sizeof(m));
        for (int k = 0; k < 4; ++k) {
            const float x = q.corner[k][0], y = q.corner[k][1], z = q.corner[k][2],
                        w = q.corner[k][3];
            for (int i = 0; i < 4; ++i) {
                q.corner[k][i] = x * m[i] + y * m[4 + i] + z * m[8 + i] + w * m[12 + i];
            }
        }
    }
    memcpy(q.rgba, r + 0x40, sizeof(q.rgba));
    memcpy(q.st, r + 0x50, sizeof(q.st));
    memcpy(&q.clamp, r + 0x70, 8);
    memcpy(&q.tex0, r + 0x78, 8);
    memcpy(&q.tex1, r + 0x80, 8);
    memcpy(&q.alpha, r + 0x88, 8);
    q.source = openrac_effect_texture(q.tex0, GREF(int, uploads), records, &q.pixels, &q.clut);
    flight_picture(&q);
    /* A draw callback's quad: its owner (the callback's argument, a moby whose body the quad may sit
     * on: frontend scene_effects.cpp), and an id that is the same from one tick to the next while the
     * callback draws the same quads (its function, its argument and the quad's place in what it
     * draws), so that a frame between ticks moves it from the last tick's corners. */
    if (rac3_draw_owner != 0) {
        q.parent = rac3_draw_owner;
        uint32_t h = 2166136261u;
        const uint32_t words[3] = {rac3_draw_callback, rac3_draw_owner, rac3_draw_ordinal++};
        for (int i = 0; i < 3; ++i) {
            h = (h ^ words[i]) * 16777619u;
        }
        q.id = 0x40000000u | (h & 0x3FFFFFFFu);
    } else if (s_trail_record != 0) {
        /* A puff of the ship's trail: its record's place is the same from one tick to the next
         * while it lives (a freed one used again moves too far to be blended: the frontend's
         * kMaximumEffectStep). */
        q.id = 0x20000000u | ((s_trail_record >> 4) & 0x1FFFFFFFu);
    }
    openrac_game_effect_quad(&q);
    const uint32_t marker = openrac_game_effect_quad_marker();
    if (marker != 0) {
        anchor_at(list);
        openrac_frame_effect_quad(marker);
    }
}

int func_L01_00407E28(int a0, int a1) {
    rac3_world_quad(
        (gaddr)a0,
        (gaddr)a1,
        VELDIN(TEXTURE_UPLOADS),
        VELDIN(TEXTURE_RECORDS),
        at_copy(&kVeldin, kVeldin.list_pointer)
    );
    return 0;
}

/* The boot program's effect textures' uploads (its GetEffectTex, 0x00389978, and GetFrameTexture,
 * 0x00396E18): their count and their 16-byte records, as the levels' above. */
#define BOOT_TEXTURE_UPLOADS RAC3_ADDR(0x001D9BFCu)
#define BOOT_TEXTURE_RECORDS RAC3_ADDR(0x00225C00u)

/* The boot program's list pointer (0x003A97F8, its VU1_addGSregister, appends there). */
#define BOOT_LIST_POINTER RAC3_ADDR(0x001DA050u)

/* The boot program's DrawWorldQuad (0x003D3158, frontbin's func_003D2DA0, the levels' 0x00407E28):
 * the loading screen's planet, a picture on a quad the ship flies towards, which its draw callback
 * (0x003C1660, the planet's, from the flight's last part) draws with the planet's texture
 * (0x003BCA28). Drawn natively as the levels' quads, in the loading screen's world. */
void func_003D3158(int a0, int a1, int a2, int a3) {
    (void)a2;
    (void)a3;
    rac3_world_quad(
        (gaddr)a0, (gaddr)a1, BOOT_TEXTURE_UPLOADS, BOOT_TEXTURE_RECORDS, BOOT_LIST_POINTER
    );
}

/* The ship's cockpit glass on the loading screen (0x003A7EF8, the boot program's copy of Going
 * Commando's render_environment_mapped_object, func_002ECB70): not in the hull's model, but 38 world
 * quads the game writes each tick at the place joint 0 of the ship (its argument, class 3412) has
 * that tick, called by the flight (0x003C1848 from the planet's draw callback 0x003C1660) and as the
 * ship's draw callback (0x003A6C90). The boot program's callback lists are not run by wide_view.c,
 * so its quads had no owner and no id: a frame between ticks drew the hull part way from the last
 * tick and the glass where this tick has it, and at the loading screen's 20 ticks a second the
 * glass slid off the ship. Owned by the ship and with ids, they move from the last tick's corners
 * as the hull does (frontend scene_effects.cpp), as Going Commando's (rac2/host/ship_glass.c). */
void func_003A7EF8(gaddr a0) {
    const gaddr outer = rac3_draw_owner, outer_callback = rac3_draw_callback;
    const unsigned outer_ordinal = rac3_draw_ordinal;
    rac3_draw_owner = a0;
    rac3_draw_callback = 0; /* no callback runner's function: the ids are the ship's alone */
    rac3_draw_ordinal = 0;
    func_003A7EF8__game(a0);
    rac3_draw_owner = outer;
    rac3_draw_callback = outer_callback;
    rac3_draw_ordinal = outer_ordinal;
}

/* The ship's engine flames (0x003A6B90, the boot program's copy of the catalogue's frontbin row
 * func_003A6790): two rows of world quads, one an engine (0x003A66D0, through DrawWorldQuad's copy
 * 0x003D98C0), laid along each engine from where its joint is that tick (0x003C3278), called by the
 * ship's draw. Without an owner they had no id and no parent: a frame between ticks drew the hull
 * part way from the last tick and the flames where this tick has them, so in the approach to the
 * planet (20 ticks a second, loading.c) the exhaust stood still for a tick and jumped at the next
 * while the ship moved smoothly ahead of it (OPENRAC_MOTION_LOG: about 75 to 135 quads a tick, none
 * with an id). Owned by the ship, they move with it between ticks (frontend scene_effects.cpp). */
void func_003A6B90(gaddr a0) {
    const gaddr outer = rac3_draw_owner, outer_callback = rac3_draw_callback;
    const unsigned outer_ordinal = rac3_draw_ordinal;
    rac3_draw_owner = a0;
    rac3_draw_callback = 0; /* no callback runner's function: the ids are the ship's alone */
    rac3_draw_ordinal = 0;
    func_003A6B90__game(a0);
    rac3_draw_owner = outer;
    rac3_draw_callback = outer_callback;
    rac3_draw_ordinal = outer_ordinal;
}

/* The levels' copy of the engine flames (frontbin's func_003A6790, Veldin's 0x004AD768, 34 levels),
 * which a level's own flight leaving a planet runs: owned by the ship as the boot program's above. */
void func_L01_004AD768(gaddr a0) {
    const gaddr outer = rac3_draw_owner, outer_callback = rac3_draw_callback;
    const unsigned outer_ordinal = rac3_draw_ordinal;
    rac3_draw_owner = a0;
    rac3_draw_callback = 0;
    rac3_draw_ordinal = 0;
    func_L01_004AD768__game(a0);
    rac3_draw_owner = outer;
    rac3_draw_callback = outer_callback;
    rac3_draw_ordinal = outer_ordinal;
}

/* The ship's trail on the loading screen: 0x003A8850 draws each live record of a pool of 512 (0x50
 * bytes each, the pool at *0x001D6EEC, +0x48 set while it lives) as one quad through 0x003D98C0,
 * the boot program's copy of DrawWorldQuad's caller. They are world-space puffs left behind the
 * ship, which move, grow and fade each tick: drawn with no id, a frame between ticks left them where
 * the tick had them, and in the approach (20 ticks a second) the trail stepped behind the smoothly
 * moving ship (OPENRAC_MOTION_LOG: up to about 70 quads a tick without an id). Each now has its
 * record's place as its id, so that a frame between ticks moves it from the last tick's corners. */
#define TRAIL_POOL RAC3_ADDR(0x001D6EECu)
#define TRAIL_RECORDS 512u
#define TRAIL_RECORD_SIZE 0x50u

void func_003D98C0(gaddr a0, int a1, int a2, int a3, float a4, float a5, float a6) {
    const gaddr outer = s_trail_record;
    const gaddr pool = GREF(gaddr, TRAIL_POOL);
    s_trail_record = rac3_draw_owner == 0 && pool != 0 && a0 >= pool
                             && a0 < pool + TRAIL_RECORDS * TRAIL_RECORD_SIZE
                         ? a0
                         : 0;
    func_003D98C0__game(a0, a1, a2, a3, a4, a5, a6);
    s_trail_record = outer;
}

/* The levels' copy of the cockpit glass (the catalogue's frontbin row func_003A7AF8, Veldin's
 * 0x004B04D8, the 34 single-player levels), which the level's own flight runs leaving a planet
 * (its copy of 0x003C1848): owned by the ship and with ids as the boot program's above, else its
 * quads stayed where each tick left them while the hull moved between ticks. */
void func_L01_004B04D8(gaddr a0) {
    const gaddr outer = rac3_draw_owner, outer_callback = rac3_draw_callback;
    const unsigned outer_ordinal = rac3_draw_ordinal;
    rac3_draw_owner = a0;
    rac3_draw_callback = 0;
    rac3_draw_ordinal = 0;
    func_L01_004B04D8__game(a0);
    rac3_draw_owner = outer;
    rac3_draw_callback = outer_callback;
    rac3_draw_ordinal = outer_ordinal;
}

/* What the console's renderers needed each frame (Going Commando's func_002F0468, 0x002EF310,
 * 0x002F3E78, 0x002F0E38, 0x002E2C38, 0x002E10B8, 0x0029FEE0): the texture packets, and the baked
 * lights of terrain, ties and shrubs, which the extracted level carries. */
int func_L01_004B27B8(int a0) {
    (void)a0;
    return 0;
}

int func_L01_004B1688(void) {
    return 0;
}

int func_L01_004B6ED0(void) {
    return 0;
}

int func_L01_004B3178(void) {
    return 0;
}

int func_L01_004A75E0(void) {
    return 0;
}

int func_L01_004A58F0(void) {
    return 0;
}

int func_L01_0043AAC0(void) {
    return 0;
}

/* 0x003CB6B8 (boot_elf, hand-written; Going Commando's func_002A4D88, MobyAnimProc's draw side):
 * draws the jobs the boot program's MobyProc queued (a list in the display list buffer, *0x001D9D30,
 * each job's packets through 0x003C81D8 and 0x003D5A20 with VU0 and the scratchpad), called by
 * 0x003C39A8 in the level loading screen's pass (0x003C3BB8, the boot program in mode 6), after its
 * stars and before its text. Those mobys are the boot program's table's (Ratchet's ship, class
 * 3412, its parts and the planet): the window draws them natively, with the boot program's camera,
 * from the scene's export (level 97, GameProfile::loading_world), where the world's marker and the
 * mobys' put them in the list, over the stars. */
void func_003CB6B8(int list, int count) {
    (void)list;
    (void)count;
    openrac_game_world_view();
    anchor_at(BOOT_LIST_POINTER);
    openrac_frame_world(OPENRAC_DRAW_MOBYS);
    openrac_frame_world_mobys();
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
}

/* frontbin's copy of the draw side (0x003CB300), called by its level loading screen's pass
 * (0x003C35F0): translated from its assembly, its jobs reached the scratchpad's DMA (0x003C7E20),
 * which the port does not copy, and the game stopped as New Game from frontbin started the level.
 * As the boot program's. */
void func_003CB300__frontbin(int list, int count) {
    func_003CB6B8(list, count);
}
