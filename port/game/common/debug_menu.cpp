// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "debug_menu.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "common/log.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "renderer/gl.h"

namespace openrac::frontend::debug_menu {

namespace {

using namespace openrac::gl;

// The games the menu knows how to start a level in, and their levels.
struct Game {
    const char* id;
    std::array<const char*, 19> levels;
};

constexpr Game kGames[] = {
    {"rac1-pal",
     {"Veldin", "Novalis", "Aridia", "Kerwan", "Eudora", "Rilgar", "Blarg Station", "Umbris",
      "Batalia", "Gaspar", "Orxon", "Pokitaru", "Hoven", "Gemlik Base", "Oltanis", "Quartu",
      "Kalebo III", "Drek's Fleet", "Veldin (final)"}},
};

const Game* game_of(const char* id) {
    for (const Game& g : kGames) {
        if (id != nullptr && std::strcmp(id, g.id) == 0) {
            return &g;
        }
    }
    return nullptr;
}

// The launcher's developer mode (OPENRAC_DEVELOPER=1): without it the menus are the game's own.
bool developer() {
    static const bool on = [] {
        const char* v = std::getenv("OPENRAC_DEVELOPER");
        return v != nullptr && v[0] != '\0' && std::strcmp(v, "0") != 0;
    }();
    return on;
}

// Open only over the boot program (title and main menu), where a new game can be started.
bool available(const char* game_id, int loaded_overlay) {
    return developer() && game_of(game_id) != nullptr && loaded_overlay == OPENRAC_OVERLAY_EXE;
}

struct State {
    bool open = false;
    int cursor = 0;
    int start = -1;               // a level to start at the next update
    bool native = false;          // asked for from the game's Options page (with its sound)
    std::uint16_t previous = 0xFFFF;  // pad 0 as last seen (active low)
    // GL: the menu's picture and a framebuffer to blit it from.
    unsigned texture = 0;
    unsigned framebuffer = 0;
};
State s;

// Rac1 PAL: what --level N with OPENRAC_DIRECT does (game/rac1-pal/host/boot.c, direct_start):
// NewGameInit (whose wrapper puts openrac_game_start_level in the level word), the menus closed,
// and the flag that starts the loaded new game. Called through the functions' code addresses, as
// the game's own calls through addresses go.
void start_rac1_pal(int level) {
    const auto new_game = reinterpret_cast<void (*)(void)>(openrac_guest_function(0x00209DC0u));
    const auto close_menus = reinterpret_cast<void (*)(int)>(openrac_guest_function(0x0022F4A0u));
    if (new_game == nullptr || close_menus == nullptr) {
        log::warn("debug menu: the game's new-game functions are not known");
        return;
    }
    const int kept = openrac_game_start_level;
    openrac_game_start_level = level;
    new_game();
    openrac_game_start_level = kept;  // later new games start where they would have
    close_menus(0);
    GREF(short, 0x0013E15Au) = 1;
    log::info("debug menu: starting level {}", level);
}

// ---- Rac1 PAL: a "Planets" entry in the front end's Options list, opening a page of levels ----
//
// The front end's pages are data in the boot program (docs of ReRAC's menus.md section 3: a page is
// seq[14], parent, kind, focus, widget[14]; a list widget has its flags at +0x30, items at +0x34
// and cursor at +0x40; an item is {label, action, arg, sublabel, timer}). The Options list's enter
// (0x28dbb8 in ReRAC's level 1) puts the PAL list (Language, Video, Sound) in the widget each time,
// so its items pointer is pointed at a copy with a fourth entry whenever it shows the original.
// The entry opens (action 3) a copy of the Language page whose title and list are the game's own
// widgets with other data: the title "Planets" (text 20216) and one item per level, labelled with
// the game's planet names, drawn by the game's scroll list (func_0021E4B0) in the game's font. An
// item's action is one the list ignores (0x7F); Cross on it is taken here, with the game's confirm
// sound, and starts the level as start_rac1_pal does.
namespace pal {

constexpr gaddr kMenu = 0x001D5F70u;            // the page menu: +4 the current page
constexpr gaddr kOptionsList = 0x001D4BE8u;     // the front end's Options list widget
constexpr gaddr kPalOptionsItems = 0x001D4B90u;  // its PAL items: Language, Video, Sound
constexpr gaddr kLanguagePage = 0x001D4CC0u;
constexpr gaddr kLanguageTitle = 0x001D4D48u;
constexpr gaddr kLanguageList = 0x001D4DE8u;
constexpr std::int16_t kPlanetsText = 20216;    // "Planets"
constexpr std::int16_t kIgnoredAction = 0x7F;
// The planet names in the game's text, by level (the final Veldin is Veldin again).
constexpr std::int16_t kPlanetText[19] = {20190, 20173, 20174, 20175, 20176, 20177, 20159,
                                          20179, 20180, 20181, 20182, 20183, 20184, 20166,
                                          20186, 20187, 20188, 20170, 20190};

struct Native {
    gaddr options_items = 0;  // Language, Video, Sound, Planets
    gaddr page = 0, title = 0, list = 0, items = 0;
};
Native n;

void put_item(gaddr at, std::int16_t label, std::int16_t action, std::int32_t arg) {
    GREF(std::int16_t, at) = label;
    GREF(std::int16_t, at + 2) = action;
    GREF(std::int32_t, at + 4) = arg;
    GREF(std::int16_t, at + 8) = 0;
    GREF(std::int16_t, at + 10) = 0;
}

void build() {
    if (n.page != 0) {
        return;
    }
    n.options_items = openrac_guest_static(12 * 5);
    std::memcpy(G(n.options_items), G(kPalOptionsItems), 12 * 3);
    n.page = openrac_guest_static(0xA0);
    n.title = openrac_guest_static(0x60);
    n.list = openrac_guest_static(0x60);
    n.items = openrac_guest_static(12 * 20);
    put_item(n.options_items + 12 * 3, kPlanetsText, 3, static_cast<std::int32_t>(n.page));
    for (int i = 0; i < 19; ++i) {
        put_item(n.items + 12 * static_cast<gaddr>(i), kPlanetText[i], kIgnoredAction, i);
    }
    // The page, title and list: the Language page's, with this data.
    std::memcpy(G(n.page), G(kLanguagePage), 0xA0);
    std::memcpy(G(n.title), G(kLanguageTitle), 0x60);
    std::memcpy(G(n.list), G(kLanguageList), 0x60);
    GREF(std::int32_t, n.title + 0x34) = kPlanetsText;
    // A scrolled list (0x8000: by whole rows), its items centred (0x400), wrapping (0x1000), rows
    // as high as the font (0x10); no language marker.
    GREF(std::uint32_t, n.list + 0x30) = 0x10u | 0x400u | 0x1000u | 0x8000u;
    GREF(std::uint32_t, n.list + 0x34) = n.items;
    GREF(std::uint32_t, n.list + 0x38) = 0;
    GREF(std::uint32_t, n.list + 0x3C) = 0;
    GREF(std::int32_t, n.list + 0x40) = 0;
    GREF(std::int32_t, n.list + 0x44) = 0;
    GREF(std::uint32_t, n.page + 0x44 + 4 * 2) = n.title;
    GREF(std::uint32_t, n.page + 0x44 + 4 * 3) = n.list;
    GREF(std::uint32_t, n.page + 0x40) = n.list;  // focus
    GREF(std::uint32_t, n.page + 0x80) = 0;
}

// Once a frame over the front end: the Options list shows the entry.
void patch() {
    build();
    if (GREF(std::uint32_t, kOptionsList + 0x34) == kPalOptionsItems) {
        GREF(std::uint32_t, kOptionsList + 0x34) = n.options_items;
    }
}

bool on_page() {
    return n.page != 0 && GREF(std::uint32_t, kMenu + 4) == n.page;
}

int cursor() {
    const int c = GREF(std::int32_t, n.list + 0x40);
    return c >= 0 && c < 19 ? c : 0;
}

void confirm_sound() {
    const auto play = reinterpret_cast<void (*)(int, int, gaddr)>(openrac_guest_function(0x0022ED80u));
    if (play != nullptr) {
        play(0, 0x11, GREF(std::uint32_t, n.list + 0x14));
    }
}

}  // namespace pal

void move(int by) {
    s.cursor = (s.cursor + by + 19) % 19;
}

// ---- Drawing: a picture made on the CPU in the chip's pixels, blitted over the frame ----

// 5 x 7 glyphs, one string of 7 rows of 5 columns per character ('#' lit).
struct Glyph {
    char c;
    const char* rows;
};

constexpr Glyph kFont[] = {
    {'A', " ### #   ##   #######   ##   ##   #"}, {'B', "#### #   ##   ##### #   ##   ##### "},
    {'C', " ### #   ##    #    #    #   # ### "}, {'D', "#### #   ##   ##   ##   ##   ##### "},
    {'E', "######    #    #### #    #    #####"}, {'F', "######    #    #### #    #    #    "},
    {'G', " ### #   ##    # ####   ##   # ####"}, {'H', "#   ##   ##   #######   ##   ##   #"},
    {'I', " ###   #    #    #    #    #   ### "}, {'J', "  ###   #    #    #    # #  #  ##  "},
    {'K', "#   ##  # # #  ##   # #  #  # #   #"}, {'L', "#    #    #    #    #    #    #####"},
    {'M', "#   ### ### # ##   ##   ##   ##   #"}, {'N', "#   ###  ## # ##  ###   ##   ##   #"},
    {'O', " ### #   ##   ##   ##   ##   # ### "}, {'P', "#### #   ##   ##### #    #    #    "},
    {'Q', " ### #   ##   ##   ## # ##  #  ## #"}, {'R', "#### #   ##   ##### # #  #  # #   #"},
    {'S', " #####    #     ###     #    ##### "}, {'T', "#####  #    #    #    #    #    #  "},
    {'U', "#   ##   ##   ##   ##   ##   # ### "}, {'V', "#   ##   ##   ##   ##   # # #   #  "},
    {'W', "#   ##   ##   ## # ## # ## # # # # "}, {'X', "#   ##   # # #   #   # # #   ##   #"},
    {'Y', "#   ##   # # #   #    #    #    #  "}, {'Z', "#####    #   #   #   #   #    #####"},
    {'0', " ### #   ##  ### # ###  ##   # ### "}, {'1', "  #   ##    #    #    #    #   ### "},
    {'2', " ### #   #    #   #   #   #   #####"}, {'3', "#####   #   #     #     ##   # ### "},
    {'4', "   #   ##  # # #  # #####   #    # "}, {'5', "######    ####     #    ##   # ### "},
    {'6', "  ##  #   #    #### #   ##   # ### "}, {'7', "#####    #   #   #   #    #    #   "},
    {'8', " ### #   ##   # ### #   ##   # ### "}, {'9', " ### #   ##   # ####    #   #  ##  "},
    {'(', "   #   #   #    #    #     #     # "}, {')', " #     #     #    #    #   #   #   "},
    {'-', "                ###                "}, {':', "      ##   ##        ##   ##       "},
    {'.', "                          ##   ##  "}, {'\'', "  #    #   #                       "},
    {'/', "    #    #   #   #   #   #    #    "}, {'>', " #     #     #     #   #   #   #   "},
    {',', "                     ##    #   #   "}, {'+', "       #    #  #####  #    #       "},
};

const char* glyph(char c) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (const Glyph& g : kFont) {
        if (g.c == c && std::strlen(g.rows) >= 35) {
            return g.rows;
        }
    }
    return nullptr;
}

struct Picture {
    int width, height;
    std::vector<std::uint8_t> rgba;

    void fill(int x0, int y0, int x1, int y1, std::uint32_t colour) {
        for (int y = std::max(0, y0); y < std::min(height, y1); ++y) {
            for (int x = std::max(0, x0); x < std::min(width, x1); ++x) {
                std::memcpy(&rgba[(static_cast<std::size_t>(y) * width + x) * 4], &colour, 4);
            }
        }
    }

    // Text in 6-pixel cells; returns the x after it.
    int text(int x, int y, const std::string& t, std::uint32_t colour) {
        for (char c : t) {
            if (const char* rows = glyph(c)) {
                for (int r = 0; r < 7; ++r) {
                    for (int col = 0; col < 5; ++col) {
                        if (rows[r * 5 + col] == '#') {
                            fill(x + col, y + r, x + col + 1, y + r + 1, colour);
                        }
                    }
                }
            }
            x += 6;
        }
        return x;
    }
};

constexpr std::uint32_t rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return 0xFF000000u | (std::uint32_t{b} << 16) | (std::uint32_t{g} << 8) | r;
}

// The game's menu colours (its panels' navy, khaki frame, light blue and yellow text).
constexpr std::uint32_t kNavy = rgba(14, 14, 30);
constexpr std::uint32_t kFrame = rgba(150, 146, 104);
constexpr std::uint32_t kText = rgba(136, 168, 255);
constexpr std::uint32_t kSelected = rgba(255, 255, 64);
constexpr std::uint32_t kTitle = rgba(230, 230, 230);
constexpr std::uint32_t kHint = rgba(120, 120, 140);

constexpr int kRow = 10;
constexpr int kPanelWidth = 232;
constexpr int kPanelHeight = 30 + 19 * kRow + 18;

Picture make_picture(const Game& game) {
    Picture p{kPanelWidth, kPanelHeight, std::vector<std::uint8_t>(static_cast<std::size_t>(kPanelWidth) * kPanelHeight * 4)};
    p.fill(0, 0, p.width, p.height, kFrame);
    p.fill(2, 2, p.width - 2, p.height - 2, kNavy);
    p.text(10, 9, "OPENRAC DEBUG: START A LEVEL", kTitle);
    p.fill(8, 20, p.width - 8, 21, kFrame);
    for (int i = 0; i < 19; ++i) {
        const int y = 27 + i * kRow;
        const bool sel = i == s.cursor;
        if (sel) {
            p.fill(6, y - 2, p.width - 6, y + 9, rgba(36, 36, 70));
            p.text(10, y, ">", kSelected);
        }
        const std::string number = (i < 10 ? " " : "") + std::to_string(i);
        p.text(20, y, number + "  " + game.levels[static_cast<std::size_t>(i)], sel ? kSelected : kText);
    }
    p.text(10, p.height - 12, "X/RETURN START   TRIANGLE/F1 CLOSE", kHint);
    return p;
}

}  // namespace

bool is_open() {
    return s.open;
}

bool event(const SDL_Event& e, const char* game_id, int loaded_overlay) {
    if (e.type != SDL_EVENT_KEY_DOWN || e.key.repeat) {
        return false;
    }
    const SDL_Keycode key = e.key.key;
    if (!s.open) {
        if (key == SDLK_F1 && available(game_id, loaded_overlay)) {
            s.open = true;
            return true;
        }
        return false;
    }
    switch (key) {
        case SDLK_F1:
        case SDLK_BACKSPACE:
            s.open = false;
            return true;
        case SDLK_UP:
            move(-1);
            return true;
        case SDLK_DOWN:
            move(1);
            return true;
        case SDLK_PAGEUP:
            move(-5);
            return true;
        case SDLK_PAGEDOWN:
            move(5);
            return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            s.start = s.cursor;
            s.open = false;
            return true;
        default:
            // The game's keys stay with the menu while it is open.
            return true;
    }
}

void filter_pad(std::uint16_t& buttons, const char* game_id, int loaded_overlay) {
    const std::uint16_t pressed = static_cast<std::uint16_t>(~buttons & s.previous);  // newly down
    s.previous = buttons;
    constexpr std::uint16_t kSelect = 0x0001, kUp = 0x0010, kDown = 0x0040, kCross = 0x4000,
                            kTriangle = 0x1000;
    if (!s.open && available(game_id, loaded_overlay) && pal::on_page() && (pressed & kCross) != 0) {
        // Cross on a level of the Planets page: the game's list ignores the item's action.
        s.start = pal::cursor();
        s.native = true;
        buttons = static_cast<std::uint16_t>(buttons | kCross);
        return;
    }
    if (!s.open) {
        if ((pressed & kSelect) != 0 && available(game_id, loaded_overlay)) {
            s.open = true;
            buttons = 0xFFFF;
        }
        return;
    }
    if (!available(game_id, loaded_overlay)) {
        s.open = false;
        return;
    }
    if ((pressed & kUp) != 0) {
        move(-1);
    }
    if ((pressed & kDown) != 0) {
        move(1);
    }
    if ((pressed & (kTriangle | kSelect)) != 0) {
        s.open = false;
    }
    if ((pressed & kCross) != 0) {
        s.start = s.cursor;
        s.open = false;
    }
    buttons = 0xFFFF;
}

void update(const char* game_id, int loaded_overlay, std::uint64_t frame) {
    static const long long open_at = [] {
        const char* v = std::getenv("OPENRAC_DEBUG_MENU");
        return v != nullptr ? std::atoll(v) : -1ll;
    }();
    static bool opened_by_script = false;
    if (!opened_by_script && open_at >= 0 && frame >= static_cast<std::uint64_t>(open_at)
        && available(game_id, loaded_overlay)) {
        opened_by_script = true;
        s.open = true;
    }
    if (s.open && !available(game_id, loaded_overlay)) {
        s.open = false;
    }
    const bool pal_front_end = available(game_id, loaded_overlay) && std::strcmp(game_id, "rac1-pal") == 0;
    if (pal_front_end) {
        pal::patch();
    }
    if (s.start < 0) {
        return;
    }
    const int level = s.start;
    const bool native = s.native;
    s.start = -1;
    s.native = false;
    if (pal_front_end) {
        if (native) {
            pal::confirm_sound();
        }
        start_rac1_pal(level);
    }
}

void draw(unsigned framebuffer, int width, int height) {
    if (!s.open || width <= 0 || height <= 0) {
        return;
    }
    // The menu is only open in a game it knows (available()).
    const Game& game = kGames[0];
    const Picture p = make_picture(game);
    if (s.texture == 0) {
        glGenTextures(1, &s.texture);
        glGenFramebuffers(1, &s.framebuffer);
    }
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA8), p.width, p.height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, p.rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s.framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s.texture, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glDisable(GL_SCISSOR_TEST);
    // The picture in the chip's pixels: the frame is the game's 512 x 448 scaled.
    const float sx = static_cast<float>(width) / 512.0f;
    const float sy = static_cast<float>(height) / 448.0f;
    const int w = static_cast<int>(static_cast<float>(p.width) * sx);
    const int h = static_cast<int>(static_cast<float>(p.height) * sy);
    const int x0 = (width - w) / 2;
    const int y0 = (height - h) / 2;
    // The picture's first row is its top; GL's framebuffer counts rows from the bottom.
    glBlitFramebuffer(0, 0, p.width, p.height, x0, y0 + h, x0 + w, y0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void release() {
    if (s.texture != 0) {
        glDeleteTextures(1, &s.texture);
        glDeleteFramebuffers(1, &s.framebuffer);
        s.texture = 0;
        s.framebuffer = 0;
    }
}

}  // namespace openrac::frontend::debug_menu
