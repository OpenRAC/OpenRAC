// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The menu's sounds (ui_sounds.h): rounded "pops", short sines whose pitch falls (or, opening,
// rises) as they die, with a little filtered air under the menu's opening and closing. Each layer
// starts softly (a raised cosine, so there is never a click) and falls exponentially. The layers
// are summed on a bus that takes the top off (a one-pole low-pass at 5 kHz) and puts the sound in
// a small room: a feedback delay network of four lines a side, about 0.2 s long, mixed in low.

#include "audio/ui_sounds.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <mutex>
#include <numbers>
#include <vector>

namespace openrac::audio::ui_sounds {

namespace {

using Clock = std::chrono::steady_clock;

enum class Kind : std::uint8_t {
    Body,
    Air
};

// One layer of a sound.
struct Layer {
    Kind kind;
    float delay_ms;        // after the sound starts
    float from_hz, to_hz;  // the body's pitch, or the centre of the air's band
    float glide_ms;        // from_hz to to_hz over this, easing out
    float attack_ms;
    float decay_ms;  // the time the level takes to fall to 1/e after the attack
    float length_ms;
    float gain;      // 0..1 of full scale
    float q = 1.0f;  // the air's band: higher is narrower
};

struct Shape {
    std::array<Layer, 2> layers;
    int count;
};

// The levels: the loudest near -18 dB.
Shape shape_of(Sound s) {
    switch (s) {
        case Sound::Cursor:
            return {{{{Kind::Body, 0, 1400, 800, 14, 1, 9, 45, 0.07f}}}, 1};
        case Sound::Confirm:
            return {
                {{{Kind::Body, 0, 900, 420, 25, 1, 22, 120, 0.13f},
                  {Kind::Body, 0, 1800, 840, 25, 1, 10, 60, 0.02f}}},
                2
            };
        case Sound::Back:
            return {{{{Kind::Body, 0, 600, 330, 30, 1, 20, 110, 0.11f}}}, 1};
        case Sound::Denied:
            return {
                {{{Kind::Body, 0, 420, 300, 20, 1, 12, 70, 0.11f},
                  {Kind::Body, 80, 420, 300, 20, 1, 12, 70, 0.09f}}},
                2
            };
        case Sound::Open:
            return {
                {{{Kind::Body, 0, 500, 1100, 90, 6, 45, 220, 0.09f},
                  {Kind::Air, 60, 800, 2000, 120, 60, 50, 220, 0.04f, 1.6f}}},
                2
            };
        case Sound::Close:
            return {
                {{{Kind::Body, 0, 1100, 480, 90, 4, 45, 220, 0.08f},
                  {Kind::Air, 0, 2000, 700, 120, 30, 50, 200, 0.035f, 1.6f}}},
                2
            };
        case Sound::Page:
            return {{{{Kind::Body, 0, 700, 1150, 40, 2, 18, 90, 0.08f}}}, 1};
    }
    return {{}, 0};
}

// A layer sounding.
struct Voice {
    Layer layer{};
    int sound = -1;                 // which Sound it is part of; -1 free
    std::uint32_t age = 0;          // the order the voices started in
    int at = 0;                     // samples since the sound started
    double phase = 0.0;             // the body's
    float low = 0.0f, band = 0.0f;  // the air's filter
    std::uint32_t noise = 0x9E3779B9u;
    float fade = 1.0f;  // 1, falling once released
    bool released = false;
};

constexpr std::size_t kVoices = 16;
// A cursor sound at most this often; the arrows' repeats in between make none.
constexpr auto kCursorGap = std::chrono::milliseconds(55);
// No layer starts faster: a sharper start is heard as a click.
constexpr float kMinAttackMs = 0.5f;
// A released voice is gone after this.
constexpr float kReleaseMs = 12.0f;
// The bus: its low-pass, and how much of the room is heard.
constexpr float kLowPassHz = 5000.0f;
constexpr float kWet = 0.14f;
// The room: four lines a side (samples, primes, 7 to 17 ms), their echoes falling 60 dB in 0.2 s.
constexpr std::array<std::array<int, 4>, 2> kLines{{{347, 431, 563, 797}, {373, 467, 599, 829}}};
constexpr float kRoomSeconds = 0.2f;
// Once the voices are over, the room rings on this long before the bus rests.
constexpr int kTailSamples = kRate * 3 / 10;

// One side's room: a feedback delay network (a Householder mix of four lines).
struct Room {
    std::array<std::vector<float>, 4> lines;
    std::array<float, 4> feedback{};
    std::array<int, 4> at{};
    float scale = 1.0f;  // brings the room's impulse response to unit energy

    explicit Room(const std::array<int, 4>& lengths) {
        for (std::size_t i = 0; i < 4; ++i) {
            lines[i].assign(static_cast<std::size_t>(lengths[i]), 0.0f);
            // Each line's own loss, so that every echo falls at the same rate in time.
            feedback[i] =
                std::pow(10.0f, -3.0f * static_cast<float>(lengths[i]) / (kRoomSeconds * kRate));
        }
        float energy = 0.0f;
        for (int n = 0; n < kRate / 2; ++n) {
            const float y = step(n == 0 ? 1.0f : 0.0f);
            energy += y * y;
        }
        scale = 1.0f / std::sqrt(std::max(energy, 1e-9f));
        clear();
    }

    float step(float x) {
        std::array<float, 4> out{};
        float sum = 0.0f;
        for (std::size_t i = 0; i < 4; ++i) {
            out[i] = lines[i][static_cast<std::size_t>(at[i])];
            sum += out[i];
        }
        for (std::size_t i = 0; i < 4; ++i) {
            // Householder: each line takes the others' and gives back its own, inverted.
            const float mixed = out[i] - 0.5f * sum;
            lines[i][static_cast<std::size_t>(at[i])] = x + feedback[i] * mixed;
            at[i] = (at[i] + 1) % static_cast<int>(lines[i].size());
        }
        return 0.5f * sum * scale;
    }

    void clear() {
        for (auto& line : lines) {
            std::fill(line.begin(), line.end(), 0.0f);
        }
        at = {};
    }
};

struct State {
    std::mutex lock;
    std::array<Voice, kVoices> voices{};
    std::uint32_t started = 0;
    Clock::time_point cursor_at{};
    bool cursor_played = false;
    std::vector<float> bus;  // a block's dry sum
    float low_pass = 0.0f;
    Room left{kLines[0]}, right{kLines[1]};
    int quiet = kTailSamples;  // samples since the last voice ended
};

State& state() {
    static State s;
    return s;
}

float ms_to_samples(float ms) {
    return ms * static_cast<float>(kRate) / 1000.0f;
}

// White noise, -1..1.
float white(std::uint32_t& x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return static_cast<float>(static_cast<std::int32_t>(x)) / 2147483648.0f;
}

// Adds one voice's samples to `bus`.
void render(Voice& v, std::span<float> bus) {
    constexpr double kTau = 2.0 * std::numbers::pi;
    constexpr float kPi = std::numbers::pi_v<float>;
    const float release_step = 1.0f / ms_to_samples(kReleaseMs);
    const Layer& l = v.layer;
    const int delay = static_cast<int>(ms_to_samples(l.delay_ms));
    const int length = delay + static_cast<int>(ms_to_samples(l.length_ms));
    const float attack = std::max(ms_to_samples(kMinAttackMs), ms_to_samples(l.attack_ms));
    const float decay = std::max(1.0f, ms_to_samples(l.decay_ms));
    const float glide = ms_to_samples(l.glide_ms);
    const float tail_len = ms_to_samples(10.0f);
    for (float& out : bus) {
        if (v.at >= length || v.fade <= 0.0f) {
            v.sound = -1;
            return;
        }
        if (v.at < delay) {
            if (v.released) {
                v.sound = -1;  // released before it began: it never sounds
                return;
            }
            ++v.at;
            continue;
        }
        const float n = static_cast<float>(v.at - delay);
        const float k = glide > 0.0f ? std::min(n / glide, 1.0f) : 1.0f;
        const float ease = 1.0f - (1.0f - k) * (1.0f - k);
        // The pitch moves by ratio, as the ear hears it.
        const float hz = l.from_hz * std::pow(l.to_hz / l.from_hz, ease);
        float x = 0.0f;
        if (l.kind == Kind::Body) {
            v.phase += kTau * hz / kRate;
            if (v.phase > kTau) {
                v.phase -= kTau;
            }
            x = static_cast<float>(std::sin(v.phase));
        } else {
            // A state-variable band-pass at `hz`, brought back to about a sine's level.
            const float g = 2.0f * std::sin(kPi * std::min(hz, 12000.0f) / kRate);
            const float damp = 1.0f / l.q;
            const float high = white(v.noise) - v.low - damp * v.band;
            v.band += g * high;
            v.low += g * v.band;
            x = v.band * damp * 2.5f;
        }
        // A raised-cosine start, then the fall; the last 10 ms brought to 0.
        const float env =
            n < attack ? 0.5f - 0.5f * std::cos(kPi * n / attack) : std::exp(-(n - attack) / decay);
        const float tail = std::min(1.0f, static_cast<float>(length - v.at) / tail_len);
        if (v.released) {
            v.fade -= release_step;
        }
        out += x * env * tail * std::max(v.fade, 0.0f) * l.gain;
        ++v.at;
    }
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
        *slot = Voice{shape.layers[static_cast<std::size_t>(i)], id, ++s.started};
        slot->noise += s.started * 0x6D2B79F5u;  // each burst its own grain
    }
    s.quiet = 0;
}

void mix(std::span<std::int16_t> interleaved) {
    State& s = state();
    std::lock_guard guard(s.lock);
    const std::size_t frames = interleaved.size() / 2;
    if (s.quiet >= kTailSamples) {
        return;  // nothing sounding, the room at rest
    }
    s.bus.assign(frames, 0.0f);
    for (Voice& v : s.voices) {
        if (v.sound >= 0) {
            render(v, s.bus);
        }
    }
    const float a = std::exp(-2.0f * std::numbers::pi_v<float> * kLowPassHz / kRate);
    for (std::size_t f = 0; f < frames; ++f) {
        s.low_pass = (1.0f - a) * s.bus[f] + a * s.low_pass;
        const float dry = s.low_pass;
        const float sides[2] = {dry + kWet * s.left.step(dry), dry + kWet * s.right.step(dry)};
        for (std::size_t c = 0; c < 2; ++c) {
            std::int16_t& out = interleaved[f * 2 + c];
            out = static_cast<std::int16_t>(
                std::clamp(static_cast<float>(out) + sides[c] * 32767.0f, -32768.0f, 32767.0f)
            );
        }
    }
    const bool still =
        std::any_of(s.voices.begin(), s.voices.end(), [](const Voice& v) { return v.sound >= 0; });
    s.quiet = still ? 0 : s.quiet + static_cast<int>(frames);
    if (s.quiet >= kTailSamples) {
        s.left.clear();
        s.right.clear();
        s.low_pass = 0.0f;
    }
}

bool sounding() {
    State& s = state();
    std::lock_guard guard(s.lock);
    return s.quiet < kTailSamples;
}

void reset() {
    State& s = state();
    std::lock_guard guard(s.lock);
    s.voices = {};
    s.cursor_played = false;
    s.left.clear();
    s.right.clear();
    s.low_pass = 0.0f;
    s.quiet = kTailSamples;
}

}  // namespace openrac::audio::ui_sounds
