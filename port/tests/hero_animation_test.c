/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/level_hero_animation.c"
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define NEAR(a, b) CHECK(fabsf((a) - (b)) < 0.00001f)
#if __SIZEOF_POINTER__ == 4
_Static_assert(offsetof(HeroAnimObject, fraction) == 0x54, "EE blend field");
_Static_assert(offsetof(HeroAnimObject, key_data) == 0x68, "EE key pointer");
_Static_assert(offsetof(HeroAnimObject, voice) == 0x7D, "EE voice field");
_Static_assert(offsetof(HeroAnimClass, sequences) == 0x48, "EE sequence table");
#endif

unsigned char hero_anim_state[0x4000];
float hero_anim_curves[50], hero_anim_restart_rate;
int hero_anim_mode;
static HeroAnimClass model;
static HeroAnimObject object, other;
static unsigned char sequences[2][128];
static float rates[2][4], new_position;
static int ticks, restarts, rebinds, update_a, update_b, stopped, played, sound_ids[4],
    sound_flags[4];
static unsigned char* g;

void hero_anim_tick(void) {
    ++ticks;
}

void hero_anim_restart(void) {
    ++restarts;
}

void hero_anim_rebind(void) {
    ++rebinds;
}

void hero_anim_update_a(void) {
    ++update_a;
}

void hero_anim_update_b(void) {
    ++update_b;
}

int hero_anim_truncate(float x) {
    return (int)x;
}

int hero_anim_play(int id, int flags, void* obj) {
    CHECK(obj == &object && played < 4);
    sound_ids[played] = id;
    sound_flags[played++] = flags;
    return 3;
}

void hero_anim_stop(int id) {
    CHECK(id == 2);
    ++stopped;
}

float hero_anim_position(void* obj) {
    CHECK(obj == &object);
    return new_position;
}

static void reset(void) {
    int i, j;
    memset(hero_anim_state, 0, sizeof(hero_anim_state));
    memset(&object, 0, sizeof(object));
    memset(&model, 0, sizeof(model));
    memset(sequences, 0, sizeof(sequences));
    memset(hero_anim_curves, 0, sizeof(hero_anim_curves));
    ticks = restarts = rebinds = update_a = update_b = stopped = played = 0;
    hero_anim_mode = 0;
    hero_anim_restart_rate = 0.125f;
    g = hero_anim_state + 0xE1D;
    *(HeroAnimObject**)(g + 0x2080) = &object;
    *(float*)(g + 0xA90) = 1.0f;
    *(float*)(g + 0xA94) = 0.25f;
    *(int*)(g + 0xA98) = 15;
    *(int*)(g + 0xAA0) = *(int*)(g + 0xAB0) = -1;
    *(float*)(g + 0xAA8) = 6;
    new_position = 7;
    object.model = &model;
    object.next_key = 1;
    object.fraction = 0.25f;
    object.sound = object.voice = 255;
    for (i = 0; i < 2; ++i) {
        model.sequences[i] = sequences[i];
        sequences[i][0x10] = 4;
        for (j = 0; j < 4; ++j) {
            rates[i][j] = 1;
            ((float**)(sequences[i] + 0x1C))[j] = &rates[i][j];
        }
    }
    object.key_data = &rates[0][0];
    object.next_key_data = &rates[0][1];
}

static void run(void) {
    func_L00_00232EF0();
    CHECK(ticks == 1 && update_a == 1 && update_b == 1);
}

static void stepping(void) {
    reset();
    run();
    NEAR(object.fraction, 0.5f);
    CHECK(*(int*)(g + 0xA98) == 12 && *(int*)(g + 0xA9C) == 0);
    NEAR(*(float*)(g + 0xAAC), 1);
    reset();
    object.next_sequence = 1;
    *(int*)(g + 0xAA0) = 1;
    *(int*)(g + 0xAA4) = 2;
    hero_anim_curves[27] = 0.6f;
    new_position = 2;
    run();
    NEAR(object.fraction, 0.6f);
    NEAR(*(float*)(g + 0xAAC), 0.35f);
    CHECK(*(int*)(g + 0xAA4) == 3 && *(int*)(g + 0xA9C) == 1);
    reset();
    object.next_sequence = 1;
    *(float*)(g + 0xA90) = 100;
    *(float*)(g + 0xA94) = 0.4f;
    run();
    NEAR(object.fraction, 0.65f);
    reset();
    *(float*)(g + 0xA94) = 0;
    object.fraction = 0.009f;
    run();
    CHECK(object.fraction == 0);
    reset();
    *(float*)(g + 0xA94) = 0;
    object.fraction = 0.01f;
    run();
    CHECK(object.fraction == 0.01f); /* strict snap boundary */
    reset();
    *(float*)(g + 0xA94) = 0.746f;
    run();
    CHECK(object.fraction == 0 && object.key == 1 && object.next_key == 2);
    reset();
    *(float*)(g + 0xA94) = 1;
    object.fraction = 0.5f;
    rates[0][1] = 0.5f;
    run();
    NEAR(object.fraction, 0.25f);
    CHECK(object.key_data == &rates[0][1] && object.next_key_data == &rates[0][2]);
    NEAR(*(float*)(g + 0xA94), 0.5f);
    reset();
    *(float*)(g + 0xA94) = 1;
    *(float*)(g + 0xA90) = 3;
    new_position = 2;
    run();
    NEAR(object.fraction, 0.25f);
    CHECK(object.key == 3 && object.next_key == 0 && *(int*)(g + 0xA98) == 15);
    NEAR(*(float*)(g + 0xAAC), 3);
}

static void transitions(void) {
    reset();
    object.next_sequence = 1;
    *(float*)(g + 0xA94) = 1;
    object.next_key_data = &rates[1][1];
    sequences[1][0x12] = 2;
    run();
    CHECK(object.sequence == 1 && object.event_count == 2 && played == 0);
    CHECK(object.next_key_data == &rates[1][2] && *(int*)(g + 0xA9C) == 0);
    reset();
    *(float*)(g + 0xA94) = 1;
    *(int*)(g + 0xAB8) = 1;
    *(int*)(g + 0xAB4) = 3;
    run();
    CHECK(restarts == 1 && rebinds == 1 && object.next_key == 3 && object.fraction == 0);
    CHECK(*(int*)(g + 0xAB8) == 0 && object.next_key_data == &rates[0][3]);
    CHECK(*(float*)(g + 0xA94) == hero_anim_restart_rate);
    reset();
    *(float*)(g + 0xA94) = 1;
    *(int*)(g + 0xAB0) = 0;
    *(int*)(g + 0xAB4) = 1;
    run();
    CHECK(restarts == 0 && rebinds == 1 && object.next_key == 0 && object.fraction == 0);
    NEAR(*(float*)(g + 0xA94), 1.0f / 3.0f);
}

static void sounds(void) {
    int* events;
    unsigned char* voice;
    int mode;
    reset();
    object.event_count = 3;
    sequences[0][0x12] = 3;
    events = (int*)((float**)(sequences[0] + 0x1C) + 4);
    events[0] = (4 << 16) | 40; /* old endpoint is excluded */
    events[1] = (8 << 16) | 80; /* new endpoint is included */
    events[2] = (7 << 16) | 70; /* only the first qualifying event is emitted */
    run();
    CHECK(played == 1 && sound_ids[0] == 80 && sound_flags[0] == 0);
    reset();
    object.voice = 2;
    object.sound = 9;
    voice = hero_anim_state + 0x1D + 2 * 0x70;
    *(HeroAnimObject**)(voice + 0x88) = &other;
    run();
    CHECK(object.voice == 255 && stopped == 0 && played == 0);
    reset();
    object.voice = 2;
    object.sound = 9;
    voice = hero_anim_state + 0x1D + 2 * 0x70;
    *(HeroAnimObject**)(voice + 0x88) = &object;
    *(short*)(voice + 0x7E) = 8;
    run();
    CHECK(object.voice == 255 && stopped == 1 && played == 0);
    reset();
    object.voice = 2;
    object.sound = 9;
    voice = hero_anim_state + 0x1D + 2 * 0x70;
    *(HeroAnimObject**)(voice + 0x88) = &object;
    *(short*)(voice + 0x7E) = 9;
    run();
    CHECK(object.voice == 2 && stopped == 0 && played == 0);
    for (mode = 0; mode <= 6; ++mode) {
        reset();
        object.sound = 9;
        hero_anim_mode = mode;
        run();
        if (mode == 2 || mode == 6) {
            CHECK(played == 0 && object.voice == 255);
        } else {
            CHECK(played == 1 && sound_ids[0] == 9 && sound_flags[0] == 4 && object.voice == 3);
        }
    }
}

int main(void) {
    stepping();
    transitions();
    sounds();
    puts("hero animation tests passed");
    return 0;
}
