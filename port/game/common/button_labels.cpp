// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "button_labels.h"

#include <algorithm>
#include <cstring>
#include <mutex>

#include "key_labels.h"
#include "openrac/openrac_mod.h"

namespace openrac::button_labels {

namespace {

// A pad button: its bit, its icon cell (key_prompts.h's order, -1 none), the game font's character
// for its icon (0 none), and its names on a PlayStation, an Xbox and a Nintendo pad (by place: the
// game's Cross is the bottom button, a Nintendo pad's B).
struct Button {
    std::uint32_t bit;
    int cell;
    char token;
    const char* playstation;
    const char* xbox;
    const char* nintendo;
};

constexpr Button kButtons[] = {
    {OPENRAC_PAD_CROSS, 0, '\x10', "CROSS", "A", "B"},
    {OPENRAC_PAD_SQUARE, 1, '\x13', "SQUARE", "X", "Y"},
    {OPENRAC_PAD_TRIANGLE, 2, '\x12', "TRIANGLE", "Y", "X"},
    {OPENRAC_PAD_CIRCLE, 3, '\x11', "CIRCLE", "B", "A"},
    {OPENRAC_PAD_L1, 4, '\x14', "L1", "LB", "L"},
    {OPENRAC_PAD_L2, 5, '\x16', "L2", "LT", "ZL"},
    {OPENRAC_PAD_R1, 6, '\x15', "R1", "RB", "R"},
    {OPENRAC_PAD_R2, 7, '\x17', "R2", "RT", "ZR"},
    {OPENRAC_PAD_L3, 8, '\x18', "L3", "LS", "LS"},
    {OPENRAC_PAD_R3, 9, '\x19', "R3", "RS", "RS"},
    {OPENRAC_PAD_START, -1, 0, "START", "MENU", "+"},
    {OPENRAC_PAD_SELECT, -1, 0, "SELECT", "VIEW", "-"},
    {OPENRAC_PAD_UP, -1, 0, "UP", "UP", "UP"},
    {OPENRAC_PAD_RIGHT, -1, 0, "RIGHT", "RIGHT", "RIGHT"},
    {OPENRAC_PAD_DOWN, -1, 0, "DOWN", "DOWN", "DOWN"},
    {OPENRAC_PAD_LEFT, -1, 0, "LEFT", "LEFT", "LEFT"},
};

// The first of the game's button tokens and the count of them (OPENRAC_UI_CROSS ...).
constexpr unsigned char kFirstToken = 0x10;
constexpr unsigned char kTokens = 10;
// The tokens of the face buttons, which the overlay's font draws as icons.
constexpr unsigned char kLastFaceToken = 0x13;

const Button* find_bit(std::uint32_t bit) {
    for (const Button& b : kButtons) {
        if (b.bit == bit) {
            return &b;
        }
    }
    return nullptr;
}

const Button* find_token(unsigned char token) {
    for (const Button& b : kButtons) {
        if (b.token != 0 && static_cast<unsigned char>(b.token) == token) {
            return &b;
        }
    }
    return nullptr;
}

bool is_token(unsigned char c) {
    return c >= kFirstToken && c < kFirstToken + kTokens;
}

// A key's name as the game's icons show it, on one line: "F", "SPACE", "LMB", "NUM 4".
std::string key_text(std::string_view name) {
    std::string out = keys::icon_label(name);
    std::replace(out.begin(), out.end(), '\n', ' ');
    return out;
}

std::mutex g_lock;
Device g_device;
KeyNames g_keys;

}  // namespace

void set_device(const Device& device) {
    const std::lock_guard<std::mutex> lock(g_lock);
    g_device = device;
}

void set_key_names(const KeyNames& names) {
    const std::lock_guard<std::mutex> lock(g_lock);
    g_keys = names;
}

Device device() {
    const std::lock_guard<std::mutex> lock(g_lock);
    return g_device;
}

std::string label(std::uint32_t button, const Device& device, const KeyNames& keys, int& kind) {
    const Button* b = find_bit(button);
    if (b == nullptr) {
        kind = OPENRAC_HOOK_UNKNOWN;
        return {};
    }
    // On the keyboard, the key the game's icon shows; a button whose icon keeps the game's own
    // (no key presses it here, or the key prompts are off) is the game's.
    if (device.keys) {
        if (b->cell >= 0 && !keys[static_cast<std::size_t>(b->cell)].empty()) {
            kind = OPENRAC_BUTTONS_KEYS;
            return key_text(keys[static_cast<std::size_t>(b->cell)]);
        }
    } else if (device.pad != Pad::PlayStation) {
        kind = OPENRAC_BUTTONS_LETTERS;
        return device.pad == Pad::Xbox ? b->xbox : b->nintendo;
    }
    kind = OPENRAC_BUTTONS_PLAYSTATION;
    return b->token != 0 ? std::string(1, b->token) : std::string(b->playstation);
}

int label_into(std::uint32_t button, char* out, std::uint32_t size) {
    if (out == nullptr || size == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    Device d;
    KeyNames k;
    {
        const std::lock_guard<std::mutex> lock(g_lock);
        d = g_device;
        k = g_keys;
    }
    int kind = OPENRAC_HOOK_UNKNOWN;
    const std::string text = label(button, d, k, kind);
    if (kind < 0) {
        out[0] = '\0';
        return kind;
    }
    const std::size_t n = std::min<std::size_t>(text.size(), size - 1u);
    std::memcpy(out, text.data(), n);
    out[n] = '\0';
    return kind;
}

std::string overlay_text(std::string_view text, const Device& device, const KeyNames& keys) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        const Button* b = is_token(byte) ? find_token(byte) : nullptr;
        if (b == nullptr) {
            out += c;
            continue;
        }
        int kind = OPENRAC_HOOK_UNKNOWN;
        const std::string one = label(b->bit, device, keys, kind);
        if (kind == OPENRAC_BUTTONS_KEYS) {
            out += '[' + one + ']';
        } else if (kind == OPENRAC_BUTTONS_PLAYSTATION) {
            // The face buttons' icons are the font's; the shoulders and sticks are named.
            out += byte <= kLastFaceToken ? std::string(1, c) : std::string(b->playstation);
        } else {
            out += one;
        }
    }
    return out;
}

std::string overlay_text(std::string_view text) {
    if (!has_token(text)) {
        return std::string(text);
    }
    Device d;
    KeyNames k;
    {
        const std::lock_guard<std::mutex> lock(g_lock);
        d = g_device;
        k = g_keys;
    }
    return overlay_text(text, d, k);
}

bool has_token(std::string_view text) {
    return std::any_of(text.begin(), text.end(), [](char c) {
        return is_token(static_cast<unsigned char>(c));
    });
}

}  // namespace openrac::button_labels
