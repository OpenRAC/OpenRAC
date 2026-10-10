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

// Open only over the boot program (title and main menu), where a new game can be started.
bool available(const char* game_id, int loaded_overlay) {
    return game_of(game_id) != nullptr && loaded_overlay == OPENRAC_OVERLAY_EXE;
}

struct State {
    bool open = false;
    int cursor = 0;
    int start = -1;               // a level to start at the next update
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
    if (s.start < 0) {
        return;
    }
    const int level = s.start;
    s.start = -1;
    if (!available(game_id, loaded_overlay)) {
        return;
    }
    if (std::strcmp(game_id, "rac1-pal") == 0) {
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
