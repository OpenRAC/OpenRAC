// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "frontend/shortcuts.h"

#include <array>
#include <optional>
#include <string>

#include "controls_store.h"
#include "frontend.h"
#include "settings.h"

namespace openrac::frontend::shortcuts {

namespace {

// clang-format off
constexpr std::array<Shortcut, static_cast<std::size_t>(Id::Count)> kAll = {{
    {Id::Screenshot, "screenshot", "Screenshot", "Game", "F12"},
    {Id::FullScreen, "full_screen", "Full screen", "Game", "F11,Alt+Enter"},
    {Id::VolumeDown, "volume_down", "Volume down", "Game", "F9"},
    {Id::VolumeUp, "volume_up", "Volume up", "Game", "F10"},
    {Id::FrameCounter, "frame_counter", "Frame counter", "Game", "F2"},
    {Id::EnhancedGraphics, "enhanced_graphics", "Enhanced graphics", "Game", "F4"},
    {Id::TexturePack, "texture_pack", "Show or hide the texture pack", "Game", "Ctrl+F2"},
    {Id::StatsPanel, "stats_panel", "Stats panel", "Developer", "F3"},
    {Id::RecordDiagnostics, "record_diagnostics", "Record diagnostics", "Developer", "Ctrl+F3"},
    {Id::Pause, "pause", "Pause and resume", "Developer", "F5"},
    {Id::StepTick, "step_tick", "Step one tick", "Developer", "F6"},
    {Id::Slower, "slower", "Slower", "Developer", "F7"},
    {Id::Faster, "faster", "Faster", "Developer", "F8"},
    {Id::LevelSelect, "level_select", "Level select", "Developer", "F1"},
}};
// clang-format on

constexpr bool in_order() {
    for (std::size_t i = 0; i < kAll.size(); ++i) {
        if (static_cast<std::size_t>(kAll[i].id) != i) {
            return false;
        }
    }
    return true;
}

static_assert(in_order(), "shortcuts.cpp: one entry per Id, in the Id's order");

// The keyboard a menu row's press comes from: none SDL numbers (its ids count up from 1).
constexpr SDL_KeyboardID kMenuPress = 0xFFFF'F00Du;

std::string file_key(Id id) {
    return std::string("shortcut_") + of(id).key;
}

// What the controls file says, or nothing (no file yet, no key).
std::optional<std::string> stored(Id id) {
    controls::Store* store = controls_store();
    if (store == nullptr) {
        return std::nullopt;
    }
    return store->file(controls::Scope::Global).get(file_key(id));
}

}  // namespace

std::span<const Shortcut> all() {
    return kAll;
}

const Shortcut& of(Id id) {
    return kAll[static_cast<std::size_t>(id)];
}

std::vector<platform::ShortcutKey> keys(Id id) {
    if (const std::optional<std::string> text = stored(id)) {
        return platform::parse_shortcut(*text);
    }
    // The texture pack's key was a setting of each version's settings file before shortcuts had a
    // place of their own: one changed there stays until the shortcut is set.
    if (id == Id::TexturePack) {
        return platform::parse_shortcut(settings::get().texture_pack_key);
    }
    return platform::parse_shortcut(of(id).defaults);
}

std::string label(Id id) {
    return platform::shortcut_label(keys(id));
}

bool hit(Id id, const SDL_KeyboardEvent& key) {
    if (key.which == kMenuPress) {
        return key.raw == static_cast<Uint16>(id);
    }
    return platform::shortcut_hit(keys(id), key);
}

void as_press(Id id, SDL_KeyboardEvent& key) {
    key.which = kMenuPress;
    key.raw = static_cast<Uint16>(id);
    key.scancode = SDL_SCANCODE_UNKNOWN;
}

bool set(Id id, const std::vector<platform::ShortcutKey>& keys) {
    controls::Store* store = controls_store();
    return store != nullptr
           && store->set(controls::Scope::Global, file_key(id), platform::shortcut_text(keys));
}

bool reset(Id id) {
    controls::Store* store = controls_store();
    if (store == nullptr) {
        return false;
    }
    if (id == Id::TexturePack) {
        settings::get().texture_pack_key = settings::Settings{}.texture_pack_key;
        settings::save();
    }
    return !store->file(controls::Scope::Global).get(file_key(id))
           || store->erase(controls::Scope::Global, file_key(id));
}

}  // namespace openrac::frontend::shortcuts
