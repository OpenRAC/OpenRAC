// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// When the menu makes its sounds (menu_ui.h, menu_sound; the tones are audio/ui_sounds.cpp): a row
// pressed, the focus moved, the menu opened or shut, another tab or page.

#include <imgui.h>
// ImGuiContext::NavJustMovedToId: the focus moved by the arrows this frame.
#include <imgui_internal.h>

#include "debug/menu_ui.h"
#include "debug/overlay.h"

namespace openrac::frontend::debug_overlay {

namespace {

// One sound a frame: a row pressed says so, and the page it opens or the menu it shuts plays no
// second one over it.
bool g_played = false;

// What the menu showed the frame before (menu_sounds_frame).
struct Seen {
    bool open = false;
    int tab = 0;
    bool controller = false;
    bool quit_asked = false;
};

Seen g_seen{};

}  // namespace

void menu_sound(MenuSound sound) {
    if (g_played) {
        return;
    }
    audio::ui_sounds::play(sound);
    g_played = true;
}

void menu_sounds_frame() {
    const Seen now{s.menu_open, s.menu_tab, s.drawer_controller, s.quit_asked};
    if (now.open != g_seen.open) {
        menu_sound(now.open ? MenuSound::Open : MenuSound::Close);
    } else if (now.open && now.controller != g_seen.controller) {
        menu_sound(now.controller ? MenuSound::Page : MenuSound::Back);
    } else if (now.open && now.quit_asked != g_seen.quit_asked && !now.quit_asked) {
        menu_sound(MenuSound::Back);
    } else if (now.open && now.tab != g_seen.tab) {
        menu_sound(MenuSound::Page);
    } else if (now.open && ImGui::GetCurrentContext()->NavJustMovedToId != 0) {
        // The focus moved to another row: the arrows, the d-pad, a held one repeating.
        menu_sound(MenuSound::Cursor);
    }
    g_seen = now;
    g_played = false;
}

}  // namespace openrac::frontend::debug_overlay
