// SPDX-License-Identifier: MIT
// Copyright (c) 2026 the OpenRAC contributors
//
// An example mod for interface 1.31 (sdk/README.md, docs/port/MODS.md): a panel with a button prompt,
// "<Square> Buy all", drawn over the Gadgetron vendor. It changes nothing in the game: a real mod
// would read the button on its tick and buy.
//
// How it works: on_overlay_phases asks for the frames of a level with a menu up
// (OPENRAC_OVERLAY_MENU), where plain on_overlay draws nothing. The game's tick (on_frame, the game's
// thread) notes whether that menu is the vendor; the overlay callback (the window's thread) draws
// the panel with rect and the line with text. OPENRAC_UI_SQUARE in the text is a button token: the
// port draws it as the player's device has Square (the game's icon on a PlayStation pad, X on an
// Xbox pad, the bound key in brackets on the keyboard and mouse). button_label gives the same label
// as text, for the log here.
#include <atomic>
#include <cstddef>
#include <string>

#include "openrac/mod.hpp"

namespace {

// The level's game mode of the Gadgetron vendor (openrac_moby_run.level_mode: Ratchet & Clank's; the
// game state of Going Commando has the same number).
constexpr int kVendorMode = 5;

// Whether the vendor was up at the last tick. Written on the game's thread, read on the window's.
std::atomic<bool> g_vendor{false};

// The panel and its line, in the game's 512 x 448 picture: between the vendor's Buy and Exit
// monitors, above its strip of items.
constexpr float kPanelX = 176.0f;
constexpr float kPanelY = 268.0f;
constexpr float kPanelW = 160.0f;
constexpr float kPanelH = 28.0f;
constexpr float kTextHeight = 16.0f;
constexpr const char* kPrompt = OPENRAC_UI_SQUARE " Buy all";

// Once a frame of a level with a menu up: the panel, over the game's own 2D.
void draw(void*, const openrac_overlay* overlay) {
    if (overlay->size < offsetof(openrac_overlay, phase) + sizeof overlay->phase
        || overlay->phase != OPENRAC_OVERLAY_MENU || !g_vendor.load()) {
        return;
    }
    overlay->rect(overlay->context, kPanelX, kPanelY, kPanelW, kPanelH, 0x000000B0u);
    overlay->rect(overlay->context, kPanelX, kPanelY, kPanelW, 2.0f, 0x40C0FFFFu);
    const float width = overlay->text_width(overlay->context, kTextHeight, kPrompt);
    overlay->text(
        overlay->context,
        kPanelX + (kPanelW - width) / 2.0f,
        kPanelY + (kPanelH - kTextHeight) / 2.0f,
        kTextHeight,
        0xFFFFFFFFu,
        kPrompt
    );
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1" && mod.game() != "rac2") {
        throw openrac::Error("this example is for Ratchet & Clank and Going Commando");
    }
    if (!mod.has_overlay_phases() || !mod.has_button_label()) {
        throw openrac::Error("needs OpenRAC with mod interface 1.31");
    }
    // Each tick: is the menu up the vendor? A game whose port does not say (moby_run) shows the
    // panel over every menu.
    mod.on_frame([&mod] {
        openrac_moby_run run{};
        run.size = sizeof run;
        const bool vendor = mod.moby_run(-1, run) ? run.level_mode == kVendorMode : true;
        if (vendor && !g_vendor.load()) {
            int kind = 0;
            const std::string label = mod.button_label(OPENRAC_PAD_SQUARE, &kind);
            mod.info(
                std::string("vendor open; Square is ")
                + (kind == OPENRAC_BUTTONS_KEYS      ? "the key " + label
                   : kind == OPENRAC_BUTTONS_LETTERS ? "the button " + label
                                                     : std::string("the game's icon"))
            );
        }
        g_vendor.store(vendor);
    });
    if (!mod.on_overlay_phases(draw, nullptr, OPENRAC_OVERLAY_MENU)) {
        throw openrac::Error("the port refused the overlay");
    }
}

}  // namespace

OPENRAC_MOD_CPP("vendor-prompt", "1.0.0", load)
