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
 * gate's and the watcher's 8700s and 8800s) (interface 1.9). It reports the interface it is
 * built for
 * (OPENRAC_MOD_TARGET_MINOR): test-old is built for 1.5. */
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

static void on_frame(void* user) {
    (void)user;
    s_frames++;
    if (s_world && s_record != NULL) {
        world_frame();
        return;
    }
    if (s_play && s_record != NULL) {
        play_frame();
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

/* The host's rules heard (interface 1.11): its bytes as a number, 0 for none any more. */
static char s_big[OPENRAC_PARTY_RULES_MAX + 1];

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
        overlay->rect(overlay->context, x, y, overlay->text_width(overlay->context, 8.0f, "A"), 8.0f, 0xFF0000FFu);
    }
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
        return api->hook(mod, "Test_Add", "void(i32)", (void*)add, &original) == OPENRAC_HOOK_SIGNATURE ? 2 : 0;
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
    if (behaviour[0] == 'r') { /* rules */
        if (api->api_minor < 11 || s_record == NULL
            || api->on_party_rules(mod, on_party_rules, NULL) != 0
            || api->party_rules_publish(mod, "abc", 3, "Bolts x2") != OPENRAC_HOOK_OK
            || api->party_rules_publish(mod, s_big, sizeof s_big, "") == OPENRAC_HOOK_OK) {
            return 16;
        }
        s_api = api;
    }
    if (behaviour[0] == 't') { /* twelve: every call of interface 1.12 not filled in yet answers unknown; with no stack, no packs */
        float where[3] = {0, 0, 0};
        uint8_t missions[OPENRAC_MISSION_STATE_SIZE] = {0};
        char text[16] = {0};
        uint32_t digest = 0;
        openrac_save_info info = {sizeof info, 0, 0, 0, 0};
        openrac_shot2 shot = {sizeof shot};
        openrac_moby2 moby = {sizeof moby};
        openrac_packs_info packs = {sizeof packs, 0, 0};
        openrac_pack_entry pack = {sizeof pack, NULL, NULL, 0, NULL};
        if (api->api_minor < 12 || api->card_filter(mod, OPENRAC_CARD_PASS, "") != OPENRAC_HOOK_UNKNOWN
            || api->card_slot_hint(mod, 0) != OPENRAC_HOOK_UNKNOWN
            || api->coop_save_write(mod, 0, text, 1) != OPENRAC_HOOK_UNKNOWN
            || api->save_slot2("", 0, &info) != OPENRAC_HOOK_UNKNOWN
            || api->save_read2("", 0, text, sizeof text) != 0
            || api->card_saves() != 0
            || api->card_note_write(mod, 0, "x") != OPENRAC_HOOK_UNKNOWN
            || api->card_note_read("coop", 0, text, sizeof text) != 0
            || api->shared_read(1, text, sizeof text) != OPENRAC_HOOK_UNKNOWN
            || api->shared_give(1, text, 1) != OPENRAC_HOOK_UNKNOWN
            || api->mission_state_read(0, missions) != OPENRAC_HOOK_UNKNOWN
            || api->mission_state_write(0, missions) != OPENRAC_HOOK_UNKNOWN
            || api->hero_place(mod, where, 0.0f, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_grab(mod, 1, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_release(mod) != OPENRAC_HOOK_UNKNOWN
            || api->hero_revive(mod, NULL, 0) != OPENRAC_HOOK_UNKNOWN
            || api->hero_shots2(&shot, 1) != 0
            || api->figure_shot2(mod, 0, &shot) != OPENRAC_HOOK_UNKNOWN
            || api->moby_list2(&moby, 1) != 0
            || api->on_camera_script(mod, NULL, NULL) != OPENRAC_HOOK_UNKNOWN
            || api->camera_script_play(mod, 0) != OPENRAC_HOOK_UNKNOWN
            || api->play_scene_from(mod, 0, 0) != OPENRAC_HOOK_UNKNOWN
            || api->play_live_picture(mod, 1) != OPENRAC_HOOK_UNKNOWN
            || api->on_metal_detect(mod, NULL, NULL) != OPENRAC_HOOK_UNKNOWN
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
    if (behaviour[0] == 'h') { /* hud: draws over the game */
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
        };
        const openrac_developer_menu_item duplicate_action[] = {{"Other", NULL, 1, "Other"}};
        s_developer_menu = 1;
        if (api->api_minor < 10
            || api->developer_menu(mod, "Savestates", items, 2) != OPENRAC_HOOK_OK
            || api->developer_menu(mod, "Savestates", items, 2) != OPENRAC_HOOK_TAKEN
            || api->developer_menu(mod, "Developer", items, 2) != OPENRAC_HOOK_TAKEN
            || api->developer_menu(mod, "Other", duplicate_action, 1) != OPENRAC_HOOK_TAKEN
            || api->on_tick_boundary(mod, on_tick_boundary, NULL) != OPENRAC_HOOK_OK) {
            return 16;
        }
        /* The host copies its strings, so the caller's stack storage can go away. */
        items[0].label = "Changed after registration";
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
