// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu's tones (ui_sounds.h): each sound is a few sine partials, each with a soft start (a few
// milliseconds, so there is never a click), an exponential fall and a slight glide in pitch, as a
// console's menu sounds are: rounded, quiet, under half a second.

#include "audio/ui_sounds.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <mutex>
#include <numbers>

namespace openrac::audio::ui_sounds {

namespace {

using Clock = std::chrono::steady_clock;

// One partial of a sound.
struct Tone {
    float delay_ms;  // after the sound starts
    float from_hz, to_hz;
    float glide_ms;  // the pitch moves from from_hz to to_hz over this
    float attack_ms;
    float decay_ms;  // the time the level takes to fall to 1/e
    float length_ms;
    float gain;  // 0..1 of full scale
    float pan;   // -1 left .. 1 right
};

struct Shape {
    std::array<Tone, 4> tones;
    int count;
};

// Soft and low in level: the loudest peaks near -16 dB, the cursor's well under it.
// The notes are of one chord (E major: E5, G#5, B5, E6), so every sound fits the others.
constexpr float kE5 = 659.26f, kGs5 = 830.61f, kB5 = 987.77f, kE6 = 1318.51f, kB4 = 493.88f;

constexpr Shape shape_of(Sound s) {
    switch (s) {
        case Sound::Cursor:
            // A short, rounded tick: a high note with its octave faint over it.
            return {
                {{{0, kB5 * 1.01f, kB5, 12, 1.5f, 14, 60, 0.11f, 0},
                  {0, kB5 * 2.0f, kB5 * 2.0f, 0, 1.0f, 7, 30, 0.025f, 0}}},
                2
            };
        case Sound::Confirm:
            // Two notes up, the second ringing a little longer.
            return {
                {{{0, kGs5, kGs5, 0, 2, 40, 120, 0.14f, -0.15f},
                  {45, kE6 * 0.995f, kE6, 20, 2, 70, 260, 0.12f, 0.15f},
                  {45, kE6 * 2.0f, kE6 * 2.0f, 0, 2, 25, 90, 0.02f, 0.15f}}},
                3
            };
        case Sound::Back:
            // Two notes down, softer.
            return {
                {{{0, kB5, kB5, 0, 2, 35, 110, 0.11f, 0.1f},
                  {40, kE5, kE5 * 0.995f, 40, 2, 60, 220, 0.11f, -0.1f}}},
                2
            };
        case Sound::Denied:
            // A low muted double knock.
            return {
                {{{0, kB4 * 1.02f, kB4 * 0.97f, 40, 2, 25, 80, 0.13f, 0},
                  {70, kB4 * 1.02f, kB4 * 0.97f, 40, 2, 25, 80, 0.10f, 0}}},
                2
            };
        case Sound::Open:
            // A rising chord, spread across the sides.
            return {
                {{{0, kE5, kE5, 0, 6, 80, 260, 0.09f, -0.3f},
                  {35, kGs5, kGs5, 0, 6, 80, 260, 0.08f, 0.0f},
                  {70, kB5, kB5, 0, 6, 90, 300, 0.08f, 0.3f},
                  {105, kE6, kE6, 0, 6, 110, 320, 0.06f, 0.0f}}},
                4
            };
        case Sound::Page:
            // A light slide up, as a page passing.
            return {
                {{{0, kGs5, kB5, 35, 3, 30, 110, 0.09f, 0},
                  {0, kGs5 * 2.0f, kB5 * 2.0f, 35, 3, 15, 60, 0.015f, 0}}},
                2
            };
    }
    return {{}, 0};
}

// A partial sounding.
struct Voice {
    Tone tone{};
    int sound = -1;         // which Sound it is part of; -1 free
    std::uint32_t age = 0;  // the order the voices started in
    int at = 0;             // samples since the sound started
    double phase = 0.0;
    float fade = 1.0f;  // 1, falling once released
    bool released = false;
};

constexpr std::size_t kVoices = 16;
// A cursor sound at most this often; the arrows' repeats in between make none.
constexpr auto kCursorGap = std::chrono::milliseconds(55);
// No partial starts faster: a sharper start is heard as a click.
constexpr float kMinAttackMs = 4.0f;
// A released voice is gone after this.
constexpr float kReleaseMs = 12.0f;

struct State {
    std::mutex lock;
    std::array<Voice, kVoices> voices{};
    std::uint32_t started = 0;
    Clock::time_point cursor_at{};
    bool cursor_played = false;
};

State& state() {
    static State s;
    return s;
}

float ms_to_samples(float ms) {
    return ms * static_cast<float>(kRate) / 1000.0f;
}

}  // namespace

void play(Sound sound) {
    State& s = state();
    const Shape shape = shape_of(sound);
    const int id = static_cast<int>(sound);
    const Clock::time_point now = Clock::now();
    std::lock_guard guard(s.lock);
    if (sound == Sound::Cursor) {
        if (s.cursor_played && now - s.cursor_at < kCursorGap) {
            return;
        }
        s.cursor_at = now;
        s.cursor_played = true;
    }
    // The same sound again: the one before it fades out under it, so they never stack.
    for (Voice& v : s.voices) {
        if (v.sound == id) {
            v.released = true;
        }
    }
    for (int i = 0; i < shape.count; ++i) {
        // A free voice, or else the oldest.
        Voice* slot = &s.voices[0];
        for (Voice& v : s.voices) {
            if (v.sound < 0) {
                slot = &v;
                break;
            }
            if (v.age < slot->age) {
                slot = &v;
            }
        }
        *slot = Voice{shape.tones[static_cast<std::size_t>(i)], id, ++s.started};
    }
}

void mix(std::span<std::int16_t> interleaved) {
    State& s = state();
    std::lock_guard guard(s.lock);
    const std::size_t frames = interleaved.size() / 2;
    const float release_step = 1.0f / ms_to_samples(kReleaseMs);
    constexpr double kTau = 2.0 * std::numbers::pi;
    for (Voice& v : s.voices) {
        if (v.sound < 0) {
            continue;
        }
        const Tone& t = v.tone;
        const int delay = static_cast<int>(ms_to_samples(t.delay_ms));
        const int length = delay + static_cast<int>(ms_to_samples(t.length_ms));
        const float attack = std::max(ms_to_samples(kMinAttackMs), ms_to_samples(t.attack_ms));
        const float decay = std::max(1.0f, ms_to_samples(t.decay_ms));
        const float glide = ms_to_samples(t.glide_ms);
        // Equal power across the sides.
        const float angle = (t.pan + 1.0f) * 0.25f * std::numbers::pi_v<float>;
        const float left = std::cos(angle) * t.gain * 32767.0f;
        const float right = std::sin(angle) * t.gain * 32767.0f;
        for (std::size_t f = 0; f < frames; ++f, ++v.at) {
            if (v.at >= length || v.fade <= 0.0f) {
                v.sound = -1;
                break;
            }
            if (v.at < delay) {
                if (v.released) {
                    v.sound = -1;  // released before it began: it never sounds
                    break;
                }
                continue;
            }
            const float n = static_cast<float>(v.at - delay);
            const float k = glide > 0.0f ? std::min(n / glide, 1.0f) : 1.0f;
            // The glide eases out, as a voice settling on its note.
            const float hz = t.from_hz + (t.to_hz - t.from_hz) * (1.0f - (1.0f - k) * (1.0f - k));
            v.phase += kTau * hz / kRate;
            if (v.phase > kTau) {
                v.phase -= kTau;
            }
            // A raised-cosine start, the exponential fall, and the last 10 ms brought to 0.
            const float rise =
                n < attack ? 0.5f - 0.5f * std::cos(std::numbers::pi_v<float> * n / attack) : 1.0f;
            const float tail =
                std::min(1.0f, static_cast<float>(length - v.at) / ms_to_samples(10.0f));
            if (v.released) {
                v.fade -= release_step;
            }
            const float level = rise * std::exp(-n / decay) * tail * std::max(v.fade, 0.0f);
            const float x = static_cast<float>(std::sin(v.phase)) * level;
            for (int c = 0; c < 2; ++c) {
                std::int16_t& out = interleaved[f * 2 + static_cast<std::size_t>(c)];
                const float sum = static_cast<float>(out) + x * (c == 0 ? left : right);
                out = static_cast<std::int16_t>(std::clamp(sum, -32768.0f, 32767.0f));
            }
        }
    }
}

bool sounding() {
    State& s = state();
    std::lock_guard guard(s.lock);
    return std::any_of(s.voices.begin(), s.voices.end(), [](const Voice& v) {
        return v.sound >= 0;
    });
}

void reset() {
    State& s = state();
    std::lock_guard guard(s.lock);
    s.voices = {};
    s.cursor_played = false;
}

}  // namespace openrac::audio::ui_sounds
