// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "game/common/frontend/touch_controls.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

#include "common/log.h"
#include "mod_touch.h"
#include "openrac/game_host.h"
#include "platform/touch_actions.h"
#include "renderer/ui/touch_icons.h"
#include "renderer/ui/touch_overlay.h"
#include "ui_colors.h"

// game/common/touch/ as built into the program (cmake/Games.cmake, openrac_touch_files): each file
// by its path under that folder ("icons/jump.svg", "sets/rac1.json").
struct OpenracTouchFile {
    const char* path;
    const unsigned char* data;
    unsigned int size;
};

extern const OpenracTouchFile openrac_touch_files[];
extern const unsigned int openrac_touch_file_count;

namespace openrac::frontend::touch_controls {
namespace {

// The frames a new context must hold before the pad takes it, and a hint before it shows; a hint
// gone stays this many frames more (a swingshot target the hero turns from for a moment).
constexpr int kContextFrames = 6;
constexpr int kHintFrames = 3;
constexpr int kHintLinger = 20;

struct Hint {
    std::string name;
    int up = 0;    // frames it has been reported
    int down = 0;  // frames since it last was
};

struct State {
    std::mutex lock;
    bool loaded = false;
    unsigned mods_generation = 0;  // of the mods' sets and icons taken in (mod_touch.h)
    platform::TouchActionSets sets;
    std::string shown = "default";
    std::string candidate;
    int candidate_frames = 0;
    std::vector<Hint> hints;
};

std::atomic<bool> g_movie{false};

State& state() {
    static State s;
    return s;
}

std::string_view text(const OpenracTouchFile& f) {
    return {reinterpret_cast<const char*>(f.data), f.size};
}

// The built-in sets and icons, the first time the pad needs them and again when a mod adds its own;
// the mods' over them.
void load(State& s, const mods::touch::Snapshot& mod) {
    if (!mod.files) {
        return;
    }
    s.loaded = true;
    s.mods_generation = mod.generation;
    s.sets.clear();
    const std::string game = openrac_game.game != nullptr ? openrac_game.game : "";
    const std::string own = "sets/" + game + ".json";
    std::string error;
    for (const char* wanted : {"sets/common.json", own.c_str()}) {
        for (unsigned int i = 0; i < openrac_touch_file_count; ++i) {
            if (std::strcmp(openrac_touch_files[i].path, wanted) == 0
                && !s.sets.read(text(openrac_touch_files[i]), wanted, error)) {
                log::warn("touch controls: {}", error);
            }
        }
    }
    for (unsigned int i = 0; i < openrac_touch_file_count; ++i) {
        const std::string_view path = openrac_touch_files[i].path;
        if (path.starts_with("icons/") && path.ends_with(".svg")) {
            const std::string_view name = path.substr(6, path.size() - 10);
            if (!renderer::ui::touch_icons::add(name, text(openrac_touch_files[i]))) {
                log::warn("touch controls: the icon {} cannot be drawn", path);
            }
        }
    }
    for (const auto& [name, json] : mod.sets) {
        if (!s.sets.read(json, name, error)) {
            log::warn("touch controls: {}", error);
        }
    }
    for (const auto& [name, svg] : mod.icons) {
        if (!renderer::ui::touch_icons::add(name, svg)) {
            log::warn("touch controls: a mod's icon {} cannot be drawn", name);
        }
    }
}

// The game's context now, and its hints.
std::string game_context(std::vector<std::string>& hints) {
    hints.clear();
    if (g_movie.load(std::memory_order_relaxed)) {
        return "cutscene";
    }
    if (openrac_game_touch == nullptr) {
        return "default";
    }
    const char* context = nullptr;
    const char* raw[OPENRAC_TOUCH_HINTS_MAX] = {};
    const int count = openrac_game_touch(&context, raw);
    if (count < 0 || context == nullptr || *context == '\0') {
        return "default";
    }
    for (int i = 0; i < std::min(count, OPENRAC_TOUCH_HINTS_MAX); ++i) {
        if (raw[i] != nullptr && *raw[i] != '\0') {
            hints.emplace_back(raw[i]);
        }
    }
    return context;
}

void set_accent() {
    const ui_colors::Color c = ui_colors::launcher().amber;
    renderer::ui::touch_overlay::set_accent(
        static_cast<float>(c & 0xFFu) / 255.0f,
        static_cast<float>((c >> 8) & 0xFFu) / 255.0f,
        static_cast<float>((c >> 16) & 0xFFu) / 255.0f
    );
}

}  // namespace

void update(platform::TouchPad& touch, int style) {
    State& s = state();
    const std::lock_guard guard(s.lock);
    const mods::touch::Snapshot mod = mods::touch::snapshot(s.loaded ? s.mods_generation : ~0u);
    load(s, mod);
    set_accent();
    std::vector<std::string> reported;
    std::string context = game_context(reported);
    if (!mod.context.empty()) {
        context = mod.context;  // a mod's own context over the game's
    }
    reported.insert(reported.end(), mod.hints.begin(), mod.hints.end());
    // A new context once it has held for kContextFrames.
    if (context == s.shown) {
        s.candidate.clear();
        s.candidate_frames = 0;
    } else if (context == s.candidate) {
        if (++s.candidate_frames >= kContextFrames) {
            log::verbose("touch controls: {}", context);
            s.shown = context;
            s.candidate.clear();
            s.candidate_frames = 0;
        }
    } else {
        s.candidate = context;
        s.candidate_frames = 1;
    }
    // Hints: up after a few frames, down after more.
    for (Hint& h : s.hints) {
        const bool now = std::find(reported.begin(), reported.end(), h.name) != reported.end();
        h.up = now ? h.up + 1 : h.up;
        h.down = now ? 0 : h.down + 1;
    }
    std::erase_if(s.hints, [](const Hint& h) { return h.down > kHintLinger; });
    for (const std::string& r : reported) {
        if (std::none_of(s.hints.begin(), s.hints.end(), [&](const Hint& h) {
                return h.name == r;
            })) {
            s.hints.push_back(Hint{r, 1, 0});
        }
    }
    std::vector<std::string> up;
    for (const Hint& h : s.hints) {
        if (h.up >= kHintFrames) {
            up.push_back(h.name);
        }
    }
    std::string_view shown = s.shown;
    std::vector<platform::TouchAction> actions = s.sets.actions(shown);
    if (actions.empty()) {
        shown = "default";
        actions = s.sets.actions(shown);
    }
    const bool dpad = s.sets.dpad(shown);
    // No set for it (a game that reports none): the DualShock's buttons.
    const bool arc = style == 0 && !actions.empty();
    touch.set_style(arc ? platform::TouchStyle::Arc : platform::TouchStyle::Classic);
    touch.set_actions(std::move(actions), std::move(up), dpad);
}

void set_movie(bool skippable) {
    g_movie.store(skippable, std::memory_order_relaxed);
}

}  // namespace openrac::frontend::touch_controls
