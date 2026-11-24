/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * A mod for mod_loader_test.cpp, built as mods are (a shared library against
 * sdk/include/openrac/openrac_mod.h alone), twice: as test-a and test-b (TEST_MOD_ID). Its
 * settings say what it does: "behaviour" (ok, fail, unknown_hook, bad_signature, input, views) and
 * "tag", a number it adds to what it returns and reports through the test's function
 * "Test_Record". With "input" it also takes the keyboard, asks for the cursor captured, and writes
 * into its pad reports whether W is held and how far the mouse moved (interface 1.2). With "views"
 * it asks for one view a frame and makes the Vulkan device, and reports each frame's end through
 * Test_Record (2000 + the views ended so far) and its closing (4000) (interface 1.4). With "menu"
 * it adds a front-end menu entry whose page draws a rectangle and a line of text each frame and
 * closes on Cross, and draws outside its page too (ignored), and asks for an entry after load
 * (refused): Test_Record gets 1100 + that refusal (interface 1.5). With "savestate" it takes the
 * tick boundary and, in it, reports the tick (3000 + it), the snapshot's size (3100 + it) and a
 * refused save and load (3200 and 3300 + the refusal), and asks for the boundary after load
 * (refused: 1200 + that) and a save outside it (refused: 1300 + that) (interface 1.6). With
 * "game_ui" it makes two pages of the game's own menus (the second asking before going back), puts
 * the first in the main menu after Load Game with a button, a line and a choice on it, and reports
 * each event (5000 + 100 * page + 10 * kind + item + 1); Cross on the first item opens the second
 * page, which asks a question when it shows and plays a sound, and an answer is 6000 + 10 * yes +
 * the tag; a page and an item after load are refused (1400 and 1500 + that) (interface 1.7). With
 * "play" it uses the play calls on the test's game, a step a frame (play_frame: 7000s), and
 * starts a game with save 0 and later travels (interface 1.8). With "world" it takes the gate and
 * the hit watcher and uses the world calls on the test's fake level (world_frame: 8000s, and the
 * gate's and the watcher's 8700s and 8800s) (interface 1.9). With "hoverboard" it uses the calls of
 * a mod that moves the hero (motion_frame: 9500s): his pose set, the collision triangles, a prop
 * and a sound (interface 1.12). With "cutscene" it uses what a scene needs (cutscene_frame:
 * 9800s): the camera's field of view, the HUD hidden, the music ducked, a moby's joints, a joint's
 * place, a moby's sound, the scene's air, slow motion and a hidden tie. It reports the interface it is
 * built for
 * (OPENRAC_MOD_TARGET_MINOR): test-old is built for 1.5. */
#include <math.h>
#include <stddef.h>
#include <string.h>

#ifndef TEST_MOD_ID
#define TEST_MOD_ID "test-a"
#endif

#include "openrac/openrac_mod.h"

static const openrac_api* s_api;
static openrac_mod* s_mod;
static int (*s_add)(int);
static void (*s_record)(int);
static int s_tag;
static int s_frames;
static int s_input;
static int s_views;
static int s_ended;
static int s_menu;
static int s_savestate;
static int s_developer_menu;
static int s_ui;
static int s_ui_second = -1;
static int s_play;
static int s_world;
static int s_motion;
static int s_cutscene;
static int s_prop = -1;
static int s_model = -1;
static int s_figure = -1;

/* The menu page: a white rectangle and "HELLO" each frame; Cross closes it. */
static int menu_page(void* user, const openrac_menu_frame* frame) {
    (void)user;
    s_api->menu_rect(s_mod, 10, 20, 30, 40, OPENRAC_RGBA(255, 255, 255, 255));
    s_api->menu_text(
        s_mod, 1, 2, 3, OPENRAC_RGBA(255, 0, 0, 255), frame->opened ? "HELLO" : "AGAIN"
    );
    return (frame->pressed & OPENRAC_PAD_CROSS) != 0 ? OPENRAC_MENU_CLOSE : OPENRAC_MENU_STAY;
}

/* The pages of the game's own menus: each event reported; the first page's first item opens the
 * second, which asks when it shows; its second opens the first itself (the main menu's item's page,
 * interface 1.9). */
static void ui_page(void* user, int page, const openrac_ui_event* event) {
    (void)user;
    s_record(5000 + 100 * page + 10 * (int)event->kind + event->item + 1);
    if (event->kind == OPENRAC_UI_CHOOSE && event->item == 0) {
        s_record(5900 + -s_api->ui_open(s_mod, s_ui_second));
    } else if (event->kind == OPENRAC_UI_CHOOSE && event->item == 1) {
        s_record(5800 + -s_api->ui_open(s_mod, 0));
    } else if (event->kind == OPENRAC_UI_SHOW && page == s_ui_second) {
        s_api->ui_confirm(s_mod, page, "Leave the party?", 7);
        s_api->ui_sound(s_mod, OPENRAC_UI_SOUND_SELECT);
    } else if (event->kind == OPENRAC_UI_ANSWER) {
        s_record(6000 + 10 * event->value + (int)event->tag);
    }
}

OPENRAC_MOD_EXPORT void openrac_mod_query(openrac_mod_info* info) {
    info->api_major = OPENRAC_MOD_API_MAJOR;
    info->api_minor = OPENRAC_MOD_TARGET_MINOR;
    info->id = TEST_MOD_ID;
    info->version = "1.0.0";
}

static int add(int x) {
    return s_add(x) * 10 + s_tag;
}

/* The tick boundary: the snapshot calls are taken here, and the test's game has no snapshot. */
static void on_tick_boundary(void* user, uint32_t tick) {
    (void)user;
    if (s_developer_menu) {
        const uint32_t action = s_api->take_developer_menu_action(s_mod);
        if (action != 0) {
            s_record(9000 + (int)action);
            s_record(9100 + s_api->developer_menu_set_label(s_mod, action, "Saved: Veldin"));
            /* Interface 1.19: the state the menu left the switch or the choice in. */
            s_record(9200 + s_api->developer_menu_value(s_mod, action));
            /* The mod sets its own switch here, as it changed elsewhere. */
            s_record(9300 + -s_api->developer_menu_switch(s_mod, 3, 0));
        }
        return;
    }
    char bytes[16] = {0};
    s_record(3000 + (int)tick);
    s_record(3100 + (int)s_api->savestate_size());
    s_record(3200 + -s_api->savestate_save(bytes, sizeof bytes));
    s_record(3300 + -s_api->savestate_load(bytes, sizeof bytes));
}

/* Play (interface 1.8), a step a frame: in the front end (frame 1) the status, a figure refused,
 * save 0 read and started with (twice: the second waits for the first); in a level (frame 2) a
 * figure made and posed, the hero's pose, what he holds and his shots (1.9), a travel and a level's
 * name; after a new load (frame 3) the figure is gone. */
static void play_frame(void) {
    openrac_play_status status = {sizeof status, 0, 0, 0, 0};
    openrac_pose pose = {0};
    openrac_save_info info = {sizeof info, 0, 0, 0, 0};
    openrac_held held;
    openrac_shot shots[4];
    unsigned char save[16];
    size_t size;
    pose.size = sizeof pose;
    memset(&held, 0, sizeof held);
    held.size = sizeof held;
    memset(shots, 0, sizeof shots);
    s_record(7000 + 10 * -s_api->play_status(&status) + status.phase);
    if (s_frames == 1) {
        s_record(7100 + -s_api->figure_create(s_mod));
        s_record(7200 + 10 * s_api->save_slot(0, &info) + info.level);
        s_record(7200 + 10 * (s_api->save_slot(9, &info) + 2));
        size = s_api->save_read(0, save, sizeof save);
        s_record(7300 + (int)size);
        s_record(7400 + -s_api->play_start(s_mod, -1, save, size));
        s_record(7400 + -s_api->play_start(s_mod, 2, NULL, 0));
        s_record(7400 + -s_api->play_travel(s_mod, 2));
        if (s_api->api_minor >= 11) {
            /* 1.11: the slot the game saves into, and the mod's own folder (named for its id). */
            const char* dir = s_api->data_dir(s_mod);
            const size_t n = dir != NULL ? strlen(dir) : 0;
            s_record(7500 + s_api->save_slot_current());
            s_record(7600 + (n >= 6 && strcmp(dir + n - 6, "test-a") == 0));
        }
    } else if (s_frames == 2) {
        s_figure = s_api->figure_create(s_mod);
        s_record(7500 + s_figure);
        s_record(7600 + -s_api->figure_pose(s_mod, s_figure + 1, &pose));
        pose.state = 42;
        s_record(7600 + -s_api->figure_pose(s_mod, s_figure, &pose));
        s_record(7700 + -s_api->hero_pose(&pose) + pose.state);
        /* Interface 1.9: what the hero holds, a figure made to hold it (not another's id), the
         * hero's shots, a figure firing one (a weapon the game cannot fire for it refused). */
        s_record(7720 + -s_api->hero_held(&held) + held.count);
        s_record(7730 + -s_api->figure_held(s_mod, s_figure, &held));
        s_record(7730 + -s_api->figure_held(s_mod, s_figure + 1, &held));
        s_record(7740 + s_api->hero_shots(shots, 4) + 2 * shots[0].item);
        s_record(7760 + -s_api->figure_shot(s_mod, s_figure, &shots[0]));
        shots[0].item = 3;
        s_record(7760 + -s_api->figure_shot(s_mod, s_figure, &shots[0]));
        s_record(7800 + -s_api->play_travel(s_mod, 4));
        s_record(
            7900 + (s_api->level_name(1) != NULL && s_api->level_name(1)[0] == 'N')
            + 2 * (s_api->level_name(99) == NULL)
        );
        if (s_api->api_minor >= 32) {
            /* Interface 1.33: a game the manifest does not declare is refused (-6), an id that is not a
             * game's is 0. */
            char folder[512];
            s_record(7970 + -s_api->game_data_dir(s_mod, "rac2", folder, sizeof folder));
            s_record(7980 + -s_api->game_data_dir(s_mod, "../up", folder, sizeof folder));
        }
    } else if (s_frames == 3) {
        s_record(7600 + -s_api->figure_pose(s_mod, s_figure, &pose));
        s_record(7730 + -s_api->figure_held(s_mod, s_figure, &held));
        s_record(7600 + -s_api->figure_remove(s_mod, s_figure));
    } else if (s_frames == 4) {
        /* Interface 1.9: a travel from the level's menu (a party's member taken along). */
        s_record(7810 + -s_api->play_travel(s_mod, 3));
    }
}

/* The gate (interface 1.9): around an update it reports the slot (8700 + slot before, 8720 + slot
 * after); before one it has the hero stand in elsewhere (8710 + the refusal, 0 accepted) and skips
 * the update of slot 2. */
static int world_gate(void* user, int slot, int phase) {
    const float where[3] = {1.0f, 2.0f, 3.0f};
    (void)user;
    if (phase == OPENRAC_MOBY_AFTER) {
        s_record(8720 + slot);
        return 0;
    }
    s_record(8700 + slot);
    s_record(8710 + -s_api->hero_stand_in(s_mod, where, -1));
    return slot == 2;
}

/* The hit watcher: reports victim and attacker (8800 + 10 * victim + attacker + 1) and takes back
 * what hurts more than 5. */
static int world_hit(void* user, const openrac_hit* hit) {
    (void)user;
    s_record(8800 + 10 * hit->victim + hit->attacker + 1);
    return hit->damage > 5.0f;
}

/* Interface 1.9, on the test's fake level (3 live mobys, slots 0 to 2, keys 0, 1 and none), a step
 * a frame (8000s). Frame 1: the list (count, keys, flags, a short buffer), slots by key, drives
 * (a dead slot refused, the flag shown), poses, kills, a spawn (driven), the hero's slot, eyes,
 * bolts, a stand-in outside the gate, a hit posted, and the gate and the watcher asked for after
 * load (refused); the level's scenes (none playing, one started and then playing with its frames,
 * a request dropped). Frame 2 (the level loaded again): no drive left. */
static void world_frame(void) {
    openrac_moby list[8];
    openrac_moby_deletion deletion = {sizeof deletion, 0, 0, 0, 0, 0};
    float camera[3] = {0};
    openrac_moby pose = {0};
    openrac_hit hit = {sizeof hit, 1, 0, 4, 3.0f, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
    const float eyes[24] = {0};
    const float where[3] = {0};
    int i, n, frame = -1;
    for (i = 0; i < 8; i++) {
        list[i].size = sizeof list[i];
    }
    pose.size = sizeof pose;
    if (s_frames == 1) {
        n = s_api->moby_list(list, 8);
        s_record(8000 + n);
        for (i = 0; i < n; i++) {
            s_record(8010 + list[i].key + 1);
            s_record(8020 + (int)list[i].flags);
        }
        s_record(8030 + s_api->moby_list(list, 2));
        s_record(8040 + s_api->moby_slot(1));
        s_record(8050 + s_api->moby_slot(7) + 1);
        s_record(8055 + (s_api->moby_uid(1) == 1006) + (s_api->moby_uid(5) == -1));
        s_record(8060 + -s_api->moby_drive(s_mod, 0, 1));
        s_record(8060 + -s_api->moby_drive(s_mod, 0, 0));
        s_record(8060 + -s_api->moby_drive(s_mod, 1, 1));
        s_record(8070 + -s_api->moby_drive(s_mod, 5, 1));
        s_api->moby_list(list, 8);
        s_record(
            8080 + (int)(list[0].flags & OPENRAC_MOBY_DRIVEN)
            + (int)(list[1].flags & OPENRAC_MOBY_DRIVEN)
        );
        pose.position[0] = 4.0f;
        pose.scale = 2.0f;
        s_record(8100 + -s_api->moby_pose(s_mod, 1, &pose));
        s_record(8110 + -s_api->moby_pose(s_mod, 5, &pose));
        s_record(8120 + -s_api->moby_kill(s_mod, 2));
        s_record(8130 + -s_api->moby_kill(s_mod, 5));
        s_record(8140 + s_api->moby_spawn(s_mod, 77, &pose));
        n = s_api->moby_list(list, 8);
        s_record(8160 + n);
        s_record(8170 + (int)(list[n - 1].flags & OPENRAC_MOBY_DRIVEN));
        /* Copies (1.25): slot 1 may be copied, slot 0 is listed; a copy of 1, none of a dead slot. */
        s_record(8150 + s_api->moby_copy_check(s_mod, 1));
        s_record(8155 + s_api->moby_copy_check(s_mod, 0));
        s_record(8250 + s_api->moby_copy(s_mod, 1, 2));
        s_record(8260 + s_api->moby_copy(s_mod, 7, 0));
        s_record(8180 + s_api->figure_slot(s_mod, -1));
        s_record(8190 + s_api->figure_slot(s_mod, 0) + 1);
        s_record(8200 + -s_api->play_eyes(s_mod, eyes, 2));
        s_record(8210 + -s_api->play_eyes(s_mod, eyes, 20));
        s_record(8220 + -s_api->play_eyes(s_mod, NULL, 0));
        s_record(8300 + s_api->bolts(5) - 100);
        s_record(8320 + -s_api->play_overlay_pause(s_mod, 1));
        s_record(8330 + -s_api->play_skip(s_mod));
        s_record(8340 + -s_api->play_notice(s_mod, "Waiting for the other players to arrive"));
        s_record(8350 + s_api->scene_playing(&frame) + 1 + frame);
        s_record(8360 + -s_api->play_scene(s_mod, 3));
        s_record(8370 + s_api->scene_playing(&frame));
        s_record(8380 + (frame == 42));
        s_record(8390 + -s_api->play_scene(s_mod, -1));
        s_record(8600 + -s_api->moby_hide(s_mod, 1, 1));
        s_record(8610 + -s_api->moby_hide(s_mod, 5, 1));
        s_record(8620 + -s_api->moby_deletion(2, &deletion));
        s_record(8630 + deletion.by);
        s_record(8640 + deletion.by_mod);
        s_record(8650 + -s_api->moby_deletion(1, &deletion));
        s_record(8660 + -s_api->camera_position(camera));
        s_record(8670 + (int)camera[0]);
        s_record(8400 + -s_api->hero_stand_in(s_mod, where, -1));
        s_record(8500 + -s_api->moby_hit(s_mod, &hit));
        hit.victim = 5;
        s_record(8500 + -s_api->moby_hit(s_mod, &hit));
        s_record(8900 + -s_api->on_moby_update(s_mod, world_gate, NULL));
        s_record(8910 + -s_api->on_moby_hit(s_mod, world_hit, NULL));
    } else if (s_frames == 2) {
        n = s_api->moby_list(list, 8);
        s_record(8600 + n);
        s_record(8610 + (int)(list[1].flags & OPENRAC_MOBY_DRIVEN));
    }
}

/* Interface 1.12, a step a frame. Frame 1 (in a level): the hero's pose set (the test's game
 * names Test_Add, which the mod holds, as the hero's update), the collision triangles (3, two
 * read), a prop made (one outside the mod's folder and one missing refused) and posed at (1, 2, 3)
 * at scale 2, a second prop of it fixed to joint 5 of the moby in slot 3, then to the hero's attach
 * point 1 (interface 1.18; no moby in slot 4 and no attach point 2 refused), a sound loaded (a
 * missing one refused) and played, its voice changed and stopped.
 * Frame 2 (another hook point named, the level loaded again): the pose refused, the prop gone, the
 * same mesh asked for again. */
static void motion_frame(void) {
    openrac_pose pose;
    openrac_collision_triangle tris[2];
    openrac_prop_pose where;
    float skin[32] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
                      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 0, 0, 1};
    float bad[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    openrac_moby_model mm;
    volatile float zero = 0.0f;
    const float lo[3] = {0.0f, 0.0f, 0.0f};
    const float hi[3] = {8.0f, 8.0f, 8.0f};
    uint32_t voice;
    int sound;
    memset(&pose, 0, sizeof pose);
    pose.size = sizeof pose;
    memset(tris, 0, sizeof tris);
    memset(&where, 0, sizeof where);
    where.size = sizeof where;
    where.position[0] = 1.0f;
    where.position[1] = 2.0f;
    where.position[2] = 3.0f;
    where.rotation[3] = 1.0f;
    where.scale = 2.0f;
    if (s_frames == 1) {
        pose.position[0] = 5.0f;
        s_record(9500 + -s_api->hero_set_pose(s_mod, &pose));
        s_record(9510 + s_api->collision_triangles(lo, hi, tris, 2) + 10 * (int)tris[1].type);
        s_prop = s_api->prop_create(s_mod, "board.glb");
        s_record(9540 + s_prop);
        s_record(9540 + -s_api->prop_create(s_mod, "../test-b/board.glb"));
        s_record(9540 + -s_api->prop_create(s_mod, "missing.glb"));
        s_record(9550 + -s_api->prop_pose(s_mod, s_prop, &where));
        s_record(9550 + -s_api->prop_pose(s_mod, s_prop + 1, &where));
        s_api->prop_animate(s_mod, s_prop, 2, 1.5f, 1);
        {
            const float offset[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0.5f, 0, 0, 1};
            const int held = s_api->prop_create(s_mod, "board.glb");
            s_record(9660 + -s_api->prop_attach(s_mod, held, 3, 5, 0, offset));
            s_record(9660 + -s_api->prop_attach(s_mod, held, 4, 5, 0, NULL));
            s_record(9670 + -s_api->prop_attach(s_mod, held, 3, 1, OPENRAC_ATTACH_HELD, offset));
            s_record(9670 + -s_api->prop_attach(s_mod, held, 3, 2, OPENRAC_ATTACH_HELD, NULL));
            {
                /* Where they were drawn: the posed one at its pose; the fixed one not drawn on
                 * its joint yet; another mod's not ours. */
                float at[16];
                memset(at, 0, sizeof at);
                s_record(9680 + -s_api->prop_frame(s_mod, s_prop, at) + (at[12] == 1.0f ? 0 : 5));
                s_record(9680 + -s_api->prop_frame(s_mod, held, at));
                s_record(9680 + -s_api->prop_frame(s_mod, s_prop + 2, at));
            }
        }
        /* Interface 1.33: the prop's skin (two joints: the bind pose, and 3 along x), the refusals
         * (a prop that is not there, a value that is not a number, too many joints, no matrices),
         * the skin taken off and put on again. */
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, skin, 2));
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop + 2, skin, 2));
        bad[0] = 0.0f / zero;
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, bad, 1));
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, skin, OPENRAC_PROP_JOINTS_MAX + 1));
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, NULL, 2));
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, NULL, 0));
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, skin, 2));
        sound = s_api->sound_load(s_mod, "beep.wav");
        s_record(9560 + sound);
        s_record(9560 + -s_api->sound_load(s_mod, "missing.wav"));
        voice = s_api->sound_play(s_mod, sound, 0.5f, 2.0f, 1);
        s_record(9570 + (int)voice);
        s_record(9580 + -s_api->sound_set(NULL, voice, 1.0f, 1.0f)); /* not the mod's voice */
        s_record(9580 + -s_api->sound_set(s_mod, voice, 1.0f, 1.0f));
        s_api->sound_stop(s_mod, voice);
        s_record(9580 + -s_api->sound_set(s_mod, voice, 1.0f, 1.0f));
        {
            openrac_haze haze[2];
            openrac_camera cam;
            memset(haze, 0, sizeof haze);
            haze[0].position[0] = 4.0f;
            haze[0].radius = 1.0f;
            haze[0].strength = 0.5f;
            haze[1].radius = -1.0f;
            s_record(9590 + -s_api->haze_set(s_mod, haze, 2)); /* a radius below 0 */
            haze[1].radius = 0.0f;
            s_record(9590 + -s_api->haze_set(s_mod, haze, 2));
            memset(&cam, 0, sizeof cam);
            cam.size = sizeof cam;
            cam.eye[0] = 6.0f;
            s_record(9590 + -s_api->camera_set(s_mod, &cam)); /* rows not unit vectors */
            cam.forward[0] = 1.0f;
            cam.left[0] = 1.0f;
            cam.up[2] = 1.0f;
            s_record(9590 + -s_api->camera_set(s_mod, &cam)); /* rows not at right angles */
            cam.left[0] = 0.0f;
            cam.left[1] = 1.0f;
            s_record(9590 + -s_api->camera_set(s_mod, &cam));
            memset(&cam, 0, sizeof cam);
            cam.size = sizeof cam;
            s_record(9600 + -s_api->camera_view(&cam) + (int)cam.eye[0]);
            /* Interface 1.33: a bad channel, amplitude or count is refused (-1) and never reaches the
             * host; a good one is handed on (0). */
            s_record(9670 + -s_api->camera_shake(s_mod, 2, 0.1f, 10));
            s_record(9670 + -s_api->camera_shake(s_mod, 0, 1.5f, 10));
            s_record(9670 + -s_api->camera_shake(s_mod, 1, 0.1f, 601));
            s_record(9670 + -s_api->camera_shake(s_mod, 0, 0.15f, 30));
        }
        if (s_api->api_minor >= 32) {
            /* Interface 1.33: the culling bypass. A value other than 0 or 1 is refused (-1), 1 is taken
             * in a level in play (0), 0 gives it back, and 1 again leaves it on for the host to read. */
            s_record(9820 + -s_api->camera_visibility_bypass(s_mod, 2));
            s_record(9820 + -s_api->camera_visibility_bypass(s_mod, 1));
            s_record(9820 + -s_api->camera_visibility_bypass(s_mod, 0));
            s_record(9820 + -s_api->camera_visibility_bypass(s_mod, 1));
        }
        if (s_api->api_minor >= 32) {
            /* Interface 1.33. The test game is not Ratchet & Clank, so reserving item IDs is unknown
             * (-1). An ammo panel offset past the range is refused (-1), one in range is taken (0) and
             * zero gives it back (0). A weapon the mod never registered is not granted (-6). */
            const uint32_t item = 36;
            s_record(9830 + -s_api->rac1_reserve_item_ids(s_mod, &item, 1));
            s_record(9840 + -s_api->rac1_ammo_panel_offset(s_mod, 3, 2000, 0));
            s_record(9840 + -s_api->rac1_ammo_panel_offset(s_mod, 3, 10, -20));
            s_record(9840 + -s_api->rac1_ammo_panel_offset(s_mod, 3, 0, 0));
            s_record(9850 + -s_api->rac1_weapon_grant(s_mod, 36));
        }
        {
            openrac_hud_text line;
            memset(&line, 0, sizeof line);
            strcpy(line.text, "KICKFLIP");
            line.x = 0.5f;
            line.y = 0.8f;
            line.scale = 0.1f;
            s_record(9610 + -s_api->hud_text_set(s_mod, &line, 1)); /* a scale too small */
            line.scale = 1.0f;
            s_record(9610 + -s_api->hud_text_set(s_mod, &line, 1));
        }
        s_record(9620 + s_api->hero_sound_play(s_mod, 26, 0));
        s_record(9630 + s_api->hero_sound_play(s_mod, 0, 1));
        /* Interface 1.33: a model of the mod's board.glb (asked for twice: the same handle; a path
         * outside the mod's folder and a missing file refused), the moby in slot 1 drawn with it,
         * and the refusals (a slot with no moby, a model that is not the mod's, unknown flags, a
         * short size, a value that is not a number, no struct); a handle that is not the mod's
         * finds the slot taken; cleared, cleared again, and set again with the game's joints. */
        s_model = s_api->model_create(s_mod, "board.glb");
        s_record(9700 + s_model);
        s_record(9710 + s_api->model_create(s_mod, "board.glb"));
        s_record(9720 + -s_api->model_create(s_mod, "../test-b/board.glb"));
        s_record(9720 + -s_api->model_create(s_mod, "missing.glb"));
        memset(&mm, 0, sizeof mm);
        mm.size = sizeof mm;
        mm.model = s_model;
        mm.matrix[0] = mm.matrix[5] = mm.matrix[10] = mm.matrix[15] = 1.0f;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        s_record(9730 + -s_api->moby_model_slot(s_mod, 5, &mm));
        mm.model = s_model + 5;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        mm.model = s_model;
        mm.flags = 2;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        mm.flags = 0;
        mm.size = 8;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        mm.size = sizeof mm;
        mm.matrix[1] = 0.0f / zero;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        mm.matrix[1] = 0.0f;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, NULL));
        s_record(9730 + -s_api->moby_model_slot((openrac_mod*)&s_model, 1, &mm));
        mm.model = -1;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        mm.model = s_model;
        mm.flags = 0;
        mm.matrix[12] = 2.0f;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        /* Interface 1.33: the mobys of class 511 in both scopes (the refusals: no scope, a scope
         * that is none of the port's, a class below 0, a model that is not the mod's, unknown
         * flags, no struct); another mod's handle finds the class taken in either scope while the
         * mod holds one; the menu scope given up, free for anyone. */
        mm.flags = 0;
        mm.matrix[12] = 0.0f;
        mm.matrix[13] = 3.0f;
        s_record(
            9740
            + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_WORLD | OPENRAC_MODEL_MENU, &mm)
        );
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, 0, &mm));
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, 4, &mm));
        s_record(9740 + -s_api->moby_model_class(s_mod, -1, OPENRAC_MODEL_WORLD, &mm));
        mm.model = s_model + 5;
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_WORLD, &mm));
        mm.model = s_model;
        mm.flags = 2;
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_WORLD, &mm));
        mm.flags = 0;
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_WORLD, NULL));
        mm.model = -1;
        s_record(
            9740 + -s_api->moby_model_class((openrac_mod*)&s_model, 511, OPENRAC_MODEL_WORLD, &mm)
        );
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_MENU, &mm));
        s_record(
            9740 + -s_api->moby_model_class((openrac_mod*)&s_model, 511, OPENRAC_MODEL_MENU, &mm)
        );
        s_record(
            9740
            + -s_api->moby_model_class(
                (openrac_mod*)&s_model, 511, OPENRAC_MODEL_WORLD | OPENRAC_MODEL_MENU, &mm
            )
        );
        /* Interface 1.33: the smoke with the game's texture 1 and blend 0x44 is made; a texture past
         * one byte, a spin rate past 1, a flag the port does not know and a null spawn are refused. */
        {
            static const float at[3] = {1.0f, 2.0f, 3.0f};
            static const float go[3] = {0.0f, 0.0f, 4.0f};
            openrac_particle_spawn sp = {0};
            sp.size = sizeof sp;
            sp.kind = OPENRAC_PARTICLE_SMOKE;
            sp.position[0] = at[0];
            sp.position[1] = at[1];
            sp.position[2] = at[2];
            sp.velocity[2] = go[2];
            sp.particle_size = 40000.0f;
            sp.life_ticks = 60;
            sp.color = 0x7F606060u;
            sp.texture = 1;
            sp.blend = 0x44;
            sp.rot = -0.0002f;
            s_record(9760 + -s_api->particle_spawn_ex(s_mod, &sp));
            sp.texture = 256;
            s_record(9760 + -s_api->particle_spawn_ex(s_mod, &sp));
            sp.texture = 1;
            sp.rot = 2.0f;
            s_record(9760 + -s_api->particle_spawn_ex(s_mod, &sp));
            sp.rot = -0.0002f;
            sp.flags = 2u;
            s_record(9760 + -s_api->particle_spawn_ex(s_mod, &sp));
            sp.flags = 0u;
            s_record(9760 + -s_api->particle_spawn_ex(s_mod, NULL));
        }
        /* Interface 1.33: the moby at 0x3F00 (class 511 to the test's game, not in the level's list) is drawn
         * with the model; the refusals: no moby there (0x4000, class -1), address 0, a model that is not the
         * mod's, unknown flags, a matrix value that is not a number and no struct. */
        {
            memset(&mm, 0, sizeof mm);
            mm.size = sizeof mm;
            mm.model = s_model;
            mm.matrix[0] = mm.matrix[5] = mm.matrix[10] = mm.matrix[15] = 1.0f;
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, &mm));
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x4000, &mm));
            s_record(9770 + -s_api->moby_model_at(s_mod, 0, &mm));
            mm.model = s_model + 5;
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, &mm));
            mm.model = s_model;
            mm.flags = 2;
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, &mm));
            mm.flags = 0;
            mm.matrix[1] = 0.0f / zero;
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, &mm));
            mm.matrix[1] = 0.0f;
            s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, NULL));
            if (s_api->api_minor >= 32) {
                mm.model = -1;
                mm.flags = OPENRAC_MODEL_HIDE_NATIVE;
                s_record(9770 + -s_api->moby_model_at(s_mod, 0x3F00, &mm));
            }
        }
        /* Interface 1.33: a texture of the game's format is registered and gets id 128; the same content
         * gets 128 again, other content 129; a null texture and a size too small are refused. Recorded as
         * 9790 + (id - 128) for an id, 9800 + -answer for a refusal. */
        if (s_api->api_minor >= 32) {
            static openrac_particle_texture pt;
            int id;
            memset(&pt, 0, sizeof pt);
            pt.size = sizeof pt;
            pt.indices[0] = 1;
            pt.clut[0] = 0x10;
            id = s_api->particle_texture_register(s_mod, &pt);
            s_record(
                id >= OPENRAC_PARTICLE_TEXTURE_FIRST ? 9790 + (id - OPENRAC_PARTICLE_TEXTURE_FIRST)
                                                     : 9800 + -id
            );
            id = s_api->particle_texture_register(s_mod, &pt);
            s_record(
                id >= OPENRAC_PARTICLE_TEXTURE_FIRST ? 9790 + (id - OPENRAC_PARTICLE_TEXTURE_FIRST)
                                                     : 9800 + -id
            );
            pt.indices[0] = 2;
            id = s_api->particle_texture_register(s_mod, &pt);
            s_record(
                id >= OPENRAC_PARTICLE_TEXTURE_FIRST ? 9790 + (id - OPENRAC_PARTICLE_TEXTURE_FIRST)
                                                     : 9800 + -id
            );
            pt.size = 4;
            s_record(9800 + -s_api->particle_texture_register(s_mod, &pt));
            pt.size = sizeof pt;
            s_record(9800 + -s_api->particle_texture_register(s_mod, NULL));
        }
    } else if (s_frames == 2) {
        openrac_joint_offset joints[2];
        s_record(9500 + -s_api->hero_set_pose(s_mod, &pose));
        s_record(9550 + -s_api->prop_pose(s_mod, s_prop, &where));
        /* The prop went with its level, and its skin with it. */
        s_record(9660 + -s_api->prop_set_skin(s_mod, s_prop, skin, 2));
        /* The joints, from a mod that no longer holds the hero's update: taken all the same. */
        memset(joints, 0, sizeof joints);
        joints[0].joint = 1;
        joints[0].rotation[3] = 1.0f;
        joints[1].joint = 2;
        joints[1].rotation[3] = 1.0f;
        s_record(9640 + -s_api->hero_set_joints(s_mod, joints, 2));
        s_record(9640 + -s_api->hero_set_joints(s_mod, joints, OPENRAC_JOINT_OFFSETS_MAX + 1));
        s_record(9650 + -s_api->hero_set_joints(s_mod, NULL, 0));
        /* The level was loaded again: the meshes of the last load's props are not this one's, so
         * the same mesh is asked for again (the port's list of them started again). */
        s_record(9660 + s_api->prop_create(s_mod, "board.glb"));
        /* The model went with its level too. */
        memset(&mm, 0, sizeof mm);
        mm.size = sizeof mm;
        mm.model = s_model;
        s_record(9730 + -s_api->moby_model_slot(s_mod, 1, &mm));
        s_record(9740 + -s_api->moby_model_class(s_mod, 511, OPENRAC_MODEL_WORLD, &mm));
    }
}

/* What a scene needs, on frame 1 in a level: the field of view refused without the camera and out
 * of range, taken with it; the HUD hidden; the music ducked (a gain past 1 refused); joints on the
 * moby in slot 1 (none on a slot with no moby, nor too many); joint 3's place; a looping sound on
 * the moby, stopped (none on a slot with no moby). */
static void cutscene_frame(void) {
    openrac_camera cam;
    openrac_joint_offset joint;
    float at[3] = {0.0f, 0.0f, 0.0f};
    float frame[12] = {0.0f};
    int voice;
    openrac_air air;
    if (s_frames != 1) {
        return;
    }
    s_record(9800 + -s_api->camera_fov(s_mod, 40.0f));
    memset(&cam, 0, sizeof cam);
    cam.size = sizeof cam;
    cam.forward[0] = 1.0f;
    cam.left[1] = 1.0f;
    cam.up[2] = 1.0f;
    s_record(9800 + -s_api->camera_set(s_mod, &cam));
    s_record(9810 + -s_api->camera_fov(s_mod, 200.0f));
    s_record(9810 + -s_api->camera_fov(s_mod, 0.5f));
    s_record(9810 + -s_api->camera_fov(s_mod, 40.0f));
    s_record(9820 + -s_api->hud_hide(s_mod, 1));
    s_record(9820 + -s_api->menu_option_set(s_mod, "rumble_strength", OPENRAC_MENU_OPTION_ON));
    s_record(9820 + -s_api->menu_option_set(s_mod, "vsync", 7));
    s_record(9830 + -s_api->music_volume(s_mod, 2.0f));
    s_record(9830 + -s_api->music_volume(s_mod, 0.25f));
    memset(&joint, 0, sizeof joint);
    joint.joint = 3;
    joint.rotation[3] = 1.0f;
    s_record(9840 + -s_api->moby_set_joints(s_mod, 1, &joint, 1));
    s_record(9840 + -s_api->moby_set_joints(s_mod, 6, &joint, 1));
    s_record(9840 + -s_api->moby_set_joints(s_mod, 1, &joint, OPENRAC_JOINT_OFFSETS_MAX + 1));
    joint.joint = 4;
    s_record(9840 + -s_api->moby_set_joint_poses(s_mod, 1, &joint, 1));
    joint.joint = 0x80000004u;
    s_record(9840 + -s_api->moby_set_joint_poses(s_mod, 1, &joint, 1));
    joint.joint = 3;
    s_record(9840 + -s_api->moby_set_joints(s_mod, 1, &joint, 1));
    s_record(9850 + -s_api->moby_joint_position(1, 3, at));
    s_record((int)at[0]);
    s_record(9850 + -s_api->moby_joint_position(6, 3, at));
    s_record(9850 + -s_api->moby_joint_frame(1, 3, frame));
    s_record((int)frame[0]);
    voice = s_api->moby_sound_play(s_mod, 1, 12, 1);
    s_record(9860 + voice);
    s_record(9870 + s_api->moby_sound_play(s_mod, 6, 12, 0));
    s_api->moby_sound_stop(s_mod, voice);
    memset(&air, 0, sizeof air);
    air.size = sizeof air;
    air.flags = OPENRAC_AIR_FOG | OPENRAC_AIR_BACKGROUND;
    air.fog_near = 10.0f;
    air.fog_far = 5.0f; /* nearer than it starts: refused */
    s_record(9880 + -s_api->scene_air(s_mod, &air));
    air.fog_far = 50.0f;
    air.fog_thickness = 0.5f;
    air.background[0] = 1.0f;
    air.weight = 1.0f;
    s_record(9880 + -s_api->scene_air(s_mod, &air));
    s_record(9890 + -s_api->time_stretch(s_mod, 9.0f));
    s_record(9890 + -s_api->time_stretch(s_mod, 2.0f));
    s_record(9900 + -s_api->tie_hide(s_mod, -1, 1));
    s_record(9900 + -s_api->tie_hide(s_mod, 7, 1));
    s_record(9900 + -s_api->tie_hide(s_mod, 3, 1));
}

static void on_frame(void* user) {
    (void)user;
    s_frames++;
    if (s_cutscene && s_record != NULL) {
        cutscene_frame();
        return;
    }
    if (s_world && s_record != NULL) {
        world_frame();
        return;
    }
    if (s_play && s_record != NULL) {
        play_frame();
        return;
    }
    if (s_motion && s_record != NULL) {
        motion_frame();
        return;
    }
    if (s_record != NULL) {
        s_record(s_tag);
        /* A hook point taken after load is refused. */
        if (s_frames == 1) {
            void* original = NULL;
            s_record(1000 + -s_api->hook(s_mod, "Test_Add", "i32(i32)", (void*)add, &original));
            /* Interface 1.5: an entry after load is refused, and drawing outside the page is not
             * drawn. */
            if (s_menu) {
                s_record(1100 + -s_api->menu_entry(s_mod, "Late", menu_page, NULL));
                s_api->menu_text(s_mod, 0, 0, 1, 0, "OUTSIDE");
            }
            /* Interface 1.7: a page and a main-menu item after load are refused. */
            if (s_ui) {
                s_record(1400 + -s_api->ui_page(s_mod, 0, ui_page, NULL));
                s_record(1500 + -s_api->ui_main_item(s_mod, OPENRAC_UI_OPTIONS, "Late", 0));
                s_record(1600 + -s_api->ui_pause_item(s_mod, "Late", 0));
            }
            /* Interface 1.6: the tick boundary after load is refused. */
            if (s_savestate) {
                char bytes[16] = {0};
                s_record(1200 + -s_api->on_tick_boundary(s_mod, on_tick_boundary, NULL));
                /* And a snapshot outside the boundary. */
                s_record(1300 + -s_api->savestate_save(bytes, sizeof bytes));
            }
            if (s_developer_menu) {
                s_record(1600 + (s_api->take_developer_menu_action(s_mod) == 0));
                s_record(
                    1700
                    + (s_api->developer_menu_set_label(s_mod, 1, "Too late")
                       == OPENRAC_HOOK_TOO_LATE)
                );
                /* Interface 1.19: a switch is set while loading or at the tick boundary only. */
                s_record(
                    1800 + (s_api->developer_menu_switch(s_mod, 3, 0) == OPENRAC_HOOK_TOO_LATE)
                );
            }
        }
    }
}

/* One view a frame: the game's camera, 64 by 48. */
static int views_begin(void* user, openrac_view* views, int max) {
    (void)user;
    if (max < 1) {
        return 0;
    }
    views[0].kind = OPENRAC_VIEW_FULL;
    views[0].width = 64;
    views[0].height = 48;
    views[0].orientation[3] = 1.0f;
    return 1;
}

static void views_end(void* user, const openrac_view* views, int count) {
    (void)user;
    (void)views;
    s_ended += count;
    if (s_record != NULL) {
        s_record(2000 + s_ended);
    }
}

static void views_closing(void* user) {
    (void)user;
    if (s_record != NULL) {
        s_record(4000);
    }
}

/* Never called by the loader's test (no window); refuses, as a runtime without a headset would. */
static int32_t vk_create_instance(
    void* user, void* get_instance_proc_addr, const void* create_info, void** instance
) {
    (void)user;
    (void)get_instance_proc_addr;
    (void)create_info;
    (void)instance;
    return -3; /* VK_ERROR_INITIALIZATION_FAILED */
}

static void on_pad(void* user, int port, uint8_t* report, int size) {
    (void)user;
    if (port == 0 && size > 0) {
        report[0] = (uint8_t)(report[0] + s_tag);
    }
    if (s_input && size >= 3) {
        const openrac_input* in = s_api->input();
        report[1] = (uint8_t)((in->keys[26] & OPENRAC_KEY_HELD) ? 0x57 : 0); /* W */
        report[2] = (uint8_t)(in->mouse_dx + 0.5f);
        s_api->capture_mouse(s_mod, in->overlay ? 0 : 1);
    }
}

/* The host's rules heard (interface 1.14): its bytes as a number, 0 for none any more. */
static char s_big[OPENRAC_PARTY_RULES_MAX + 1];

/* The launcher changed the mod's settings (interface 1.15): its tag as it reads now. */
static void on_settings(void* user) {
    (void)user;
    s_record(12100 + (int)s_api->setting_int(s_mod, "tag"));
}

/* The game in progress changed (interface 1.16): its data's size, and its first byte. */
static void on_save_data(void* user) {
    unsigned char first = 0;
    const uint32_t size = s_api->save_data_get(s_mod, NULL, 0);
    (void)user;
    if (size > 0) {
        unsigned char bytes[16];
        s_record(
            13300
            + (int)(size <= sizeof bytes
                    && s_api->save_data_get(s_mod, bytes, sizeof bytes) == size)
        );
        first = bytes[0];
    }
    s_record(13100 + (int)size);
    s_record(13200 + first);
}

static void on_party_rules(void* user, const void* data, uint32_t size) {
    (void)user;
    s_record(data == NULL ? 7000 : 7000 + (int)size * 10 + ((const char*)data)[0] - 'a');
}

/* Over the game: a rectangle where the middle of the level's camera is, with its user data seen. */
static int overlay_user = 7;

static void on_overlay(void* user, const openrac_overlay* overlay) {
    const float world[3] = {1.0f, 2.0f, 3.0f};
    float x = 0.0f;
    float y = 0.0f;
    if (user == &overlay_user && overlay->size >= sizeof(openrac_overlay)
        && overlay->project(overlay->context, world, &x, &y)) {
        overlay->rect(
            overlay->context,
            x,
            y,
            overlay->text_width(overlay->context, 8.0f, "A"),
            8.0f,
            0xFF0000FFu
        );
    }
}

static void failed_camera(void* user, int id, int started) {
    (void)user;
    (void)id;
    (void)started;
    s_record(9901);
}

static int failed_metal(void* user, int slot, int key) {
    (void)user;
    (void)slot;
    (void)key;
    s_record(9902);
    return 0;
}

OPENRAC_MOD_EXPORT int openrac_mod_load(const openrac_api* api, openrac_mod* mod) {
    s_api = api;
    s_frames = 0;
    s_mod = mod;
    s_input = 0;
    s_views = 0;
    s_ended = 0;
    s_menu = 0;
    s_savestate = 0;
    s_developer_menu = 0;
    s_ui = 0;
    s_play = 0;
    s_world = 0;
    s_figure = -1;
    s_tag = (int)api->setting_int(mod, "tag");
    s_record = (void (*)(int))api->function("Test_Record");
    const char* behaviour = api->setting_string(mod, "behaviour");
    api->log(mod, OPENRAC_LOG_INFO, behaviour);
    if (strcmp(behaviour, "native_handles") == 0) {
        const openrac_graphics* graphics = api->graphics();
        if (graphics->backend != OPENRAC_GRAPHICS_VULKAN) {
            return 1;
        }
        const int has_handles = graphics->vk_device != NULL;
        if ((graphics->vk_queue != NULL) != has_handles
            || (graphics->vk_queue_index != 0) != has_handles) {
            return 2;
        }
        s_record(14000 + has_handles);
        return 0;
    }
    if (behaviour[0] == 'z') { /* watchers registered before an exclusive gate refuses */
        if (api->on_camera_script(mod, failed_camera, NULL) != OPENRAC_HOOK_OK
            || api->on_metal_detect(mod, failed_metal, NULL) != OPENRAC_HOOK_OK) {
            return 20;
        }
        if (api->on_moby_update(mod, world_gate, NULL) == OPENRAC_HOOK_TAKEN) {
            api->fail(mod, "the world gate is already taken");
            return 21;
        }
        return 22;
    }
    if (behaviour[0] == 'f') { /* fail */
        api->fail(mod, "this mod refuses on purpose");
        return 1;
    }
    if (behaviour[0] == 'u') { /* unknown_hook: carries on regardless; the loader does not */
        void* original = NULL;
        api->hook(mod, "Test_Nothing", "void()", (void*)add, &original);
        return 0;
    }
    if (behaviour[0] == 'b') { /* bad_signature */
        void* original = NULL;
        const int other = api->hook(mod, "Test_Add", "void(i32)", (void*)add, &original);
        /* Differing only in sign is the same signature. */
        const int sign = api->hook(mod, "Test_Add", "u32(u32)", (void*)add, &original);
        return other == OPENRAC_HOOK_SIGNATURE && sign == OPENRAC_HOOK_OK ? 2 : 0;
    }
    if (behaviour[0] == 'i') { /* input */
        s_input = 1;
        if (api->api_minor < 2 || api->take_input(mod, OPENRAC_INPUT_KEYBOARD) != 0) {
            return 4;
        }
        api->capture_mouse(mod, 1);
    }
    if (behaviour[0] == 'v') { /* views */
        const openrac_view_source source =
            {sizeof source, NULL, views_begin, views_end, views_closing};
        const openrac_vulkan_setup setup = {sizeof setup, NULL, vk_create_instance, NULL, NULL};
        s_views = 1;
        if (api->api_minor < 4 || api->graphics()->backend != OPENRAC_GRAPHICS_NONE) {
            return 5;
        }
        /* Taken by another mod: refuse to load, as a mod that needs them would. */
        if (api->on_views(mod, &source) != OPENRAC_HOOK_OK) {
            return 6;
        }
        if (api->setup_vulkan(mod, &setup) != OPENRAC_HOOK_OK) {
            return 7;
        }
    }
    if (behaviour[0] == 'm') { /* menu */
        s_menu = 1;
        if (api->api_minor < 5
            || api->menu_entry(mod, "Join the fun", menu_page, NULL) != OPENRAC_HOOK_OK) {
            return 8;
        }
        /* An entry without a label or a page is refused. */
        if (api->menu_entry(mod, "", menu_page, NULL) != OPENRAC_HOOK_UNKNOWN
            || api->menu_entry(mod, "No page", NULL, NULL) != OPENRAC_HOOK_UNKNOWN) {
            return 9;
        }
    }
    if (strcmp(behaviour, "keep")
        == 0) { /* keep: sets its own settings, hears the launcher's (1.15) */
        if (api->api_minor < 15 || s_record == NULL
            || api->on_settings(mod, on_settings, NULL) != OPENRAC_HOOK_OK
            || api->on_settings(mod, NULL, NULL) != OPENRAC_HOOK_TOO_LATE) {
            return 17;
        }
        s_record(12000 + -api->setting_set_int(mod, "tag", 42));
        s_record(12010 + -api->setting_set_int(mod, "missing", 1));
        s_record(12020 + -api->setting_set_bool(mod, "tag", 1));
        s_record(12030 + -api->setting_set_string(mod, "behaviour", "nope"));
        s_record(12040 + -api->setting_set_string(mod, "jump", "KeyW,ArrowUp"));
        s_record(12050 + -api->setting_set_string(mod, "jump", "NoSuchKey"));
        s_record(12060 + -api->setting_set_float(mod, "tag", 1.5));
        s_record(12300 + (int)api->setting_int(mod, "tag"));
        s_record(12200 + (strcmp(api->setting_string(mod, "jump"), "KeyW,ArrowUp") == 0));
    }
    if (behaviour[0] == 'j') { /* journal: its data kept with the saves (1.16) */
        static unsigned char big[OPENRAC_SAVE_DATA_MAX + 1u];
        if (api->api_minor < 16 || s_record == NULL
            || api->on_save_data(mod, on_save_data, NULL) != OPENRAC_HOOK_OK
            || api->on_save_data(mod, NULL, NULL) != OPENRAC_HOOK_TOO_LATE) {
            return 18;
        }
        s_record(13000 + -api->save_data_set(mod, "abc", 3));
        s_record(13010 + -api->save_data_set(mod, big, sizeof big));
        s_record(13020 + (int)api->save_data_get(mod, NULL, 0));
    }
    if (behaviour[0] == 'r') { /* rules */
        if (api->api_minor < 14 || s_record == NULL
            || api->on_party_rules(mod, on_party_rules, NULL) != 0
            || api->party_rules_publish(mod, "abc", 3, "Bolts x2") != OPENRAC_HOOK_OK
            || api->party_rules_publish(mod, s_big, sizeof s_big, "") == OPENRAC_HOOK_OK) {
            return 16;
        }
        s_api = api;
    }
    if (behaviour[0]
        == 't') { /* twelve: with no game behind them, the calls of interface 1.14 answer unknown; with no stack, no packs */
        float where[3] = {0, 0, 0};
        int blocked = 0;
        openrac_stand_in stand_in = {{0, 0, 0}, 0, 0, 0, 0};
        uint8_t missions[OPENRAC_MISSION_STATE_SIZE] = {0};
        char text[16] = {0};
        uint32_t digest = 0;
        openrac_save_info info = {sizeof info, 0, 0, 0, 0};
        openrac_shot2 shot = {sizeof shot};
        openrac_moby2 moby = {sizeof moby};
        openrac_packs_info packs = {sizeof packs, 0, 0};
        openrac_pack_entry pack = {sizeof pack, NULL, NULL, 0, NULL};
        if (api->api_minor < 14
            || api->card_filter(mod, OPENRAC_CARD_PASS, "") != OPENRAC_HOOK_UNKNOWN
            || api->card_slot_hint(mod, 0) != OPENRAC_HOOK_UNKNOWN
            || api->coop_save_write(mod, 0, text, 1) != OPENRAC_HOOK_UNKNOWN
            || api->save_slot2("", 0, &info) != OPENRAC_HOOK_UNKNOWN
            || api->save_read2("", 0, text, sizeof text) != 0 || api->card_saves() != 0
            || api->card_note_write(mod, 0, "x") != OPENRAC_HOOK_UNKNOWN
            || api->card_note_read("coop", 0, text, sizeof text) != 0
            || api->shared_read(1, text, sizeof text) != OPENRAC_HOOK_UNKNOWN
            || api->shared_give(1, text, 1) != OPENRAC_HOOK_UNKNOWN
            || api->mission_state_read(0, missions) != OPENRAC_HOOK_UNKNOWN
            || api->mission_state_write(0, missions) != OPENRAC_HOOK_UNKNOWN
            || api->mission_state_clear(0, missions) != OPENRAC_HOOK_UNKNOWN
            || api->hero_place(mod, where, 0.0f, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_grab(mod, 1, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_release(mod) != OPENRAC_HOOK_UNKNOWN
            || api->hero_hold_at(mod, where, 0.0f) != OPENRAC_HOOK_UNKNOWN
            || api->hero_stand_in_as(mod, &stand_in, -1) != OPENRAC_HOOK_UNKNOWN
            || api->hero_stand_in_read(mod, &stand_in) != OPENRAC_HOOK_UNKNOWN
            || api->hero_revive(mod, NULL, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_shots2(&shot, 1) != 0
            || api->figure_shot2(mod, 0, &shot) != OPENRAC_HOOK_UNKNOWN
            || api->moby_list2(&moby, 1) != 0
            || (api->on_camera_script(mod, NULL, NULL) != OPENRAC_HOOK_UNKNOWN
                && api->on_camera_script(mod, NULL, NULL) != OPENRAC_HOOK_TOO_LATE)
            || api->camera_script_play(mod, 0) != OPENRAC_HOOK_UNKNOWN
            || api->camera_script_view(where, where, where, NULL) != OPENRAC_HOOK_UNKNOWN
            || api->camera_script_show(mod, where, where) != OPENRAC_HOOK_UNKNOWN
            || api->level_line(where, where, &blocked) != OPENRAC_HOOK_UNKNOWN
            || api->play_scene_from(mod, 0, 0) != OPENRAC_HOOK_UNKNOWN
            || api->play_live_picture(mod, 1) != OPENRAC_HOOK_UNKNOWN
            || api->on_metal_detect(mod, NULL, NULL) != OPENRAC_HOOK_TOO_LATE
            || api->packs_info(&packs) != OPENRAC_HOOK_OK || packs.count != 0
            || api->packs_entry(0, &pack) != OPENRAC_HOOK_UNKNOWN
            || api->clipboard_set("x") != OPENRAC_HOOK_UNKNOWN
            || api->open_link("openrac://x") != OPENRAC_HOOK_UNKNOWN
            || api->world_family(text, sizeof text) != 0
            || api->sim_digest(0, &digest) != OPENRAC_HOOK_UNKNOWN
            || api->identity_key(text, sizeof text) != 0) {
            return 17;
        }
    }
    if (strcmp(behaviour, "hud-menu") == 0) { /* draws over the level and its menus (1.31) */
        if (api->on_overlay_phases(mod, on_overlay, &overlay_user, 0) != OPENRAC_HOOK_UNKNOWN
            || api->on_overlay_phases(mod, on_overlay, &overlay_user, 0x80u) != OPENRAC_HOOK_UNKNOWN
            || api->on_overlay_phases(mod, NULL, NULL, OPENRAC_OVERLAY_MENU) != OPENRAC_HOOK_UNKNOWN
            /* the layer bit is no phase of its own (1.40) */
            || api->on_overlay_phases(mod, on_overlay, &overlay_user, OPENRAC_OVERLAY_UNDER_HUD)
                   != OPENRAC_HOOK_UNKNOWN
            || api->on_overlay_phases(
                   mod,
                   on_overlay,
                   &overlay_user,
                   OPENRAC_OVERLAY_PLAYING | OPENRAC_OVERLAY_MENU | OPENRAC_OVERLAY_UNDER_HUD
               ) != OPENRAC_HOOK_OK) {
            return 18;
        }
    } else if (behaviour[0] == 'h') { /* hud: draws over the game */
        if (api->on_overlay(mod, NULL, NULL) != OPENRAC_HOOK_TOO_LATE
            || api->on_overlay(mod, on_overlay, &overlay_user) != OPENRAC_HOOK_OK
            || api->play_live_picture(mod, 0) != OPENRAC_HOOK_OK) {
            return 18;
        }
    }
    if (behaviour[0] == 's') { /* savestate */
        s_savestate = 1;
        if (api->api_minor < 6 || s_record == NULL
            || api->on_tick_boundary(mod, NULL, NULL) == OPENRAC_HOOK_OK
            || api->on_tick_boundary(mod, on_tick_boundary, NULL) != OPENRAC_HOOK_OK) {
            return 10;
        }
    }
    if (behaviour[0] == 'd') { /* developer_menu */
        openrac_developer_menu_item items[] = {
            {"Empty", "Shift+Num0", 1, "Save"},
            {"Empty", "Ctrl+Num0", 2, "Load"},
            {"Invincible", NULL, 3, NULL},
            {"Speed", NULL, 4, NULL},
        };
        const openrac_developer_menu_item duplicate_action[] = {{"Other", NULL, 1, "Other"}};
        const char* speeds[] = {"x1", "x2", "x4"};
        const char* null_speed[] = {"x1", NULL};
        s_developer_menu = 1;
        if (api->api_minor < 10
            || api->developer_menu(mod, "Savestates", items, 4) != OPENRAC_HOOK_OK
            || api->developer_menu(mod, "Savestates", items, 4) != OPENRAC_HOOK_TAKEN
            || api->developer_menu(mod, "Developer", items, 4) != OPENRAC_HOOK_TAKEN
            || api->developer_menu(mod, "Other", duplicate_action, 1) != OPENRAC_HOOK_TAKEN
            || api->on_tick_boundary(mod, on_tick_boundary, NULL) != OPENRAC_HOOK_OK) {
            return 16;
        }
        /* Interface 1.19: item 3 a switch, item 4 a choice; nothing else is either. */
        if (api->api_minor < 19 || api->developer_menu_value(mod, 3) != -1
            || api->developer_menu_switch(mod, 3, 7) != OPENRAC_HOOK_OK
            || api->developer_menu_value(mod, 3) != 1
            || api->developer_menu_switch(mod, 99, 1) != OPENRAC_HOOK_UNKNOWN
            || api->developer_menu_choice(mod, 4, speeds, 3, 3) != OPENRAC_HOOK_UNKNOWN
            || api->developer_menu_choice(mod, 4, speeds, 0, 0) != OPENRAC_HOOK_UNKNOWN
            || api->developer_menu_choice(mod, 4, null_speed, 2, 0) != OPENRAC_HOOK_UNKNOWN
            || api->developer_menu_choice(mod, 4, speeds, 3, 1) != OPENRAC_HOOK_OK
            || api->developer_menu_value(mod, 4) != 1 || api->developer_menu_value(mod, 1) != -1
            || api->developer_menu_value(mod, 99) != -1) {
            return 23;
        }
        /* Interface 1.28: item 2 a locked row to read with a value, a progress bar and an icon;
         * item 1 set and cleared again; nothing for an unknown action, a short look or a NaN. */
        {
            openrac_developer_menu_look look = {0};
            openrac_developer_menu_look short_look = {0};
            look.size = sizeof look;
            look.flags = OPENRAC_DEVELOPER_MENU_INFO | OPENRAC_DEVELOPER_MENU_LOCKED;
            look.value = "24/40";
            look.progress = 1.5f;
            look.icon = "trophy";
            short_look.size = 8;
            if (api->api_minor < 28 || api->developer_menu_look(mod, 2, &look) != OPENRAC_HOOK_OK
                || api->developer_menu_look(mod, 1, &look) != OPENRAC_HOOK_OK
                || api->developer_menu_look(mod, 1, NULL) != OPENRAC_HOOK_OK
                || api->developer_menu_look(mod, 99, &look) != OPENRAC_HOOK_UNKNOWN
                || api->developer_menu_look(mod, 1, &short_look) != OPENRAC_HOOK_UNKNOWN) {
                return 24;
            }
            look.progress = NAN;
            if (api->developer_menu_look(mod, 1, &look) != OPENRAC_HOOK_UNKNOWN) {
                return 24;
            }
        }
        /* The host copies its strings, so the caller's stack storage can go away. */
        items[0].label = "Changed after registration";
    }
    if (strcmp(behaviour, "kerwan")
        == 0) { /* kerwan: a pack's files (pack_folder, "pack:" paths) and the language */
        char folder[512];
        char code[3];
        if (api->api_minor < 14 || s_record == NULL) {
            return 18;
        }
        s_record(9700 + (api->pack_folder(folder, sizeof folder) > 0));
        s_record(9700 + api->pack_folder(folder, 2));
        s_record(9710 + api->sound_load(mod, "pack:level_03/cutscenes/line.wav"));
        s_record(9720 + -api->sound_load(mod, "pack:../escape.wav"));
        s_record(9720 + -api->sound_load(mod, "pack:level_03/missing.wav"));
        s_record(9720 + -api->sound_load(mod, "pack:"));
        s_record(9740 + api->game_language(code, sizeof code));
        s_record(code[0] * 256 + code[1]);
        s_record(9750 + api->game_language(code, 2));
    }
    if (strcmp(behaviour, "cutscene") == 0) {
        s_cutscene = 1;
        if (api->api_minor < 14 || api->size < sizeof(openrac_api) || s_record == NULL) {
            return 19;
        }
    }
    if (behaviour[0] == 'h') { /* hoverboard */
        s_motion = 1;
        if (api->api_minor < 12 || s_record == NULL) {
            return 17;
        }
    }
    if (behaviour[0] == 'p') { /* play */
        s_play = 1;
        if (api->api_minor < 8 || s_record == NULL) {
            return 13;
        }
    }
    if (behaviour[0] == 'w') { /* world */
        const float where[3] = {0};
        s_world = 1;
        if (api->api_minor < 9 || s_record == NULL) {
            return 14;
        }
        /* The gate and the watcher: one mod's each, taken during load; a second take (by this mod
         * too) and a stand-in outside the gate are refused. */
        if (api->on_moby_update(mod, world_gate, NULL) != OPENRAC_HOOK_OK
            || api->on_moby_update(mod, world_gate, NULL) != OPENRAC_HOOK_TAKEN
            || api->on_moby_update(mod, NULL, NULL) == OPENRAC_HOOK_OK
            || api->on_moby_hit(mod, world_hit, NULL) != OPENRAC_HOOK_OK
            || api->on_moby_hit(mod, world_hit, NULL) != OPENRAC_HOOK_TAKEN
            || api->hero_stand_in(mod, where, -1) != OPENRAC_HOOK_UNKNOWN) {
            return 15;
        }
    }
    if (strcmp(behaviour, "callback_only") == 0) {
        if (api->on_tick_boundary(mod, on_tick_boundary, NULL) != OPENRAC_HOOK_OK
            || api->on_pad(mod, on_pad, NULL) != OPENRAC_HOOK_OK) {
            return 18;
        }
        return 0;
    }
    if (behaviour[0] == 'g') { /* game_ui */
        static const char* const letters[] = {"A", "B", "C"};
        const openrac_ui_item items[] = {
            {OPENRAC_UI_BUTTON, 0, "Find match", NULL, 0, 0},
            {OPENRAC_UI_TEXT, 0, "Player: Ratchet", NULL, 0, 0},
            {OPENRAC_UI_CHOICE, 0, "Code", letters, 3, 2},
            {OPENRAC_UI_CHOICE, 0, "Nothing to choose", NULL, 0, 0},
        };
        const char* const footer[] = {"Connected"};
        const openrac_ui_content content = {sizeof content, "Multiplayer", items, 4, footer, 1};
        int home;
        s_ui = 1;
        if (api->api_minor < 7 || s_record == NULL) {
            return 11;
        }
        home = api->ui_page(mod, 0, ui_page, NULL);
        s_ui_second = api->ui_page(mod, OPENRAC_UI_OWN_BACK, ui_page, NULL);
        /* A page without a callback; an item after an unknown one, without a label, or of no page
         * of the mod's; a page shown while none is on the screen: refused. */
        if (home != 0 || s_ui_second != 1
            || api->ui_page(mod, 0, NULL, NULL) != OPENRAC_HOOK_UNKNOWN
            || api->ui_main_item(mod, "quit", "X", home) != OPENRAC_HOOK_UNKNOWN
            || api->ui_main_item(mod, OPENRAC_UI_LOAD_GAME, "", home) != OPENRAC_HOOK_UNKNOWN
            || api->ui_main_item(mod, OPENRAC_UI_LOAD_GAME, "X", 9) != OPENRAC_HOOK_UNKNOWN
            || api->ui_open(mod, s_ui_second) != OPENRAC_HOOK_UNKNOWN
            || api->ui_main_item(mod, OPENRAC_UI_LOAD_GAME, "Multiplayer", home) != OPENRAC_HOOK_OK
            || api->ui_set(mod, home, &content) != OPENRAC_HOOK_OK
            || api->ui_set(mod, 5, &content) != OPENRAC_HOOK_UNKNOWN) {
            return 12;
        }
        /* Interface 1.9: the pause menu's Options items, without a label or of no page of the
         * mod's refused, two at most. */
        if (api->api_minor >= 9
            && (api->ui_pause_item(mod, "", home) != OPENRAC_HOOK_UNKNOWN
                || api->ui_pause_item(mod, NULL, home) != OPENRAC_HOOK_UNKNOWN
                || api->ui_pause_item(mod, "X", 9) != OPENRAC_HOOK_UNKNOWN
                || api->ui_pause_item(mod, "Multiplayer", home) != OPENRAC_HOOK_OK
                || api->ui_pause_item(mod, "Party", s_ui_second) != OPENRAC_HOOK_OK
                || api->ui_pause_item(mod, "Third", home) != OPENRAC_HOOK_UNKNOWN)) {
            return 13;
        }
    }
    if (api->hook(mod, "Test_Add", "i32(i32)", (void*)add, (void**)&s_add) != OPENRAC_HOOK_OK) {
        return 3;
    }
    api->on_frame(mod, on_frame, NULL);
    api->on_pad(mod, on_pad, NULL);
    return 0;
}
