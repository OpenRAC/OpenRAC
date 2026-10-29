// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "mod_ui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "common/log.h"
#include "exit_menu.h"
#include "menu_fit.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"
#include "options_menu.h"

namespace openrac::frontend::mod_ui {

namespace {

/*
 * Ratchet & Clank's page menu (rac1-decomp pause.c and include/menu.h; ~/RACSwarm/plans/rac1-mp/MENU.md
 * has the research). A page is {seq[14], back, state, focus, screen[14], ...}: screen i is drawn
 * in the frame of frame moby i, which plays the page's seq i. The main menu's three items are
 * one-item text lists in slots 2, 3 and 4 at seqs 16, 17 and 18, the rows of the pause menu's
 * root at 49-pixel steps; 19 and 20 are the two rows below, and slots 5 and 7 are free (slot 6
 * draws only with the Goodies). The page is data: its rows are put in the new order, each widget
 * in its row's slot, the cursor's up and down links redone, and a mod's item is a copy of the
 * Options item that opens its page (action 3).
 */

// MenuSystem: its state (1 while a page switch runs), the page shown, the page asked for, the
// switch's timer (12 at its start), the page it left, the memory card prompt's kind.
constexpr gaddr kState = 0x00;
constexpr gaddr kCurrent = 0x04;
constexpr gaddr kNext = 0x08;
constexpr gaddr kTimer = 0x14;
constexpr gaddr kPrevious = 0xD0;
constexpr gaddr kPromptKind = 0xD4;
constexpr std::int32_t kSwitching = 1;
constexpr std::int32_t kSwitchTicks = 12;

// MenuPage: the frame mobys' seqs, back, focus, screens, pending focus; its size.
constexpr gaddr kSeqs = 0x00;
constexpr gaddr kBack = 0x38;
constexpr gaddr kFocus = 0x40;
constexpr gaddr kScreens = 0x44;
constexpr gaddr kPending = 0x80;
constexpr gaddr kPageBytes = 0x88;

// MenuScreen: update, enter, leave, its moby (for the sounds); a text list's flags, items, up and
// down, cursor and scroll; a label's text. What of each kind is copied.
constexpr gaddr kUpdate = 0x00;
constexpr gaddr kEnter = 0x08;
constexpr gaddr kLeave = 0x0C;
constexpr gaddr kMoby = 0x14;
constexpr gaddr kListFlags = 0x30;
constexpr gaddr kListItems = 0x34;
constexpr gaddr kListUp = 0x38;
constexpr gaddr kListDown = 0x3C;
constexpr gaddr kListCursor = 0x40;
constexpr gaddr kListScroll = 0x44;
constexpr gaddr kLabelText = 0x34;
constexpr gaddr kWidgetBytes = 0x58;
constexpr gaddr kListBytes = 0x48;

// A text list's item: label, action, argument, sublabel, timer. The actions: none (drawn dim),
// open the page in the argument, and one the game ignores (drawn as any; the port acts on it).
constexpr gaddr kItemBytes = 12;
constexpr std::int16_t kActionNone = 0;
constexpr std::int16_t kActionPage = 3;
constexpr std::int16_t kActionIgnored = 0x7F;

// A list's flags: rows as high as the font (0x10), centred (0x400); a footer's: drops to the small
// font when a line is too wide for its frame (0x20000), which the game then keeps (8).
constexpr std::uint32_t kListRows = 0x10u | 0x400u;
constexpr std::uint32_t kFooterShrinks = 0x20000u;
constexpr std::uint32_t kSmallFont = 0x8u;

// The page's slots: filler, filler, title, list, footer.
constexpr int kTitleSlot = 2;
constexpr int kListSlot = 3;
constexpr int kFooterSlot = 4;

// The main menu's rows: the slot each is drawn in and its seq. The game's own are the first three,
// then the mods' and the port's Exit (exit_menu.h), last.
constexpr std::array<int, 5> kRowSlots = {2, 3, 4, 5, 7};
constexpr std::array<std::int32_t, 5> kRowSeqs = {16, 17, 18, 19, 20};
constexpr int kGameRows = 3;
static_assert(kGameRows + OPENRAC_UI_MAIN_ITEMS_MAX <= static_cast<int>(kRowSlots.size()));
// The game's items' names, in their rows' order (openrac_api.ui_main_item's `after`).
constexpr std::array<const char*, kGameRows> kGameItems =
    {OPENRAC_UI_NEW_GAME, OPENRAC_UI_LOAD_GAME, OPENRAC_UI_OPTIONS};

// The game's pages the mods' are made from: a first level (each main-menu item's own, from the
// Options page) and one second level (from the Language page) that every page under them shows.
constexpr int kFirstLevels = OPENRAC_UI_MAIN_ITEMS_MAX;
constexpr int kSecondLevel = kFirstLevels;
constexpr int kGamePages = kFirstLevels + 1;

// The texts' ids, past the game's (under 0x7000) and below the options page's (0x7E00): a game
// page's title, items and footer lines, then the main-menu labels and the question's answers.
constexpr int kTextFirst = 0x7C00;
constexpr int kPageTexts = 1 + OPENRAC_UI_ITEMS_MAX + OPENRAC_UI_FOOTER_MAX;
constexpr int kMainTexts = kTextFirst + kGamePages * kPageTexts;
constexpr int kYesText = kMainTexts + OPENRAC_UI_MAIN_ITEMS_MAX;
constexpr int kNoText = kYesText + 1;
constexpr int kTexts = kNoText + 1 - kTextFirst;
constexpr gaddr kTextBytes = 64;
// The memory card prompt's text (draw_prompt_box, prompt kinds 0 to 2), the question in its place.
constexpr int kPromptText = 0x4FB3;

// Pad 0's buttons (active low, the same bits as OPENRAC_PAD_*) and the cursor's repeat while
// a direction is held: the first after so many ticks, then one every so many.
constexpr std::uint16_t kUp = 0x0010;
constexpr std::uint16_t kRight = 0x0020;
constexpr std::uint16_t kDown = 0x0040;
constexpr std::uint16_t kLeft = 0x0080;
constexpr std::uint16_t kTriangle = 0x1000;
constexpr std::uint16_t kCross = 0x4000;
constexpr std::array<std::uint16_t, 4> kDirections = {kUp, kRight, kDown, kLeft};
constexpr int kRepeatAfter = 15;
constexpr int kRepeatEvery = 4;

// One of the game's pages the mods' are shown on, made once.
struct GamePage {
    gaddr page = 0, title = 0, list = 0, footer = 0, items = 0, footer_items = 0;
    gaddr game_footer_items = 0;  // the game's hints (Select, Exit), shown with no footer
    std::uint32_t footer_flags = 0;
    std::vector<std::string> footer_shown;  // the footer's lines last put
};

/*
 * The frames' width. A widget's box is its frame moby's projected frame: the corners its update
 * (settle_attachment_weight_and_update) puts in its pvars from its pose, its scale and its rows
 * (+0xC0, the images of the model's axes), which render_level_effects_and_screen_sprites projects
 * (pause.c); the moby is drawn with the same rows. The menu's camera sits at the frames' own
 * place, (256, 256, 64), and looks along x, the game's forward (reset_active_camera_position_and_
 * basis): the frames stand 6 in front of it, and the screen's x is the world's y, its y the
 * world's z. A mod's page stretches its title's, list's and footer's frames by one factor to fit
 * its texts (menu_fit.h): each row's y times it and the moby moved along y so that the frame keeps
 * its centre, with mode 0x100, the moby keeping rows of its own (the moby loop's func_0020ED48
 * makes them from its angles otherwise). The frames' own widths, as the seqs of the Options and
 * Language pages pose them, are known (kBases, measured), so a page is as wide as it needs from
 * its first showing; a frame of another seq is learnt as a page shows it settled at stretch 1. A
 * switch to or from a mod's page goes from one stretch to the other over its 12 ticks, as the
 * frames fly; a page shown takes its own, and while it stays only grows with its texts. The game's
 * own pages are left as the game made them.
 */
constexpr gaddr kMobyPosition = 0x10;
constexpr gaddr kMobyMode = 0x34;
constexpr gaddr kMobySeq = 0x52;
constexpr gaddr kMobyFrame = 0x78;  // pvars: the frame's corners, then its box on the screen
constexpr gaddr kMobyRows = 0xC0;
constexpr gaddr kAcross = 4;  // a vector's y: the screen's x
constexpr gaddr kUpward = 8;  // a vector's z: the screen's y
constexpr std::uint16_t kOwnRows = 0x100;
constexpr std::uint16_t kRowFlipped = 0x8000;  // row 1 negated at each update: left alone
constexpr gaddr kCornerFirst = 0x00;
constexpr gaddr kCornerOpposite = 0x30;
constexpr gaddr kFrameX = 0x50;
constexpr gaddr kFrameWidth = 0x58;
constexpr std::array<int, 3> kFitSlots = {kTitleSlot, kListSlot, kFooterSlot};
// The fonts in the front end's table; the screen's width and its safe area, 5% in on each side.
constexpr int kNormalFont = 0;
constexpr int kLargeFont = 2;
constexpr gaddr kGlyphBytes = 256 * 4;
constexpr float kScreenWidth = 512.0f;
constexpr float kSafeLeft = kScreenWidth * 0.05f;
constexpr float kSafeRight = kScreenWidth * 0.95f;
// A page shown takes a new stretch this part of the way a tick.
constexpr float kEase = 0.3f;

// A frame moby stretched: its own y and its rows' own y, the stretch it has; and its frame's width
// as last measured at stretch 1, by seq.
struct Stretched {
    gaddr moby = 0;  // 0: not stretched
    float position_y = 0.0f;
    std::array<float, 3> row_y{};
    float by = 1.0f;
    int seen_seq = -1;
    float seen_width = 0.0f, seen_x = 0.0f;
};

// A frame's own width and its centre on the screen, as its seq leaves it (0: not seen yet).
struct Base {
    float width = 0.0f, centre = 0.0f;
};

// The frames of the pages a mod's are made from, settled, as measured in PAL and NTSC-U (the same
// in both): the Options page's title, list and footer (seqs 190 to 192), the Language page's (193
// to 195). Each centred on the screen.
struct KnownBase {
    int seq;
    float width;
};

constexpr float kBaseCentre = 256.5f;
constexpr std::array<KnownBase, 6> kBases = {{
    {190, 127.0f},
    {191, 127.0f},
    {192, 127.0f},
    {193, 163.0f},
    {194, 151.0f},
    {195, 127.0f},
}};

// What a mod page's texts need of the title, the list and the footer, at a revision of it.
struct Needs {
    std::uint64_t revision = 0;
    bool known = false;
    std::array<float, kFitSlots.size()> width{};
};

struct Fit {
    std::array<Stretched, kFitSlots.size()> slots{};
    std::array<Base, 256> bases{};
    std::vector<Needs> needs;  // by mod page
    float by = 1.0f;           // the stretch the frames have
    float from = 1.0f, to = 1.0f, goal = 1.0f;
    float logged = 0.0f;  // the stretch last logged
    bool switching = false;
    int game = -1, page = -1;  // the page shown whose stretch it is
};

// The main menu's rows raised as a block, so that the rows under the game's leave it centred where
// the game's three were: each row's frame moby moved up along z (the screen's y) by `by` of
// `height`, half the extra rows' height (the game's rows' step, learnt from their frames once they
// are settled). A moby's own z is taken again whenever the game moved it since (a switch's flight),
// so the rows only ever sit `height` above where the game puts them.
struct Raise {
    std::array<gaddr, kRowSlots.size()> moby{};  // 0: not moved
    std::array<float, kRowSlots.size()> own_z{}, put_z{};
    float height = 0.0f;  // 0: none seen yet
    float step = 0.0f;    // from one row to the next, along with the height
    bool learnt = false;  // the height of the settled rows
    float by = 0.0f;
};

struct State {
    bool built = false;
    std::array<GamePage, kGamePages> pages{};
    std::array<gaddr, OPENRAC_UI_MAIN_ITEMS_MAX> main_widgets{}, main_items{};
    gaddr prompt = 0;  // the prompt widget, a question's drawing
    gaddr texts = 0;   // kTexts strings of kTextBytes
    gaddr question = 0;
    bool patched = false;                         // the main menu has the mods' rows
    std::array<gaddr, kGameRows> game_widgets{};  // the game's items' widgets, by row
    gaddr exit_widget = 0, exit_item = 0;         // the port's Exit row
    int extra_rows = 0;                           // the rows under the game's: the mods' and Exit
    int second = -1;                              // the mod page the second level shows
    int game = -1;             // the game page on the screen (pages' index), or -1
    int shown = -1;            // the mod page on the screen, or -1
    bool asking = false;       // a question is open
    std::uint32_t tag = 0;     // its tag
    std::int32_t kind = 0;     // the prompt kind it replaced
    gaddr follow_page = 0;     // whose seqs the rows below fly a switch with (follow_switch)
    bool follow_out = false;   // that switch goes away from the main menu
    bool follow_seen = false;  // it began
    std::array<std::int32_t, OPENRAC_UI_MAIN_ITEMS_MAX> follow_seqs{};  // the page's own
    std::uint16_t previous = 0xFFFF;  // pad 0 as last seen (active low)
    std::uint16_t held = 0, pressed = 0;
    std::uint16_t swallow = 0;  // held as a page left: kept from the game until let go
    std::array<int, kDirections.size()> held_for{};
    Fit fit;
    Raise raise;
};

State s;

// The game's front end, when its program describes what the pages are made of.
const openrac_front_end* front_end() {
    const openrac_front_end* fe = openrac_game_front_end;
    return fe != nullptr && fe->main_page != 0 && fe->options_page != 0 && fe->language_page != 0
                   && fe->language_title != 0 && fe->language_list != 0 && fe->prompt != 0
                   && fe->page_menu != 0 && fe->menu_cue != nullptr
                   && fe->panel_animation != nullptr
               ? fe
               : nullptr;
}

std::uint32_t word(gaddr at) {
    return GREF(std::uint32_t, at);
}

void put_word(gaddr at, std::uint32_t value) {
    GREF(std::uint32_t, at) = value;
}

gaddr text_at(int id) {
    return s.texts + kTextBytes * static_cast<gaddr>(id - kTextFirst);
}

// A text as the game shows it: UTF-8 made the game's letters (options_menu::game_text), the pad
// glyphs kept, any other control character a space, as long as a text's room.
std::string game_line(const std::string& utf8) {
    std::string text = options_menu::game_text(utf8);
    std::replace_if(
        text.begin(),
        text.end(),
        [](char c) {
            const auto u = static_cast<unsigned char>(c);
            return u < 0x20 && (u < 0x10 || u > 0x19);
        },
        ' '
    );
    text.resize(std::min<std::size_t>(text.size(), kTextBytes - 1));
    return text;
}

void put_text(int id, const std::string& utf8) {
    std::snprintf(static_cast<char*>(G(text_at(id))), kTextBytes, "%s", game_line(utf8).c_str());
}

void put_item(gaddr at, int text, std::int16_t action, std::uint32_t arg) {
    GREF(std::int16_t, at) = static_cast<std::int16_t>(text);
    GREF(std::int16_t, at + 2) = action;
    GREF(std::uint32_t, at + 4) = arg;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

int page_text(int game, int index) {
    return kTextFirst + game * kPageTexts + index;
}

// A copy of `from`'s `bytes` in a new widget.
gaddr copy_widget(gaddr from, gaddr bytes) {
    const gaddr to = openrac_guest_static(kWidgetBytes);
    std::memset(G(to), 0, kWidgetBytes);
    std::memcpy(G(to), G(from), bytes);
    return to;
}

// One of the pages the mods' are shown on: a copy of `page` (the Options page or the Language
// page), its title the Language page's label, its list the Language page's text list with no
// update (the port moves its cursor), its footer the page's own hints with items of the mod's.
void make_page(GamePage& g, int game, const openrac_front_end& fe, gaddr page) {
    g.page = openrac_guest_static(kPageBytes);
    std::memcpy(G(g.page), G(page), kPageBytes);
    g.title = copy_widget(fe.language_title, kWidgetBytes);
    g.list = copy_widget(fe.language_list, kListBytes);
    g.footer = copy_widget(word(page + kScreens + 4 * kFooterSlot), kListBytes);
    g.items = openrac_guest_static(kItemBytes * (OPENRAC_UI_ITEMS_MAX + 1));
    g.footer_items = openrac_guest_static(kItemBytes * (OPENRAC_UI_FOOTER_MAX + 1));
    std::memset(G(g.items), 0, kItemBytes * (OPENRAC_UI_ITEMS_MAX + 1));
    std::memset(G(g.footer_items), 0, kItemBytes * (OPENRAC_UI_FOOTER_MAX + 1));
    g.game_footer_items = word(g.footer + kListItems);
    g.footer_flags = word(g.footer + kListFlags) | kFooterShrinks;
    put_word(g.title + kLabelText, static_cast<std::uint32_t>(page_text(game, 0)));
    put_word(g.list + kUpdate, 0);
    put_word(g.list + kEnter, 0);
    put_word(g.list + kLeave, 0);
    put_word(g.list + kListFlags, kListRows);
    put_word(g.list + kListItems, g.items);
    put_word(g.list + kListUp, 0);
    put_word(g.list + kListDown, 0);
    put_word(g.list + kListCursor, 0);
    put_word(g.list + kListScroll, 0);
    put_word(g.footer + kListFlags, g.footer_flags);
    put_word(g.page + kScreens + 4 * kTitleSlot, g.title);
    put_word(g.page + kScreens + 4 * kListSlot, g.list);
    put_word(g.page + kScreens + 4 * kFooterSlot, g.footer);
    put_word(g.page + kFocus, g.list);
    put_word(g.page + kPending, 0);
}

// Everything in game memory, once (statics made later would stop the mods' warm snapshots).
void build(const openrac_front_end& fe) {
    if (s.built) {
        return;
    }

    s.built = true;

    for (const KnownBase& b : kBases) {
        s.fit.bases[static_cast<std::size_t>(b.seq)] = {b.width, kBaseCentre};
    }

    s.texts = openrac_guest_static(kTextBytes * kTexts);
    s.question = openrac_guest_static(kTextBytes * 4);
    std::memset(G(s.texts), 0, kTextBytes * kTexts);
    std::memset(G(s.question), 0, kTextBytes * 4);

    for (int i = 0; i < kFirstLevels; ++i) {
        make_page(s.pages[static_cast<std::size_t>(i)], i, fe, fe.options_page);
        put_word(s.pages[static_cast<std::size_t>(i)].page + kBack, fe.main_page);
    }

    make_page(s.pages[kSecondLevel], kSecondLevel, fe, fe.language_page);
    // The prompt: drawn only (its update would go back outside the memory card's flow).
    s.prompt = copy_widget(fe.prompt, kWidgetBytes);
    put_word(s.prompt + kUpdate, 0);
    put_word(s.prompt + kEnter, 0);
    put_word(s.prompt + kLeave, 0);
    put_text(kYesText, OPENRAC_UI_CROSS " Yes");
    put_text(kNoText, OPENRAC_UI_TRIANGLE " No");
}

// The main menu as the game made it: three one-item lists in slots 2 to 4 at seqs 16 to 18, the
// slots of the rows below empty and parked.
bool main_menu_is_the_games(gaddr page) {
    for (int r = 0; r < kGameRows; ++r) {
        const auto slot = static_cast<gaddr>(kRowSlots[static_cast<std::size_t>(r)]);

        if (GREF(std::int32_t, page + kSeqs + 4 * slot) != kRowSeqs[static_cast<std::size_t>(r)]
            || word(page + kScreens + 4 * slot) == 0) {
            return false;
        }
    }

    for (std::size_t r = kGameRows; r < kRowSlots.size(); ++r) {
        const auto slot = static_cast<gaddr>(kRowSlots[r]);

        if (word(page + kScreens + 4 * slot) != 0) {
            return false;
        }
    }

    return true;
}

// Makes the port's Exit row: a copy of the main menu's Options widget `options` whose one item
// opens the question page (exit_menu.h), as a mod's item opens its page.
gaddr make_exit_row(const openrac_front_end& fe, gaddr options) {
    s.exit_item = openrac_guest_static(kItemBytes * 2);
    std::memset(G(s.exit_item), 0, kItemBytes * 2);
    put_item(
        s.exit_item, exit_menu::text_id(exit_menu::kExit), kActionPage, exit_menu::title_page(fe)
    );
    s.exit_widget = copy_widget(options, kListBytes);
    put_word(s.exit_widget + kListItems, s.exit_item);
    return s.exit_widget;
}

// The main menu with the mods' rows: each after the game's item it names (in the order added),
// then the port's Exit (a copy of the Options item opening its question, exit_menu.h) where a slot
// is left (with two mods' rows there is none), every widget in its row's slot, the cursor going
// round them in order.
void patch_main(const openrac_front_end& fe) {
    if (s.patched) {
        return;
    }

    s.patched = true;
    const gaddr page = fe.main_page;

    if (!main_menu_is_the_games(page)) {
        log::warn("mod ui: the main menu is not the game's three items; its items are left out");
        return;
    }

    std::vector<gaddr> rows;

    for (int r = 0; r < kGameRows; ++r) {
        s.game_widgets[static_cast<std::size_t>(r)] =
            word(page + kScreens + 4 * static_cast<gaddr>(kRowSlots[static_cast<std::size_t>(r)]));
        rows.push_back(s.game_widgets[static_cast<std::size_t>(r)]);
    }

    const gaddr options = s.game_widgets[kGameRows - 1];
    const int count = std::min(mods::ui_main_item_count(), OPENRAC_UI_MAIN_ITEMS_MAX);

    for (int i = 0; i < count; ++i) {
        const mods::UiMainItem& item = *mods::ui_main_item(i);
        const auto index = static_cast<std::size_t>(i);
        s.main_items[index] = openrac_guest_static(kItemBytes * 2);
        std::memset(G(s.main_items[index]), 0, kItemBytes * 2);
        put_text(kMainTexts + i, item.label);
        put_item(s.main_items[index], kMainTexts + i, kActionPage, s.pages[index].page);
        s.main_widgets[index] = copy_widget(options, kListBytes);
        put_word(s.main_widgets[index] + kListItems, s.main_items[index]);
        // After the game's item it names and the mods' already after it.
        const auto game = std::find_if(kGameItems.begin(), kGameItems.end(), [&](const char* n) {
            return item.after == n;
        });
        const gaddr after = s.game_widgets[static_cast<std::size_t>(game - kGameItems.begin())];
        auto at = std::find(rows.begin(), rows.end(), after) + 1;

        while (at != rows.end()
               && std::find(s.main_widgets.begin(), s.main_widgets.end(), *at)
                      != s.main_widgets.end()) {
            ++at;
        }

        rows.insert(at, s.main_widgets[index]);
    }

    // Exit last, in a free row's slot: two mods' rows take both.
    if (rows.size() < kRowSlots.size()) {
        rows.push_back(make_exit_row(fe, options));
    } else {
        log::warn("mod ui: the main menu's rows are the mods'; Exit is left out");
    }

    s.extra_rows = static_cast<int>(rows.size()) - kGameRows;
    const auto n = rows.size();

    for (std::size_t r = 0; r < n; ++r) {
        const auto slot = static_cast<gaddr>(kRowSlots[r]);
        GREF(std::int32_t, page + kSeqs + 4 * slot) = kRowSeqs[r];
        put_word(page + kScreens + 4 * slot, rows[r]);
        put_word(rows[r] + kListUp, rows[(r + n - 1) % n]);
        put_word(rows[r] + kListDown, rows[(r + 1) % n]);
    }

    log::info(
        "mod ui: {} item(s) in the main menu{}", count, s.exit_widget != 0 ? ", and Exit" : ""
    );
}

// The seq page `page` gives slot `slot` in a switch.
std::int32_t& page_seq(gaddr page, int slot) {
    return GREF(std::int32_t, page + kSeqs + 4 * static_cast<gaddr>(slot));
}

// A page switch to or from the main menu: the rows below the game's fly as Options does. The game
// gives every slot its seq when the switch begins (update_menu_system), from the page coming (its
// seqs from the first frame) or, going back, from the page leaving (its own, played backwards from
// the last): the main menu's own rows morph into that page's frames and back, and the rows below
// would play the page's seqs for their slots, which park them off the screen to the left. So once
// the game asked for such a switch, before it begins, that page's seqs for those slots are Options'
// for the switch, and the game plays them in step with Options' own; raise_rows keeps each row its
// steps below Options as the flight goes, less (away) or more (back) at each tick. Once the switch
// is over the page has its seqs back and the rows their own: parked on the page (as the page's
// seqs leave them), or the row on the main menu (its seq's first frame, as the game's open leaves
// it).
void follow_switch(const openrac_front_end& fe) {
    const gaddr menu = fe.page_menu;
    const bool switching = GREF(std::int32_t, menu + kState) == kSwitching;
    const gaddr current = word(menu + kCurrent);
    const gaddr next = word(menu + kNext);
    const int options = kRowSlots[static_cast<std::size_t>(kGameRows - 1)];

    if (s.follow_page == 0 && !switching && next != 0 && s.extra_rows > 0) {
        const bool out = current == fe.main_page && next != fe.main_page;
        const bool back = next == fe.main_page && current != 0 && current != fe.main_page
                          && word(current + kBack) == fe.main_page;
        const gaddr page = out ? next : back ? current : 0;

        for (int r = 0; page != 0 && r < s.extra_rows; ++r) {
            std::int32_t& seq = page_seq(page, kRowSlots[static_cast<std::size_t>(kGameRows + r)]);
            s.follow_seqs[static_cast<std::size_t>(r)] = seq;
            seq = page_seq(page, options);
        }

        s.follow_page = page;
        s.follow_out = out;
        s.follow_seen = false;
    }

    s.follow_seen = s.follow_seen || (s.follow_page != 0 && switching);

    // The switch over (or not made): the page's seqs back, and the rows their own.
    if (s.follow_page != 0 && !switching && next == 0) {
        for (int r = 0; r < s.extra_rows; ++r) {
            const int slot = kRowSlots[static_cast<std::size_t>(kGameRows + r)];
            const std::int32_t own = s.follow_seqs[static_cast<std::size_t>(r)];
            page_seq(s.follow_page, slot) = own;

            if (s.follow_seen && s.follow_out && current == s.follow_page) {
                fe.panel_animation(slot, own, 1, 1.0f);
            } else if (s.follow_seen && !s.follow_out && current == fe.main_page) {
                fe.panel_animation(
                    slot, kRowSeqs[static_cast<std::size_t>(kGameRows + r)], 0, -1.0f
                );
            }
        }

        s.follow_page = 0;
    }
}

// The menus gone: a page whose seqs a switch borrowed has them back.
void unfollow() {
    for (int r = 0; s.follow_page != 0 && r < s.extra_rows; ++r) {
        page_seq(s.follow_page, kRowSlots[static_cast<std::size_t>(kGameRows + r)]) =
            s.follow_seqs[static_cast<std::size_t>(r)];
    }

    s.follow_page = 0;
}

// The game page `page` is, or -1.
int game_page_of(gaddr page) {
    for (int g = 0; page != 0 && g < kGamePages; ++g) {
        if (s.pages[static_cast<std::size_t>(g)].page == page) {
            return g;
        }
    }

    return -1;
}

// The mod page a game page shows: a first level its main-menu item's, the second the one opened.
int mod_page_of(int game) {
    if (game < 0) {
        return -1;
    }

    if (game == kSecondLevel) {
        return s.second;
    }

    const mods::UiMainItem* item = mods::ui_main_item(game);
    return item != nullptr ? item->page : -1;
}

// An item's line: a choice's label and value, with Left and Right's marks under the cursor.
std::string item_line(const mods::UiItem& item, bool under_cursor) {
    if (item.kind != OPENRAC_UI_CHOICE || item.choices.empty()) {
        return item.label;
    }

    const std::string& value =
        item.choices[std::min<std::size_t>(item.choice, item.choices.size() - 1)];
    const std::string shown = under_cursor ? "< " + value + " >" : value;
    return item.label.empty() ? shown : item.label + "  " + shown;
}

// Game page `game` shows mod page `page`: its title, items and cursor, and its footer (the yes and
// no of a question, the mod's lines, or the game's hints).
void show(int game, int page) {
    GamePage& g = s.pages[static_cast<std::size_t>(game)];
    const mods::UiPage* p = mods::ui_page(page);
    static const mods::UiPage kEmpty;
    const mods::UiPage& content = p != nullptr ? *p : kEmpty;
    put_text(page_text(game, 0), content.title);
    const int count = std::min<int>(static_cast<int>(content.items.size()), OPENRAC_UI_ITEMS_MAX);

    for (int i = 0; i < count; ++i) {
        const mods::UiItem& item = content.items[static_cast<std::size_t>(i)];
        const int id = page_text(game, 1 + i);
        put_text(id, item_line(item, i == content.cursor));
        // Disabled ones dim, as the game's locked entries; lines of text as the rest.
        const std::int16_t action =
            (item.flags & OPENRAC_UI_DISABLED) != 0 ? kActionNone : kActionIgnored;
        put_item(g.items + kItemBytes * static_cast<gaddr>(i), id, action, 0);
    }

    std::memset(G(g.items + kItemBytes * static_cast<gaddr>(count)), 0, kItemBytes);
    put_word(
        g.list + kListCursor,
        static_cast<std::uint32_t>(std::clamp(content.cursor, 0, std::max(count - 1, 0)))
    );
    // The footer: the answers while a question is open, else the mod's lines, else the game's
    // hints. A footer that changed starts in its large font again.
    const bool answers = s.asking && game == s.game;
    std::vector<std::string> key =
        answers ? std::vector<std::string>{std::string(1, '\x01')} : content.footer;

    if (key != g.footer_shown) {
        g.footer_shown = key;
        put_word(g.footer + kListFlags, g.footer_flags & ~kSmallFont);
    }

    std::vector<int> ids;

    if (answers) {
        ids = {kYesText, kNoText};
    }

    for (std::size_t i = 0; !answers && i < content.footer.size() && i < OPENRAC_UI_FOOTER_MAX;
         ++i) {
        ids.push_back(page_text(game, 1 + OPENRAC_UI_ITEMS_MAX + static_cast<int>(i)));
        put_text(ids.back(), content.footer[i]);
    }

    for (std::size_t i = 0; i < ids.size(); ++i) {
        put_item(g.footer_items + kItemBytes * static_cast<gaddr>(i), ids[i], kActionNone, 0);
    }

    std::memset(G(g.footer_items + kItemBytes * static_cast<gaddr>(ids.size())), 0, kItemBytes);
    put_word(g.footer + kListItems, ids.empty() ? g.game_footer_items : g.footer_items);
}

gaddr panel(const openrac_front_end& fe, int slot) {
    return word(fe.panel_slots + 4 * static_cast<gaddr>(slot));
}

std::span<const std::uint8_t> glyphs(const openrac_front_end& fe, int font) {
    return {static_cast<const std::uint8_t*>(G(fe.fonts[font])), kGlyphBytes};
}

// The widths mod page `page`'s texts need: its title in the large font (the label's), its items in
// the list's (a choice with each of its values under the cursor, the widest of them) and its
// footer's lines in the footer's normal font.
const std::array<float, kFitSlots.size()>& needs(const openrac_front_end& fe, int page) {
    static const std::array<float, kFitSlots.size()> kNone{};
    const mods::UiPage* p = mods::ui_page(page);

    if (p == nullptr || page < 0 || fe.fonts[kNormalFont] == 0 || fe.fonts[kLargeFont] == 0) {
        return kNone;
    }

    auto& all = s.fit.needs;

    if (all.size() <= static_cast<std::size_t>(page)) {
        all.resize(static_cast<std::size_t>(page) + 1);
    }

    Needs& n = all[static_cast<std::size_t>(page)];

    if (n.known && n.revision == p->revision) {
        return n.width;
    }

    const auto normal = glyphs(fe, kNormalFont);
    const auto width = [&](const std::string& utf8, std::span<const std::uint8_t> font) {
        return menu_fit::text_width(game_line(utf8), font);
    };
    const std::array<int, 1> title = {width(p->title, glyphs(fe, kLargeFont))};
    std::vector<int> items;
    const auto count = std::min<std::size_t>(p->items.size(), OPENRAC_UI_ITEMS_MAX);

    for (std::size_t i = 0; i < count; ++i) {
        mods::UiItem item = p->items[i];

        if (item.kind != OPENRAC_UI_CHOICE || item.choices.empty()) {
            items.push_back(width(item_line(item, false), normal));
            continue;
        }

        for (std::size_t c = 0; c < item.choices.size(); ++c) {
            item.choice = static_cast<std::uint32_t>(c);
            items.push_back(width(item_line(item, true), normal));
        }
    }

    std::vector<int> footer;

    for (std::size_t i = 0; i < p->footer.size() && i < OPENRAC_UI_FOOTER_MAX; ++i) {
        footer.push_back(width(p->footer[i], normal));
    }

    n.known = true;
    n.revision = p->revision;
    n.width = {
        menu_fit::needed_width(title, menu_fit::kTitlePadding),
        menu_fit::needed_width(items, menu_fit::kListPadding),
        menu_fit::needed_width(footer, menu_fit::kFooterPadding),
    };
    return n.width;
}

// The stretch game page `game` takes for the mod page it shows (1 for one of the game's): its
// texts' needs over its frames' own widths, as far as the safe area lets them.
float target(const openrac_front_end& fe, int game) {
    const int page = mod_page_of(game);

    if (page < 0) {
        return 1.0f;
    }

    const auto& need = needs(fe, page);
    const gaddr seqs = s.pages[static_cast<std::size_t>(game)].page + kSeqs;
    std::array<menu_fit::Frame, kFitSlots.size()> frames{};

    for (std::size_t i = 0; i < kFitSlots.size(); ++i) {
        const auto seq = GREF(std::int32_t, seqs + 4 * static_cast<gaddr>(kFitSlots[i]));

        if (seq >= 0 && seq < static_cast<std::int32_t>(s.fit.bases.size())) {
            const Base& b = s.fit.bases[static_cast<std::size_t>(seq)];
            frames[i] = {b.width, b.centre, need[i]};
        }
    }

    return menu_fit::stretch(frames, kSafeLeft, kSafeRight);
}

// The frames of page `page`, settled, at stretch 1: each one's width learnt for its seq once the
// same two ticks running (the switch's flight over), where it is not the one known.
void learn(const openrac_front_end& fe, gaddr page) {
    for (std::size_t i = 0; page != 0 && i < kFitSlots.size(); ++i) {
        Stretched& st = s.fit.slots[i];
        const gaddr moby = panel(fe, kFitSlots[i]);
        const gaddr frame = moby != 0 ? word(moby + kMobyFrame) : 0;
        const auto seq = GREF(std::int32_t, page + kSeqs + 4 * static_cast<gaddr>(kFitSlots[i]));

        if (frame == 0 || st.moby != 0 || seq < 0
            || seq >= static_cast<std::int32_t>(s.fit.bases.size())
            || GREF(std::uint8_t, moby + kMobySeq) != seq) {
            st.seen_seq = -1;
            continue;
        }

        const auto width = static_cast<float>(GREF(std::int32_t, frame + kFrameWidth));
        const auto x = static_cast<float>(GREF(std::int32_t, frame + kFrameX));
        Base& b = s.fit.bases[static_cast<std::size_t>(seq)];

        if (st.seen_seq == seq && st.seen_width == width && st.seen_x == x && width > 0.0f
            && (std::fabs(b.width - width) > 1.0f
                || std::fabs(b.centre - (x + width / 2.0f)) > 1.0f)) {
            log::verbose(
                "mod ui: seq {}'s frame is {} wide at {} (was known as {} at {})",
                seq,
                width,
                x + width / 2.0f,
                b.width,
                b.centre
            );
            b = {width, x + width / 2.0f};
        }

        st.seen_seq = seq;
        st.seen_width = width;
        st.seen_x = x;
    }
}

// A frame moby given rows and a place of its own, `by` times as wide about its frame's centre (as
// the corners its update put last tick, with what it had then, place it).
void stretch_moby(Stretched& st, float by) {
    const gaddr moby = st.moby;
    const gaddr frame = word(moby + kMobyFrame);
    float centre = 0.0f;

    if (frame != 0) {
        const float middle = (GREF(float, frame + kCornerFirst + kAcross)
                              + GREF(float, frame + kCornerOpposite + kAcross))
                             / 2.0f;
        centre = (middle - GREF(float, moby + kMobyPosition + kAcross)) / st.by;
    }

    GREF(float, moby + kMobyPosition + kAcross) = st.position_y - (by - 1.0f) * centre;

    for (std::size_t r = 0; r < st.row_y.size(); ++r) {
        GREF(float, moby + kMobyRows + kAcross + 0x10 * static_cast<gaddr>(r)) = st.row_y[r] * by;
    }

    GREF(std::uint16_t, moby + kMobyMode) |= kOwnRows;
    st.by = by;
}

// A frame moby as the game made it again.
void release(Stretched& st) {
    const gaddr moby = st.moby;
    GREF(float, moby + kMobyPosition + kAcross) = st.position_y;

    for (std::size_t r = 0; r < st.row_y.size(); ++r) {
        GREF(float, moby + kMobyRows + kAcross + 0x10 * static_cast<gaddr>(r)) = st.row_y[r];
    }

    GREF(std::uint16_t, moby + kMobyMode) &= static_cast<std::uint16_t>(~kOwnRows);
    st.moby = 0;
    st.by = 1.0f;
}

// The title's, list's and footer's frame mobys at stretch `by` (1: the game's). One the menu made
// anew is not the one stretched: that one is gone.
void apply_stretch(const openrac_front_end& fe, float by) {
    for (std::size_t i = 0; i < kFitSlots.size(); ++i) {
        Stretched& st = s.fit.slots[i];
        const gaddr moby = panel(fe, kFitSlots[i]);

        if (st.moby != 0 && st.moby != moby) {
            st.moby = 0;
            st.by = 1.0f;
        }

        if (moby == 0) {
            continue;
        }

        if (!(by > 1.0005f)) {
            if (st.moby != 0) {
                release(st);
            }

            continue;
        }

        if (st.moby == 0) {
            if ((GREF(std::uint16_t, moby + kMobyMode) & kRowFlipped) != 0) {
                continue;
            }

            st.moby = moby;
            st.position_y = GREF(float, moby + kMobyPosition + kAcross);

            for (std::size_t r = 0; r < st.row_y.size(); ++r) {
                st.row_y[r] =
                    GREF(float, moby + kMobyRows + kAcross + 0x10 * static_cast<gaddr>(r));
            }

            st.by = 1.0f;
        }

        stretch_moby(st, by);
    }
}

// Once a tick: the frames' own widths learnt, the stretch the page on the screen (or the switch
// running) gives them, and a footer that dropped to the small font while too narrow given its
// own again once it fits.
void fit_frames(const openrac_front_end& fe) {
    if (fe.panel_slots == 0) {
        return;
    }

    Fit& f = s.fit;
    const gaddr menu = fe.page_menu;
    const bool switching = GREF(std::int32_t, menu + kState) == kSwitching;

    if (!switching) {
        learn(fe, word(menu + kCurrent));
    }

    if (switching) {
        if (!f.switching) {
            f.from = f.by;
            f.to = target(fe, game_page_of(word(menu + kNext)));
            f.game = -1;
            f.page = -1;
        }

        const auto left = static_cast<float>(GREF(std::int32_t, menu + kTimer));
        const float done = std::clamp((kSwitchTicks - left + 1.0f) / kSwitchTicks, 0.0f, 1.0f);
        f.by = f.from + (f.to - f.from) * done;
    } else {
        const int game = game_page_of(word(menu + kCurrent));
        const int page = mod_page_of(game);

        if (page < 0) {
            f.by = 1.0f;
            f.game = -1;
            f.page = -1;
        } else {
            // A page shown takes its own stretch; while it stays, it only grows.
            const float t = target(fe, game);

            // For OPENRAC_LOG=verbose: what the page's texts need and the stretch it takes.
            if (t != f.logged || game != f.game || page != f.page) {
                const auto& n = needs(fe, page);
                log::verbose(
                    "mod ui: page {} needs title {}, list {}, footer {}: stretch {}",
                    page,
                    n[0],
                    n[1],
                    n[2],
                    t
                );
                f.logged = t;
            }

            f.goal = game == f.game && page == f.page ? std::max(f.goal, t) : t;
            f.game = game;
            f.page = page;
            f.by += (f.goal - f.by) * kEase;

            if (std::fabs(f.goal - f.by) < 0.002f) {
                f.by = f.goal;
            }

            // The footer's large font back once its frame is wide enough (the game keeps the small
            // one once it took it).
            const GamePage& g = s.pages[static_cast<std::size_t>(game)];
            const gaddr moby = panel(fe, kFooterSlot);
            const gaddr frame = moby != 0 ? word(moby + kMobyFrame) : 0;
            const float need = needs(fe, page)[2];
            const std::uint32_t flags = word(g.footer + kListFlags);

            if (frame != 0 && need > 0.0f && (flags & kSmallFont) != 0 && !s.asking
                && static_cast<float>(GREF(std::int32_t, frame + kFrameWidth)) >= need) {
                put_word(g.footer + kListFlags, flags & ~kSmallFont);
            }
        }
    }

    f.switching = switching;
    apply_stretch(fe, f.by);
}

// No page of a mod's any more (the menu gone, another program): the frames as the game made them
// where they are still the menu's, and nothing kept of the rest.
void unfit(const openrac_front_end& fe) {
    for (std::size_t i = 0; fe.panel_slots != 0 && i < kFitSlots.size(); ++i) {
        Stretched& st = s.fit.slots[i];

        if (st.moby != 0 && st.moby == panel(fe, kFitSlots[i])) {
            release(st);
        }

        st = Stretched{};
    }

    s.fit.by = 1.0f;
    s.fit.switching = false;
    s.fit.game = -1;
    s.fit.page = -1;
}

// The centre of the frame of the moby in `slot` along the world's z (the screen's y), or false
// while it is not the row's `seq` settled.
bool row_middle(const openrac_front_end& fe, int slot, std::int32_t seq, float& middle) {
    const gaddr moby = panel(fe, slot);
    const gaddr frame = moby != 0 ? word(moby + kMobyFrame) : 0;

    if (frame == 0 || GREF(std::uint8_t, moby + kMobySeq) != seq) {
        return false;
    }

    middle = (GREF(float, frame + kCornerFirst + kUpward)
              + GREF(float, frame + kCornerOpposite + kUpward))
             / 2.0f;
    return true;
}

// A row's moby at its own z, where it was moved.
void lower(Raise& r, std::size_t row) {
    const gaddr moby = r.moby[row];

    if (GREF(float, moby + kMobyPosition + kUpward) == r.put_z[row]) {
        GREF(float, moby + kMobyPosition + kUpward) = r.own_z[row];
    }

    r.moby[row] = 0;
}

// Once a tick: the main menu's rows raised by the part of the height the main menu has on the
// screen (all of it while shown, along a switch to or from it as the frames fly, none on another
// page), each moby put back at its own z once it has none.
void raise_rows(const openrac_front_end& fe) {
    Raise& r = s.raise;
    const int rows = kGameRows + s.extra_rows;
    const gaddr menu = fe.page_menu;
    const bool switching = GREF(std::int32_t, menu + kState) == kSwitching;
    const bool main_menu = word(menu + kCurrent) == fe.main_page;

    // The game's first two rows' step, half of it per extra row: followed while the main menu's
    // rows fly in (both play the same flight, a step apart), so that the rows are raised from their
    // first showing, and kept once the main menu is settled.
    const bool coming = switching && word(menu + kNext) == fe.main_page;

    if (!r.learnt && s.extra_rows > 0 && (main_menu || coming)) {
        float first = 0.0f;
        float second = 0.0f;

        if (row_middle(fe, kRowSlots[0], kRowSeqs[0], first)
            && row_middle(fe, kRowSlots[1], kRowSeqs[1], second)) {
            r.step = first - second;
            r.height = r.step * static_cast<float>(s.extra_rows) / 2.0f;
            r.learnt = main_menu && !switching;

            if (r.learnt) {
                log::verbose("mod ui: the main menu's rows raised by {}", r.height);
            }
        }
    }

    float goal = main_menu ? 1.0f : 0.0f;
    float done = 1.0f;

    if (switching) {
        const auto left = static_cast<float>(GREF(std::int32_t, menu + kTimer));
        done = std::clamp((kSwitchTicks - left + 1.0f) / kSwitchTicks, 0.0f, 1.0f);
        const bool out = word(menu + kPrevious) == fe.main_page;
        goal = coming == out ? (coming ? 1.0f : 0.0f) : (coming ? done : 1.0f - done);
        r.by = goal;
    } else {
        r.by += (goal - r.by) * kEase;

        if (std::fabs(goal - r.by) < 0.002f) {
            r.by = goal;
        }
    }

    for (int i = 0; i < rows; ++i) {
        const auto row = static_cast<std::size_t>(i);
        const gaddr moby = panel(fe, kRowSlots[row]);

        // One the menu made anew is not the one moved: that one is gone.
        if (r.moby[row] != 0 && r.moby[row] != moby) {
            r.moby[row] = 0;
        }

        if (moby == 0) {
            continue;
        }

        if (r.by == 0.0f || r.height == 0.0f) {
            if (r.moby[row] != 0) {
                lower(r, row);
            }

            continue;
        }

        const float z = GREF(float, moby + kMobyPosition + kUpward);

        // New, or moved by the game since: its own z is where it is now.
        if (r.moby[row] == 0 || z != r.put_z[row]) {
            r.moby[row] = moby;
            r.own_z[row] = z;
        }

        r.put_z[row] = r.own_z[row] + r.height * r.by;

        // Flying Options' flight: from its own row (away) or to it (back), `row - Options` steps
        // below Options at the row, none at the page.
        if (s.follow_page != 0 && switching && i >= kGameRows) {
            r.put_z[row] -= r.step * static_cast<float>(i - (kGameRows - 1))
                            * (s.follow_out ? 1.0f - done : done);
        }

        GREF(float, moby + kMobyPosition + kUpward) = r.put_z[row];
    }
}

// The menu gone: the rows' mobys put back where they are still the menu's.
void unraise(const openrac_front_end& fe) {
    Raise& r = s.raise;

    for (std::size_t row = 0; fe.panel_slots != 0 && row < kRowSlots.size(); ++row) {
        if (r.moby[row] != 0 && r.moby[row] == panel(fe, kRowSlots[row])) {
            lower(r, row);
        }

        r.moby[row] = 0;
    }

    r.by = 0.0f;
}

void cue(const openrac_front_end& fe, int sound) {
    if (s.game >= 0) {
        fe.menu_cue(sound, word(s.pages[static_cast<std::size_t>(s.game)].list + kMoby));
    }
}

void send(int page, std::uint32_t kind, int item = -1, int value = 0, std::uint32_t tag = 0) {
    openrac_ui_event e{};
    e.kind = kind;
    e.item = item;
    e.value = value;
    e.tag = tag;
    mods::ui_send(page, e);
}

// The question on the page shown: the prompt box in the list's slot, the prompt's text the
// question (text()), the footer the answers.
void ask(const openrac_front_end& fe, const std::string& question, std::uint32_t tag) {
    const GamePage& g = s.pages[static_cast<std::size_t>(s.game)];
    // A newline is the game's (0x01).
    std::string text = options_menu::game_text(question);
    std::replace(text.begin(), text.end(), '\n', '\x01');
    std::snprintf(static_cast<char*>(G(s.question)), kTextBytes * 4, "%s", text.c_str());
    s.asking = true;
    s.tag = tag;
    s.kind = GREF(std::int32_t, fe.page_menu + kPromptKind);
    GREF(std::int32_t, fe.page_menu + kPromptKind) = 0;
    put_word(s.prompt + kMoby, word(g.list + kMoby));
    put_word(g.page + kScreens + 4 * kListSlot, s.prompt);
    cue(fe, OPENRAC_UI_SOUND_OPEN);
}

// The question closed with `yes` or no: the list back in its slot, the page told.
void answer(const openrac_front_end& fe, bool yes) {
    if (!s.asking) {
        return;
    }

    const GamePage& g = s.pages[static_cast<std::size_t>(s.game)];
    s.asking = false;
    GREF(std::int32_t, fe.page_menu + kPromptKind) = s.kind;
    put_word(g.page + kScreens + 4 * kListSlot, g.list);
    send(s.shown, OPENRAC_UI_ANSWER, -1, yes ? 1 : 0, s.tag);
}

// Back a level, as the game's back button: the switch to the page's back (the main menu, or the
// first level under the second).
void go_back(const openrac_front_end& fe) {
    put_word(fe.page_menu + kNext, word(s.pages[static_cast<std::size_t>(s.game)].page + kBack));
}

// The page shown opens mod page `page`: a level down from a first level, in place of the second
// level's, or back to the first level it is under.
void open(const openrac_front_end& fe, int page) {
    if (page == OPENRAC_UI_GO_BACK) {
        go_back(fe);
        return;
    }

    if (page == s.shown) {
        return;
    }

    GamePage& second = s.pages[kSecondLevel];

    if (s.game != kSecondLevel) {
        s.second = page;
        put_word(second.page + kBack, s.pages[static_cast<std::size_t>(s.game)].page);
        put_word(second.list + kListScroll, 0);
        show(kSecondLevel, page);
        put_word(fe.page_menu + kNext, second.page);
        return;
    }

    // The first level the second is under: back to it.
    if (page == mod_page_of(game_page_of(word(second.page + kBack)))) {
        go_back(fe);
        return;
    }

    // Another in the second level's place: its content, the page-change sound.
    answer(fe, false);
    send(s.shown, OPENRAC_UI_HIDE);
    s.second = page;
    s.shown = page;
    put_word(second.list + kListScroll, 0);
    cue(fe, OPENRAC_UI_SOUND_PAGE);
    mods::ui_set_shown(page, false);
    send(page, OPENRAC_UI_SHOW);
}

// From the title's main menu, mod page `page` opened as its main-menu item's own: the switch to
// that item's page. Nothing for a page no item opens.
void open_main_item(const openrac_front_end& fe, int page) {
    for (int i = 0; i < s.extra_rows; ++i) {
        if (mods::ui_main_item(i)->page == page) {
            put_word(fe.page_menu + kNext, s.pages[static_cast<std::size_t>(i)].page);
            return;
        }
    }
}

// The next item from the cursor that it takes, `by` 1 down or -1 up, round the ends; the cursor's
// own when there is none.
int step_cursor(const mods::UiPage& p, int by) {
    const int n = static_cast<int>(p.items.size());

    for (int k = 1; k < n; ++k) {
        const int i = ((p.cursor + by * k) % n + n) % n;

        if (mods::takes_cursor(p.items[static_cast<std::size_t>(i)])) {
            return i;
        }
    }

    return p.cursor;
}

// The buttons pressed this tick, with the directions repeated while held.
std::uint16_t take_pressed() {
    std::uint16_t pressed = s.pressed;
    s.pressed = 0;

    for (std::size_t d = 0; d < kDirections.size(); ++d) {
        const std::uint16_t b = kDirections[d];
        int& t = s.held_for[d];
        t = (s.held & b) != 0 ? t + 1 : 0;

        if (t > kRepeatAfter && (t - kRepeatAfter) % kRepeatEvery == 0) {
            pressed |= b;
        }
    }

    return pressed;
}

// A choice stepped by `by`: shown at once, the page told, the toggle's sound.
void step_choice(const openrac_front_end& fe, const mods::UiItem& item, int index, int by) {
    if ((item.flags & OPENRAC_UI_DISABLED) != 0) {
        cue(fe, OPENRAC_UI_SOUND_DENIED);
        return;
    }

    const int n = static_cast<int>(item.choices.size());
    const int value = ((static_cast<int>(item.choice) + by) % n + n) % n;
    mods::ui_set_choice(s.shown, index, static_cast<std::uint32_t>(value));
    cue(fe, OPENRAC_UI_SOUND_SELECT);
    send(s.shown, OPENRAC_UI_CHANGE, index, value);
}

// The tick's buttons on the page shown: the question's answer, or the cursor, a choice, Cross on
// an item, Triangle.
void input(const openrac_front_end& fe) {
    const std::uint16_t pressed = take_pressed();

    if (s.asking) {
        if ((pressed & (kCross | kTriangle)) != 0) {
            const bool yes = (pressed & kCross) != 0;
            cue(fe, yes ? OPENRAC_UI_SOUND_SELECT : OPENRAC_UI_SOUND_PAGE);
            answer(fe, yes);
        }

        return;
    }

    const mods::UiPage* p = mods::ui_page(s.shown);

    if (p == nullptr) {
        return;
    }

    if ((pressed & (kUp | kDown)) != 0 && !p->items.empty()) {
        const int to = step_cursor(*p, (pressed & kUp) != 0 ? -1 : 1);

        if (to != p->cursor) {
            mods::ui_set_cursor(s.shown, to);
            cue(fe, OPENRAC_UI_SOUND_CURSOR);
        }
    }

    const int at = p->cursor;
    const mods::UiItem* item = at >= 0 && at < static_cast<int>(p->items.size())
                                   ? &p->items[static_cast<std::size_t>(at)]
                                   : nullptr;

    if (item != nullptr && item->kind == OPENRAC_UI_CHOICE
        && (pressed & (kLeft | kRight | kCross)) != 0) {
        step_choice(fe, *item, at, (pressed & kLeft) != 0 ? -1 : 1);
    } else if (item != nullptr && item->kind == OPENRAC_UI_BUTTON && (pressed & kCross) != 0) {
        const bool disabled = (item->flags & OPENRAC_UI_DISABLED) != 0;
        cue(fe, disabled ? OPENRAC_UI_SOUND_DENIED : OPENRAC_UI_SOUND_SELECT);

        if (!disabled) {
            send(s.shown, OPENRAC_UI_CHOOSE, at);
        }
    }

    if ((pressed & kTriangle) != 0) {
        if ((p->flags & OPENRAC_UI_OWN_BACK) != 0) {
            send(s.shown, OPENRAC_UI_BACK);
        } else {
            go_back(fe);
        }
    }
}

// What the mods asked for during the tick's events.
void apply(const openrac_front_end& fe) {
    const mods::UiRequests r = mods::take_ui_requests();

    for (const int sound : r.sounds) {
        cue(fe, sound);
    }

    if (r.confirm && r.confirm_page == s.shown && !s.asking) {
        ask(fe, r.question, r.tag);
    }

    if (r.open) {
        open(fe, r.page);
    }
}

// The page on the screen changed to game page `game`: the last one told it left (its question
// closed with no), the new one that it came.
void change_page(const openrac_front_end& fe, int game) {
    if (s.shown >= 0) {
        answer(fe, false);
        send(s.shown, OPENRAC_UI_HIDE);
        s.swallow = s.held;
    }

    s.game = game;
    s.shown = mod_page_of(game);
    s.pressed = 0;
    s.held_for.fill(0);

    if (s.shown >= 0) {
        mods::ui_set_shown(s.shown, false);
        send(s.shown, OPENRAC_UI_SHOW);
    }
}

/*
 * The pause menu's pages of the mods' items (openrac_api.ui_pause_item). In a level the pause
 * menu's Options page gets an item per mod's after Video (options_menu.cpp makes them with the
 * Options items of each level program), each opening a copy of the game's Camera page: its title
 * box and its list of rows, each a label and a value (pause_rows.h). The game draws them and moves
 * their cursor, with its own sounds; Triangle is the game's (back to Options) but on a page made
 * with OPENRAC_UI_OWN_BACK and while a question is open, where the port takes it. What the rows say
 * is put each tick from the mod's page, a window of kPauseRowsShown of its items moving with the
 * cursor; a question is the title and two rows, Yes and No.
 */
namespace pause {

constexpr int kRows = options_menu::kPauseRowsShown;
// The texts' ids, past the main menu pages' (kTextFirst + kTexts) and below the port's Exit's
// (0x7D80, exit_menu.cpp): the items' labels, the page's title, its rows' labels, then their values.
constexpr int kItemText = 0x7D00;
constexpr int kTitleText = kItemText + OPENRAC_UI_PAUSE_ITEMS_MAX;
constexpr int kRowText = kTitleText + 1;
constexpr int kAllTexts = kRowText + 2 * kRows - kItemText;
static_assert(kTextFirst + kTexts <= kItemText && kItemText + kAllTexts <= 0x7D80);
// The row widget's list: its cursor (its rows at +0x34); a row's bytes and its int's value at rest.
constexpr gaddr kCursorAt = 0x38;
constexpr gaddr kRowBytes = 0x18;
constexpr std::int32_t kAtRest = 1;
// The pad's pressed buttons as the game's menus test them (its +0x1C4, the two bytes swapped).
constexpr gaddr kPadPressed = 0x1C4;
constexpr std::uint32_t kPadLeft = 0x8000;
constexpr std::uint32_t kPadRight = 0x2000;

struct State {
    gaddr texts = 0;
    int item = -1;   // the pause item whose page is current, -1 none
    int shown = -1;  // the mod's page on it (its own, or one ui_open put there)
    int top = 0;     // the item in the first row
    bool asking = false;
    std::uint32_t tag = 0;
    std::string question;
    std::uint16_t taken = 0;    // Triangle pressed while the port has it
    std::uint16_t swallow = 0;  // held as the port let go of it
    bool takes_back = false;    // the port has Triangle now
    std::string logged;         // what the rows showed when last logged
};

State p;

gaddr text_at(int id) {
    return p.texts + kTextBytes * static_cast<gaddr>(id - kItemText);
}

void put(int id, const std::string& utf8) {
    std::snprintf(static_cast<char*>(G(text_at(id))), kTextBytes, "%s", game_line(utf8).c_str());
}

void tell(std::uint32_t kind, int item = -1, int value = 0, std::uint32_t tag = 0) {
    openrac_ui_event e{};
    e.kind = kind;
    e.item = item;
    e.value = value;
    e.tag = tag;
    mods::ui_send(p.shown, e);
}

void answer(bool yes) {
    if (!p.asking) {
        return;
    }
    p.asking = false;
    tell(OPENRAC_UI_ANSWER, -1, yes ? 1 : 0, p.tag);
}

// The page current changed to pause item `item`'s (-1 none): the last told it left, the new one
// that it came.
void change(int item) {
    if (p.shown >= 0) {
        answer(false);
        tell(OPENRAC_UI_HIDE);
    }
    p.item = item;
    p.shown = -1;
    p.top = 0;
    p.taken = 0;
    if (const mods::UiPauseItem* it = mods::ui_pause_item(item); it != nullptr) {
        p.shown = it->page;
        put_word(options_menu::pause_rows(item).list + kCursorAt, 0);
        mods::ui_set_shown(p.shown, false);
        tell(OPENRAC_UI_SHOW);
    }
}

// What the rows show of `page` now, from its item `top`: the lines, as many as there are.
std::vector<PauseRow> lines(const mods::UiPage& page) {
    if (p.asking) {
        return {{"Yes", ""}, {"No", ""}};
    }
    std::vector<PauseRow> out;
    for (const mods::UiItem& item : page.items) {
        out.push_back(
            pause_row(item.kind == OPENRAC_UI_CHOICE, item.label, item.choices, item.choice)
        );
    }
    return out;
}

// The rows of `rows` showing `page` from p.top.
void fill(const options_menu::PauseRows& rows, const mods::UiPage& page) {
    const std::vector<PauseRow> all = lines(page);
    put(kTitleText, p.asking ? p.question : page.title);
    const int count = std::clamp(static_cast<int>(all.size()) - p.top, 1, kRows);  // one at least
    for (int i = 0; i < kRows; ++i) {
        const gaddr row = rows.rows + kRowBytes * static_cast<gaddr>(i);
        const std::size_t at = static_cast<std::size_t>(p.top + i);
        GREF(std::int32_t, row) = i < count ? kRowText + i : 0;  // a 0 label ends the rows
        put(kRowText + i, at < all.size() ? all[at].label : "");
        put(kRowText + kRows + i, at < all.size() ? all[at].value : "");
    }
    GREF(std::int32_t, rows.title + 0x34) = kTitleText;
    // What the page shows, logged (verbose) each time it changes.
    std::string shown = p.asking ? p.question : page.title;
    for (int i = 0; i < count; ++i) {
        const std::size_t at = static_cast<std::size_t>(p.top + i);
        if (at < all.size()) {
            shown += " | " + all[at].label + (all[at].value.empty() ? "" : ": " + all[at].value);
        }
    }
    if (shown != p.logged) {
        p.logged = shown;
        log::verbose("mod ui: pause page {}", shown);
    }
}

void step(const mods::UiPage& page, int index, int by) {
    const mods::UiItem& item = page.items[static_cast<std::size_t>(index)];
    if (item.kind != OPENRAC_UI_CHOICE || item.choices.empty()
        || (item.flags & OPENRAC_UI_DISABLED) != 0) {
        return;
    }
    const int n = static_cast<int>(item.choices.size());
    const int value = ((static_cast<int>(item.choice) + by) % n + n) % n;
    mods::ui_set_choice(p.shown, index, static_cast<std::uint32_t>(value));
    tell(OPENRAC_UI_CHANGE, index, value);
}

// The tick's input on the page: a row the widget stepped (Cross), Left and Right, Triangle when the
// port has it; the cursor's window.
void input(const openrac_front_end& fe, const options_menu::PauseRows& rows) {
    const mods::UiPage* page = mods::ui_page(p.shown);
    if (page == nullptr) {
        return;
    }
    int crossed = -1;
    for (int i = 0; i < kRows; ++i) {
        std::int32_t& v = GREF(std::int32_t, rows.values + 4 * static_cast<gaddr>(i));
        if (v != kAtRest) {
            crossed = p.top + i;
            v = kAtRest;
        }
    }
    const std::uint16_t taken = std::exchange(p.taken, 0);
    const int count = p.asking ? 2 : static_cast<int>(page->items.size());
    std::int32_t& cursor = GREF(std::int32_t, rows.list + kCursorAt);
    cursor = std::clamp(cursor, 0, std::max(std::min(count - p.top, kRows) - 1, 0));
    if (p.asking) {
        if (crossed >= 0 || (taken & kTriangle) != 0) {
            answer(crossed == 0 && (taken & kTriangle) == 0);
            p.top = 0;
            cursor = 0;
        }
        return;
    }
    const int at = p.top + cursor;
    const std::uint32_t pressed = GREF(std::uint32_t, fe.pad + kPadPressed);
    if (crossed >= 0 && crossed < count) {
        const mods::UiItem& item = page->items[static_cast<std::size_t>(crossed)];
        if (item.kind == OPENRAC_UI_BUTTON && (item.flags & OPENRAC_UI_DISABLED) == 0) {
            tell(OPENRAC_UI_CHOOSE, crossed);
        } else {
            step(*page, crossed, 1);
        }
    } else if ((pressed & (kPadLeft | kPadRight)) != 0 && at < count) {
        step(*page, at, (pressed & kPadRight) != 0 ? 1 : -1);
    }
    if ((taken & kTriangle) != 0) {
        tell(OPENRAC_UI_BACK);
    }
    pause_scroll(p.top, cursor, count, kRows);
    mods::ui_set_cursor(p.shown, p.top + cursor);
}

// What the mod asked for during the tick's events: a question, a page in this one's place, back.
void apply(gaddr menu, const options_menu::PauseRows& rows) {
    const mods::UiRequests r = mods::take_ui_requests();
    if (r.confirm && r.confirm_page == p.shown && !p.asking) {
        p.asking = true;
        p.tag = r.tag;
        p.question = r.question;
        p.top = 0;
        put_word(rows.list + kCursorAt, 1);  // on No
    }
    if (!r.open) {
        return;
    }
    if (r.page == OPENRAC_UI_GO_BACK) {
        put_word(menu + kNext, word(rows.page + kBack));
    } else if (r.page != p.shown) {
        answer(false);
        tell(OPENRAC_UI_HIDE);
        p.shown = r.page;
        p.top = 0;
        put_word(rows.list + kCursorAt, 0);
        mods::ui_set_shown(p.shown, false);
        tell(OPENRAC_UI_SHOW);
    }
}

void reset() {
    if (p.shown >= 0) {
        change(-1);
    }
    p.item = -1;
    p.takes_back = false;
}

// Once a tick in a level: the page current, the mod's page on it, its input and events.
void update(const openrac_front_end& fe) {
    const int count = std::min(mods::ui_pause_item_count(), OPENRAC_UI_PAUSE_ITEMS_MAX);
    // A menu over the level (the pause menu, the game's dialogs); its pages stay in memory after it.
    std::int32_t level = -1;
    const openrac_game_play* play = openrac_game_play_host;
    if (count == 0 || play == nullptr || play->phase(&level) != OPENRAC_GAME_PHASE_MENU) {
        reset();
        return;
    }
    if (p.texts == 0) {
        p.texts = openrac_guest_static(kTextBytes * kAllTexts);
        std::memset(G(p.texts), 0, kTextBytes * kAllTexts);
    }
    for (int i = 0; i < count; ++i) {
        put(kItemText + i, mods::ui_pause_item(i)->label);
    }
    const gaddr menu = OPENRAC_DATA(fe.page_menu);
    const gaddr current = word(menu + kCurrent);
    const gaddr next = word(menu + kNext);
    int item = -1, coming = -1;
    for (int i = 0; i < count; ++i) {
        const gaddr page = options_menu::pause_rows(i).page;
        item = page != 0 && page == current ? i : item;
        coming = page != 0 && page == next ? i : coming;
    }
    if (item != p.item) {
        change(item);
    }
    if (p.shown < 0) {
        p.takes_back = false;
        mods::ui_set_shown(-1, false);
        mods::take_ui_requests();
        // A switch to a mod's page: its first rows ready before it shows.
        if (const mods::UiPauseItem* it = mods::ui_pause_item(coming); it != nullptr) {
            if (const mods::UiPage* page = mods::ui_page(it->page); page != nullptr) {
                p.top = 0;
                fill(options_menu::pause_rows(coming), *page);
            }
        }
        return;
    }
    const options_menu::PauseRows rows = options_menu::pause_rows(p.item);
    mods::ui_set_shown(p.shown, p.asking);
    input(fe, rows);
    tell(OPENRAC_UI_FRAME);
    apply(menu, rows);
    mods::ui_set_shown(p.shown, p.asking);
    if (const mods::UiPage* page = mods::ui_page(p.shown); page != nullptr) {
        fill(rows, *page);
        p.takes_back = p.asking || (page->flags & OPENRAC_UI_OWN_BACK) != 0;
    }
}

}  // namespace pause

}  // namespace

void update(int loaded_overlay) {
    const openrac_front_end* fe = front_end();

    // Only the boot program's menus while the title stays (a game a mod's page started leaves it:
    // the movies and the load after it have the pad). The main menu has the port's Exit even with
    // no mod's item.
    const int title_exit =
        fe != nullptr && fe->title_exit != 0 ? GREF(std::int16_t, fe->title_exit) : 0;

    if (fe == nullptr || !menus_up(loaded_overlay == OPENRAC_OVERLAY_EXE, title_exit)) {
        if (fe != nullptr && s.shown >= 0) {
            change_page(*fe, -1);
        }

        if (fe != nullptr && s.built) {
            unfit(*fe);
            unraise(*fe);
            unfollow();
        }

        s.game = -1;
        s.shown = -1;
        mods::ui_set_shown(-1, false);
        mods::ui_set_main_menu(false);
        mods::take_ui_requests();
        // In a level: the pause menu's pages of the mods' items.
        if (fe != nullptr && loaded_overlay != OPENRAC_OVERLAY_EXE && loaded_overlay >= 0) {
            pause::update(*fe);
        } else {
            pause::reset();
        }
        return;
    }
    pause::reset();

    build(*fe);
    patch_main(*fe);
    follow_switch(*fe);
    exit_menu::update_title(*fe);
    // A page is on the screen once its switch is over.
    const bool settled = GREF(std::int32_t, fe->page_menu + kState) != kSwitching;
    const int game = settled ? game_page_of(word(fe->page_menu + kCurrent)) : -1;

    if (game != s.game || mod_page_of(game) != s.shown) {
        change_page(*fe, game);
    }

    // The title's main menu itself on the screen: a mod may open its main-menu item's page.
    const bool main_menu = settled && word(fe->page_menu + kCurrent) == fe->main_page;

    if (s.shown >= 0) {
        mods::ui_set_shown(s.shown, s.asking);
        mods::ui_set_main_menu(false);
        input(*fe);
        send(s.shown, OPENRAC_UI_FRAME);
        apply(*fe);
        mods::ui_set_shown(s.shown, s.asking);
    } else {
        mods::ui_set_shown(-1, false);
        const mods::UiRequests r = mods::take_ui_requests();

        if (main_menu && r.open) {
            open_main_item(*fe, r.page);
        }

        mods::ui_set_main_menu(main_menu);
    }

    if (s.game >= 0 && s.shown >= 0) {
        show(s.game, s.shown);
    }

    // A switch's page, made ready before it shows.
    const int to = game_page_of(word(fe->page_menu + kNext));

    if (to >= 0 && to != s.game && mod_page_of(to) >= 0) {
        show(to, mod_page_of(to));
    }

    fit_frames(*fe);
    raise_rows(*fe);
}

void filter_pad(std::uint16_t& buttons) {
    const auto down = static_cast<std::uint16_t>(~buttons);
    const auto pressed = static_cast<std::uint16_t>(down & s.previous);  // newly down
    s.previous = buttons;

    // A pause page of a mod's whose Triangle the port has (its own back, a question): kept from
    // the game, and after, until let go.
    if (pause::p.takes_back) {
        pause::p.taken |= static_cast<std::uint16_t>(pressed & kTriangle);
        pause::p.swallow |= static_cast<std::uint16_t>(down & kTriangle);
    } else {
        pause::p.swallow &= down;
    }
    buttons = static_cast<std::uint16_t>(buttons | pause::p.swallow);

    // With no page of a mod shown, the pad is the game's, but for what was held as one left (the
    // game would take it for a press: Triangle on the main menu closes it).
    if (s.shown < 0) {
        s.swallow &= down;
        s.held = 0;
        buttons = static_cast<std::uint16_t>(buttons | s.swallow);
        return;
    }

    s.held = down;
    s.pressed |= pressed;
    buttons = 0xFFFF;
}

int pause_item_text(int index) {
    return pause::kItemText + index;
}

int pause_title_text() {
    return pause::kTitleText;
}

int pause_row_text() {
    return pause::kRowText;
}

std::uint32_t text(int id) {
    // The pause menu's pages' texts.
    if (pause::p.texts != 0 && id >= pause::kItemText && id < pause::kItemText + pause::kAllTexts) {
        return pause::text_at(id);
    }

    // While a question is open the memory card prompt's text is the question.
    if (id == kPromptText && s.asking) {
        return s.question;
    }

    if (!s.built || id < kTextFirst || id >= kTextFirst + kTexts) {
        return 0;
    }

    return text_at(id);
}

}  // namespace openrac::frontend::mod_ui
