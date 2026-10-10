/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game/rac1-pal/hand/level_hero_effects.c"
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define NEAR(a, b) CHECK(fabsf((a) - (b)) < 0.00001f)

unsigned char hero_fx_state[0x4000], hero_fx_mode[4], hero_fx_settings[64];
int hero_fx_frame, hero_fx_joints[4];
unsigned int hero_fx_template_a[16], hero_fx_template_b[16], hero_fx_template_c[16];
float hero_fx_envelope[22];
static unsigned char object[0x100], other[0x100], *g;
static int approached, countdowns, attached, detached, scaled, randomized, zero_period;
static int seen_joints[4], seen_detach[4], random_low, random_high;
static float tick_scale, sine_result, sine_phase;
static jmp_buf trap;

float hero_fx_approach(float* p, float target, float step) {
    CHECK(p == (float*)(hero_fx_settings + 0x28));
    NEAR(target, hero_fx_mode[3] ? 1.8f : 1.0f);
    NEAR(step, 0.05f);
    ++approached;
    return *p;
}

int hero_fx_ticks(int n) {
    return zero_period && n == 110 ? 0 : (int)(0.5f + n * tick_scale);
}

float hero_fx_float(int n) {
    return (float)n;
}

int hero_fx_truncate(float n) {
    return (int)n;
}

float hero_fx_sin(float n) {
    sine_phase = n;
    return sine_result;
}

void hero_fx_trap(void) {
    longjmp(trap, 1);
}

int hero_fx_countdown(short* p) {
    CHECK(p == (short*)(g + 0xFFC));
    ++countdowns;
    if (*p == 0) {
        return 1;
    }
    if (*p < 1) {
        *p = 1;
    }
    --*p;
    return *p > 0 ? 0 : 2;
}

int hero_fx_random(int low, int high) {
    ++randomized;
    random_low = low;
    random_high = high;
    return 123;
}

void hero_fx_attach(void* moby, int joint, void* p) {
    unsigned char* node = p;
    CHECK(moby == object && attached < 4);
    CHECK(node == g + 0xEF0 + attached * 0x40);
    seen_joints[attached++] = joint;
    node[1] = 1;
}

void hero_fx_detach(void* moby, void* p) {
    unsigned char* node = p;
    CHECK(moby == object && detached < 4 && node[1]);
    seen_detach[detached++] = (int)(node - g - 0xEF0) / 0x40;
    memset(node, 0, 0x40);
}

void hero_fx_scale(void* out, void* in, float scale) {
    int i;
    CHECK(out == in);
    ++scaled;
    for (i = 0; i < 4; ++i) {
        ((float*)out)[i] = ((float*)in)[i] * scale;
    }
}

static void reset(void) {
    int i;
    float f;
    memset(hero_fx_state, 0, sizeof(hero_fx_state));
    memset(hero_fx_mode, 0, sizeof(hero_fx_mode));
    memset(object, 0, sizeof(object));
    memset(other, 0, sizeof(other));
    approached = countdowns = attached = detached = scaled = randomized = zero_period = 0;
    tick_scale = 1;
    sine_result = sine_phase = 0;
    hero_fx_frame = 0;
    g = hero_fx_state + 0xE1D;
    *(unsigned char**)(g + 0x1184) = object;
    *(unsigned char**)(g + 0x2080) = other;
    *(float*)(object + 0x2C) = 2;
    *(unsigned short*)(object + 0x34) = 0x204;
    *(short*)(g + 0xFFC) = 100;
    for (i = 0; i < 4; ++i) {
        hero_fx_joints[i] = 7 + i * 2;
    }
    for (i = 0; i < 16; ++i) {
        hero_fx_template_a[i] = 0x12340000u + (unsigned)i;
        hero_fx_template_b[i] = 0x56780000u + (unsigned)i;
        f = (float)i + 0.5f;
        memcpy(hero_fx_template_c + i, &f, sizeof(f));
    }
    for (i = 0; i < 22; ++i) {
        hero_fx_envelope[i] = (float)i * 0.125f;
    }
}

static unsigned int color(unsigned char* p) {
    return *(unsigned int*)(p + 0x90);
}

static void selection_and_color(void) {
    int mode;
    reset();
    func_L00_00205FF0();
    CHECK(approached == 1 && countdowns == 1 && attached == 0);
    CHECK(color(object) == 0x804CA044u && *(unsigned short*)(object + 0x34) == 0x214);
    NEAR(sine_phase, -3.141592741f);
    reset();
    hero_fx_frame = 165;
    sine_result = 1;
    *(int*)(g + 0x22A8) = 1;
    func_L00_00205FF0();
    CHECK(color(object) == 0x8064D0ACu);
    NEAR(sine_phase, 0);
    reset();
    sine_result = -1;
    tick_scale = 0.5f;
    hero_fx_frame = 11;
    func_L00_00205FF0();
    CHECK(color(object) == 0x8034702Cu);
    NEAR(sine_phase, -0.6f * 3.141592741f);
    for (mode = 1; mode <= 3; ++mode) {
        reset();
        g[0x20A4] = (unsigned char)mode;
        hero_fx_mode[3] = 1;
        func_L00_00205FF0();
        CHECK(approached == 1 && countdowns == 0);
        CHECK(color(mode == 3 ? object : other) == 0x804CA044u);
        CHECK(color(mode == 3 ? other : object) == 0);
    }
    reset();
    *(short*)(g + 0x22D8) = 1;
    func_L00_00205FF0();
    CHECK(approached == 1 && color(object) == 0 && countdowns == 0);
    g[0x20A4] = 1;
    func_L00_00205FF0();
    CHECK(color(other) == 0x804CA044u);
    reset();
    *(unsigned char**)(g + 0x1184) = NULL;
    func_L00_00205FF0();
    CHECK(approached == 1 && countdowns == 0);
    reset();
    zero_period = 1;
    if (setjmp(trap) == 0) {
        func_L00_00205FF0();
        CHECK(0);
    }
    CHECK(color(object) == 0 && countdowns == 0);
}

static void flash(void) {
    const short timers[] = {45, 42, 40, 20, 10, 0};
    const unsigned int colors[] =
        {0x804CA044u, 0x80456780u, 0x804040A8u, 0x804040A8u, 0x80467076u, 0x804CA044u};
    unsigned i;
    for (i = 0; i < sizeof(timers) / sizeof(*timers); ++i) {
        reset();
        *(short*)(g + 0x1EE) = timers[i];
        func_L00_00205FF0();
        CHECK(color(object) == colors[i]);
        CHECK(*(short*)(g + 0x1EE) == timers[i]);
    }
    reset();
    tick_scale = 2;
    *(short*)(g + 0x1EE) = 20;
    func_L00_00205FF0();
    CHECK(color(object) == 0x80467076u);
}

static void attachments(void) {
    int i, j;
    unsigned char* node;
    reset();
    object[0x53] = 1;
    *(short*)(g + 0xFFC) = 1;
    tick_scale = 0.5f;
    func_L00_00205FF0();
    CHECK(randomized == 1 && random_low == 25 && random_high == 100);
    CHECK(*(short*)(g + 0xFFC) == 123 && *(short*)(g + 0xFFE) == 2);
    CHECK(attached == 4 && scaled == 4 && detached == 0);
    for (i = 0; i < 4; ++i) {
        node = g + 0xEF0 + i * 0x40;
        CHECK(seen_joints[i] == hero_fx_joints[i] && node[3] == 1);
        CHECK(memcmp(node + 0x10, hero_fx_template_a + i * 4, 16) == 0);
        CHECK(memcmp(node + 0x20, hero_fx_template_b + i * 4, 16) == 0);
        for (j = 0; j < 4; ++j) {
            NEAR(((float*)(node + 0x30))[j], (i* 4 + j + 0.5f) * 2);
        }
        NEAR(*(float*)(node + 0xC), 0.25f);
    }
    *(float*)(object + 0x2C) = 10;
    func_L00_00205FF0();
    CHECK(attached == 4 && scaled == 4 && randomized == 1);
    NEAR(*(float*)(g + 0xF20), 1); /* active nodes are not rescaled */
    NEAR(*(float*)(g + 0xEFC), 0.375f);
    *(short*)(g + 0xFFE) = 20;
    func_L00_00205FF0();
    CHECK(*(short*)(g + 0xFFE) == 21 && detached == 0);
    NEAR(*(float*)(g + 0xEFC), 2.625f);
    g[0xEF1 + 0x40] = 0;
    func_L00_00205FF0();
    CHECK(*(short*)(g + 0xFFE) == 0 && detached == 3);
    CHECK(seen_detach[0] == 0 && seen_detach[1] == 2 && seen_detach[2] == 3);
    reset();
    *(short*)(g + 0xFFC) = 0;
    func_L00_00205FF0();
    CHECK(randomized == 0 && attached == 0); /* wrong animation */
    object[0x53] = 1;
    func_L00_00205FF0();
    CHECK(randomized == 1 && attached == 4); /* already-expired timer */
}

int main(void) {
    selection_and_color();
    flash();
    attachments();
    puts("hero effects tests passed");
    return 0;
}
