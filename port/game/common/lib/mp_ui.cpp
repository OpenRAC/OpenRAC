// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The C contract of the native multiplayer screens, openrac/mp_ui.h, over port/mpui: the context
// holds the description, the tables and the screen open, and a game's skin of C function pointers
// is wrapped as an mpui::Skin for the length of a call.

#include "openrac/mp_ui.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/c_boundary.h"
#include "common/log.h"
#include "mpui/description.h"
#include "mpui/model.h"
#include "mpui/screen_view.h"

// The context a binding holds: the C name of the contract, a C++ object behind it.
struct openrac_mp_ui {
    openrac::mpui::Description description;          // The screens shown.
    openrac::mpui::Model model;                      // The tables the binding fills.
    std::optional<openrac::mpui::ScreenView> view;   // The screen open, if any.
    std::optional<openrac::mpui::ScreenView> modal;  // The modal screen over it, if any.
    openrac::mpui::Rect modal_area;                  // Where modal screens are drawn.
    std::string table;                               // The table being filled.
    std::vector<openrac::mpui::Row> rows;            // Its rows so far.
    std::string target;                              // The last outcome's target, kept for C.
};

namespace openrac::mpui {
namespace {

// A game's C skin as the screens draw through it. It holds the C table, which outlives the call it
// was made for.
class CSkin final : public Skin {
public:
    // Wraps `skin`, the binding's for this call.
    explicit CSkin(const openrac_mp_ui_skin& skin) : m_skin(skin) {}

    float line_height() const override { return m_skin.line_height; }

    float text_width(std::string_view text, TextStyle style) const override {
        return m_skin.text_width(m_skin.user, text.data(), text.size(), static_cast<int>(style));
    }

    void text(float x, float y, std::string_view text, TextStyle style) override {
        m_skin.text(m_skin.user, x, y, text.data(), text.size(), static_cast<int>(style));
    }

    void highlight(const Rect& rect) override {
        m_skin.highlight(m_skin.user, rect.x, rect.y, rect.w, rect.h);
    }

    void panel(const Rect& rect) override {
        m_skin.panel(m_skin.user, rect.x, rect.y, rect.w, rect.h);
    }

    void scroll_marks(const Rect& table, bool up, bool down) override {
        m_skin.scroll_marks(
            m_skin.user, table.x, table.y, table.w, table.h, up ? 1 : 0, down ? 1 : 0
        );
    }

    void sound(Sound sound) override { m_skin.sound(m_skin.user, static_cast<int>(sound)); }

private:
    const openrac_mp_ui_skin& m_skin;  // The binding's skin; lives for the call that made this.
};

// The contract's events in the order of mpui::Event, which the header's enumeration follows.
Event event_of(int event) {
    // A number past the last event is none.
    if (event < OPENRAC_MP_UI_NONE || event > OPENRAC_MP_UI_SELECT) {
        return Event::NONE;
    }

    return static_cast<Event>(event);
}

}  // namespace
}  // namespace openrac::mpui

namespace {

// Opens the description's screen `id` as the modal one in `ui`'s modal area. A screen the
// description lacks opens nothing: the action was a server's mistake, logged.
void open_modal(openrac_mp_ui& ui, const std::string& id) {
    const openrac::mpui::ScreenSpec* spec = ui.description.screen(id);

    // The description names a screen it does not describe.
    if (spec == nullptr) {
        openrac::log::warn("multiplayer screens: open:{} names no screen", id);
        return;
    }

    ui.modal.emplace(*spec, ui.modal_area, openrac::mpui::Mode::MODAL);
}

}  // namespace

using openrac::c_boundary::call;
namespace mpui = openrac::mpui;

extern "C" openrac_mp_ui* openrac_mp_ui_create(void) {
    return call("mp_ui", static_cast<openrac_mp_ui*>(nullptr), [] {
        // Owned by the binding from here; openrac_mp_ui_destroy gives it back.
        return std::make_unique<openrac_mp_ui>().release();
    });
}

extern "C" void openrac_mp_ui_destroy(openrac_mp_ui* ui) {
    // Takes the context back from the binding, which frees it here.
    const std::unique_ptr<openrac_mp_ui> owned(ui);
}

extern "C" int openrac_mp_ui_describe(openrac_mp_ui* ui, const char* json, size_t size) {
    return call("mp_ui", 0, [&] {
        openrac::Result<mpui::Description> description =
            mpui::parse_description(std::string_view(json, size));

        // A refused description leaves the one in force, so the screens never go half drawn.
        if (!description) {
            openrac::log::warn("multiplayer screens: {}", description.error().message);
            return 0;
        }

        ui->description = std::move(description.value());
        ui->view.reset();
        openrac::log::info(
            "multiplayer screens: description {} revision {}, {} screens",
            ui->description.name,
            ui->description.revision,
            ui->description.screens.size()
        );
        return 1;
    });
}

extern "C" int openrac_mp_ui_max_players(openrac_mp_ui* ui, int fallback) {
    return ui->description.max_players > 0 ? ui->description.max_players : fallback;
}

extern "C" int openrac_mp_ui_open(
    openrac_mp_ui* ui, const char* id, float x, float y, float w, float h, int mode
) {
    return call("mp_ui", 0, [&] {
        const mpui::ScreenSpec* spec = ui->description.screen(id);

        // A screen the description leaves out stays the game's own.
        if (spec == nullptr) {
            ui->view.reset();
            return 0;
        }

        const mpui::Mode view_mode =
            mode == OPENRAC_MP_UI_MODAL ? mpui::Mode::MODAL : mpui::Mode::OVERLAY;
        ui->view.emplace(*spec, mpui::Rect{x, y, w, h}, view_mode);
        return 1;
    });
}

extern "C" void openrac_mp_ui_close(openrac_mp_ui* ui) {
    ui->view.reset();
}

extern "C" const char* openrac_mp_ui_current(openrac_mp_ui* ui) {
    return ui->view ? ui->view->spec().id.c_str() : "";
}

extern "C" void openrac_mp_ui_table_begin(openrac_mp_ui* ui, const char* name) {
    call("mp_ui", [&] {
        ui->table = name;
        ui->rows.clear();
    });
}

extern "C" void openrac_mp_ui_table_row(openrac_mp_ui* ui, uint32_t key, int enabled) {
    call("mp_ui", [&] { ui->rows.push_back(mpui::Row{key, enabled != 0, {}}); });
}

extern "C" void openrac_mp_ui_table_cell(openrac_mp_ui* ui, const char* field, const char* text) {
    call("mp_ui", [&] {
        // A cell before any row has no row to go in.
        if (!ui->rows.empty()) {
            ui->rows.back().cells.insert_or_assign(std::string(field), std::string(text));
        }
    });
}

extern "C" void openrac_mp_ui_table_end(openrac_mp_ui* ui) {
    call("mp_ui", [&] { ui->model.table(ui->table).set_rows(std::move(ui->rows)); });
}

extern "C" void openrac_mp_ui_event(
    openrac_mp_ui* ui, const openrac_mp_ui_skin* skin, int event, openrac_mp_ui_outcome* out
) {
    *out = openrac_mp_ui_outcome{0, OPENRAC_MP_UI_ACTION_NONE, "", 0, 0, 0};
    call("mp_ui", [&] {
        // A modal screen takes the event first; with none open, the screen over the game's.
        std::optional<mpui::ScreenView>& target = ui->modal ? ui->modal : ui->view;

        // With no screen open every event is the game's.
        if (!target) {
            return;
        }

        mpui::CSkin wrapped(*skin);
        const mpui::Outcome outcome = target->handle(mpui::event_of(event), ui->model, wrapped);
        mpui::Action action = outcome.action;

        // Back on a modal screen closes it; the game's screen under it never sees the press.
        if (ui->modal && action.kind == mpui::ActionKind::BACK) {
            ui->modal.reset();
            action = {};
        }

        // An open: action shows its screen modally, when the description has it.
        if (action.kind == mpui::ActionKind::OPEN) {
            open_modal(*ui, action.target);
            action = {};
        }

        ui->target = action.target;
        *out = openrac_mp_ui_outcome{
            outcome.consumed ? 1 : 0,
            static_cast<int>(action.kind),
            ui->target.c_str(),
            outcome.row_key,
            static_cast<int>(outcome.row),
            outcome.value
        };
    });
}

extern "C" void openrac_mp_ui_set_modal_area(
    openrac_mp_ui* ui, float x, float y, float w, float h
) {
    ui->modal_area = mpui::Rect{x, y, w, h};
}

extern "C" void openrac_mp_ui_notice(openrac_mp_ui* ui, const char* text) {
    call("mp_ui", [&] {
        ui->modal.emplace(mpui::notice_screen(text), ui->modal_area, mpui::Mode::MODAL);
    });
}

extern "C" int openrac_mp_ui_modal(openrac_mp_ui* ui) {
    return ui->modal ? 1 : 0;
}

extern "C" void openrac_mp_ui_set_active(openrac_mp_ui* ui, int active) {
    // Only an open screen has a cursor to show or hide.
    if (ui->view) {
        ui->view->set_active(active != 0);
    }
}

extern "C" void openrac_mp_ui_enter(openrac_mp_ui* ui, int from_top) {
    call("mp_ui", [&] {
        // Only an open screen takes the cursor.
        if (ui->view) {
            ui->view->enter(from_top != 0, ui->model);
        }
    });
}

extern "C" void openrac_mp_ui_draw(openrac_mp_ui* ui, const openrac_mp_ui_skin* skin) {
    call("mp_ui", [&] {
        mpui::CSkin wrapped(*skin);

        // The screen over the game's, then a modal one over both.
        if (ui->view) {
            skin->layer(skin->user, 0);
            ui->view->draw(wrapped, ui->model);
        }

        // The modal screen brings its own panel.
        if (ui->modal) {
            skin->layer(skin->user, 1);
            ui->modal->draw(wrapped, ui->model);
        }
    });
}

extern "C" int openrac_mp_ui_cursor(openrac_mp_ui* ui) {
    return ui->view ? static_cast<int>(ui->view->cursor()) : 0;
}

extern "C" int openrac_mp_ui_first_row(openrac_mp_ui* ui) {
    return ui->view ? static_cast<int>(ui->view->first_row()) : 0;
}
