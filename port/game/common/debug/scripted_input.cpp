// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Native-only diagnostic input for repeatable viewer camera, bone and gizmo interaction checks.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

#include "common/log.h"
#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {
namespace {
// Individual modifier bits preserve the other key if left and right variants are both held.
SDL_Keymod modifier(SDL_Keycode key) {
    switch (key) {
        case SDLK_LALT:
            return SDL_KMOD_LALT;
        case SDLK_RALT:
            return SDL_KMOD_RALT;
        case SDLK_LSHIFT:
            return SDL_KMOD_LSHIFT;
        case SDLK_RSHIFT:
            return SDL_KMOD_RSHIFT;
        case SDLK_LCTRL:
            return SDL_KMOD_LCTRL;
        case SDLK_RCTRL:
            return SDL_KMOD_RCTRL;
        default:
            return SDL_KMOD_NONE;
    }
}

// SDL's event modifiers, rather than host keyboard state, keep synthetic Alt drags app-local.
void push_key(const ScriptedInput& input, SDL_WindowID window, bool down) {
    const auto mask = modifier(input.key);
    s.script_modifiers =
        SDL_Keymod(down ? (s.script_modifiers | mask) : (s.script_modifiers & ~mask));
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = window;
    event.key.key = input.key;
    event.key.scancode = SDL_GetScancodeFromKey(input.key, nullptr);
    event.key.down = down;
    event.key.mod = s.script_modifiers;
    SDL_PushEvent(&event);
}

// Motion precedes button actions, matching an ordinary click or drag inside this SDL window.
void push_pointer(const ScriptedInput& input, SDL_WindowID window) {
    SDL_Event event{};

    if (input.action == ScriptedAction::Wheel) {
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.windowID = window;
        event.wheel.y = input.x;
        SDL_PushEvent(&event);
        return;
    }

    s.script_pointer = ImVec2{input.x, input.y};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = window;
    event.motion.x = input.x;
    event.motion.y = input.y;
    SDL_PushEvent(&event);

    if (input.action == ScriptedAction::Move) {
        return;
    }

    event = {};
    event.button.windowID = window;
    event.button.button = input.button;
    event.button.clicks = 1;
    event.button.x = input.x;
    event.button.y = input.y;
    event.type = input.action == ScriptedAction::ButtonUp ? SDL_EVENT_MOUSE_BUTTON_UP
                                                          : SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
    SDL_PushEvent(&event);

    if (input.action == ScriptedAction::Click) {
        event.type = SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.down = false;
        SDL_PushEvent(&event);
    }
}

// Parse the same frame/action grammar for inline scripts and diagnostic command files.
Result<std::vector<ScriptedInput>> parse_actions(std::string_view text) {
    const std::string owned(text);
    const char* cursor = owned.c_str();
    std::vector<ScriptedInput> out;

    while (cursor && *cursor) {
        ScriptedInput input;
        char* end = nullptr;
        input.frame = std::strtoull(cursor, &end, 10);

        if (end == cursor) {
            return unexpected(Error{"input action needs a frame number"});
        }

        if (*end == '+') {
            input.held = int(std::strtol(end + 1, &end, 10));
        }

        if (*end != ':') {
            return unexpected(Error{"input action needs FRAME:ACTION"});
        }

        const auto parsed = read_scripted_action(end + 1, input);

        if (!parsed) {
            return unexpected(parsed.error());
        }

        out.push_back(input);

        if (out.size() > 8192) {
            return unexpected(Error{"input script exceeds 8192 actions"});
        }

        cursor = parsed.value() ? parsed.value() + 1 : nullptr;
    }

    return out;
}

// Existing commands must remain unchanged, so updating a file can never replay past clicks or saves.
bool same_action(const ScriptedInput& a, const ScriptedInput& b) {
    return a.frame == b.frame && a.held == b.held && a.action == b.action && a.key == b.key
           && a.button == b.button && a.x == b.x && a.y == b.y;
}

// No file polling occurs unless OPENRAC_KEYS explicitly selects a local command file.
void refresh_actions() {
    if (s.script_file.empty() || SDL_GetTicks() < s.script_poll_at) {
        return;
    }

    s.script_poll_at = SDL_GetTicks() + 100;
    std::error_code error;
    const auto time = std::filesystem::last_write_time(s.script_file, error);

    if (error || time == s.script_file_time) {
        return;
    }

    s.script_file_time = time;
    const auto bytes = std::filesystem::file_size(s.script_file, error);

    if (error || bytes > 1024ull * 1024) {
        log::warn("scripted input: unreadable or oversized command file");
        return;
    }

    std::ifstream file(s.script_file, std::ios::binary);
    // A present but unreadable command file is an I/O failure, not an empty replacement script.
    if (!file) {
        log::warn("scripted input: cannot read command file");
        return;
    }
    const std::string text{std::istreambuf_iterator<char>(file), {}};
    auto parsed = parse_actions(text);

    if (!parsed) {
        log::warn("scripted input: {}", parsed.error().message);
        return;
    }

    auto& actions = parsed.value();

    if (actions.size() < s.script.size()
        || !std::equal(s.script.begin(), s.script.end(), actions.begin(), same_action)) {
        log::warn(
            "scripted input: command files must append actions without changing earlier entries"
        );
        return;
    }

    s.script
        .insert(s.script.end(), actions.begin() + std::ptrdiff_t(s.script.size()), actions.end());
}

// Parse pointer verbs separately from keyboard and wheel actions.
Result<const char*> read_pointer_action(
    const char* text,
    const std::string& verb,
    std::size_t split,
    const char* comma,
    ScriptedInput& input
) {
    struct PointerAction {
        std::string_view name;
        ScriptedAction action;
        std::uint8_t button;
    };

    const std::array<PointerAction, 8> pointer{
        {{"click", ScriptedAction::Click, SDL_BUTTON_LEFT},
         {"move", ScriptedAction::Move, SDL_BUTTON_LEFT},
         {"ldown", ScriptedAction::ButtonDown, SDL_BUTTON_LEFT},
         {"lup", ScriptedAction::ButtonUp, SDL_BUTTON_LEFT},
         {"rdown", ScriptedAction::ButtonDown, SDL_BUTTON_RIGHT},
         {"rup", ScriptedAction::ButtonUp, SDL_BUTTON_RIGHT},
         {"mdown", ScriptedAction::ButtonDown, SDL_BUTTON_MIDDLE},
         {"mup", ScriptedAction::ButtonUp, SDL_BUTTON_MIDDLE}}
    };
    const auto found = std::find_if(pointer.begin(), pointer.end(), [&](const auto& value) {
        return value.name == verb;
    });

    if (found != pointer.end() && split != std::string::npos) {
        input.action = found->action;
        input.button = found->button;

        char* end = nullptr;
        const char* coordinates = text + split + 1;
        input.x = std::strtof(coordinates, &end);

        if (end == coordinates || *end != ',') {
            return unexpected(Error{"invalid pointer X coordinate"});
        }

        const char* second = end + 1;
        input.y = std::strtof(second, &end);

        if (end == second || (*end && *end != ',') || !std::isfinite(input.x)
            || !std::isfinite(input.y)) {
            return unexpected(Error{"invalid pointer Y coordinate"});
        }

        return comma ? std::strchr(comma + 1, ',') : nullptr;
    }

    return comma;
}
}  // namespace

Result<const char*> read_scripted_action(const char* text, ScriptedInput& input) {
    const char* comma = std::strchr(text, ',');
    std::string token(text, comma ? std::size_t(comma - text) : std::strlen(text));
    const auto split = token.find('@');
    const auto verb = token.substr(0, split);

    if (verb == "keydown" || verb == "keyup") {
        input.action = verb == "keydown" ? ScriptedAction::KeyDown : ScriptedAction::KeyUp;
        input.key = SDL_GetKeyFromName(token.substr(split + 1).c_str());

        if (!input.key) {
            return unexpected(Error{"unknown held key: " + token});
        }

        return comma;
    }

    if (verb == "wheel" && split != std::string::npos) {
        char* end = nullptr;
        input.x = std::strtof(text + split + 1, &end);
        input.action = ScriptedAction::Wheel;

        if (!std::isfinite(input.x) || end == text + split + 1
            || (end && *end != 0 && *end != ',')) {
            return unexpected(Error{"invalid wheel action"});
        }

        return comma;
    }

    auto pointer = read_pointer_action(text, verb, split, comma, input);

    if (!pointer || input.action != ScriptedAction::Stroke) {
        return pointer;
    }

    input.action = ScriptedAction::Stroke;
    input.key = SDL_GetKeyFromName(token.c_str());

    if (!input.key) {
        return unexpected(Error{"unknown key or input action: " + token});
    }

    return comma;
}

void run_script(std::uint64_t frame, int held) {
    refresh_actions();

    for (auto& input : s.script) {
        // Rendering can skip a game tick; unheld actions are delivered at the next displayed tick.
        if (input.done || input.frame > frame || input.held != held
            || (input.held > 0 && input.frame != frame)) {
            continue;
        }

        input.done = true;
        const auto window = s.window ? SDL_GetWindowID(s.window) : 0;

        if (input.action == ScriptedAction::Stroke || input.action == ScriptedAction::KeyDown
            || input.action == ScriptedAction::KeyUp) {
            if (input.action != ScriptedAction::KeyUp) {
                push_key(input, window, true);
            }

            if (input.action != ScriptedAction::KeyDown) {
                push_key(input, window, false);
            }
        } else {
            push_pointer(input, window);
        }
    }
}

// Override backend host-cursor polling only while explicit diagnostic input owns the pointer.
void refresh_scripted_pointer() {
    if (s.script_pointer) {
        ImGui::GetIO().AddMousePosEvent(s.script_pointer->x, s.script_pointer->y);
    }
}

void configure_scripted_input(std::string_view text) {
    if (text.starts_with('@')) {
        s.script_file = std::string(text.substr(1));
        return;
    }

    auto parsed = parse_actions(text);

    if (!parsed) {
        log::warn("OPENRAC_KEYS: {}", parsed.error().message);
        return;
    }

    s.script = std::move(parsed.value());
}
}  // namespace openrac::frontend::debug_overlay
