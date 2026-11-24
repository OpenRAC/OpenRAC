// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Exercise the actual C menu bridge on synthetic game memory: no disc assets or injected game code.
#include <cstdlib>
#include <cstring>

#include "check.h"
#include "openrac/game_host.h"
#include "openrac/memory.h"

namespace {
int vanilla_calls, requested = -1, mount = -1, result;
const openrac_front_end descriptor{.levels = 19};
const openrac_map_config maps[] = {{.name = "Harbor", .level = 1}, {.name = "Arena", .level = 1}};
}  // namespace

extern "C" {
const openrac_front_end* openrac_game_front_end = &descriptor;
const openrac_map_config* openrac_game_destinations = maps;
unsigned openrac_game_destination_count = 0;
int openrac_game_active_destination = -1;
const openrac_map_config* openrac_game_map = nullptr;
int func_00222848(gaddr menu);
int func_0021BB90(gaddr menu);
void func_L00_00290030(int target);
gaddr rac1_ship_destination_text(int id);
int rac1_flight_pictures(gaddr to, uint32_t made);
const char* openrac_game_mods_dir = "mods";

// Our PNGs: the picture one colour, the caption two; any other file missing.
unsigned char* openrac_game_load_png(const char* path, int* width, int* height) {
    const bool picture = std::strstr(path, "level_01/textures/flight_picture.png") != nullptr;
    const bool caption = std::strstr(path, "level_01/textures/flight_caption.png") != nullptr;
    if (!picture && !caption) {
        return nullptr;
    }
    *width = picture ? 128 : 256;
    *height = picture ? 128 : 32;
    auto* rgba = static_cast<unsigned char*>(std::calloc(*width * *height, 4));
    for (int i = 0; i < *width * *height; ++i) {
        rgba[i * 4] = picture ? 200 : (i % 2 ? 255 : 0);
        rgba[i * 4 + 3] = 255;
    }
    return rgba;
}

void openrac_game_free_png(unsigned char* pixels) {
    std::free(pixels);
}
int rac1_ship_text_id(int id);

int func_00222848__game(gaddr) {
    return 0;
}

int func_0021BB90__game(gaddr) {
    return result;
}

void func_L00_00290030__game(int) {
    ++vanilla_calls;
}

void func_0022F4A0(int level) {
    requested = level;
}

void openrac_game_select_destination(int index) {
    mount = index;
    openrac_game_active_destination = index;
    openrac_game_map = index >= 0 ? &maps[index] : nullptr;
}

[[noreturn]] void openrac_game_crash(const char*) {
    std::abort();
}
}

int main() {
    openrac::runtime::Memory::create();
    constexpr gaddr menu = 0x10000, page = 0x11000, old_indices = 0x12000;
    func_L00_00290030(1);
    CHECK(vanilla_calls == 1 && requested == -1);
    openrac_game_destination_count = 2;
    GREF(gaddr, 0x001602E0) = old_indices;
    GREF(gaddr, 0x001D5F74) = page;
    GREF(gaddr, page + 0x40) = menu;
    for (int i = 1; i < 19; ++i) {
        GREF(int, 0x0013D618 + (i - 1) * 4) = i;
        GREF(short, 0x001DE0C0 + i * 12) = static_cast<short>(20000 + i);
    }
    func_00222848(menu);
    const gaddr rows = GREF(gaddr, menu + 0x34);
    CHECK(rows != 0x001D6648);
    CHECK(GREF(short, rows + 18 * 12) == 0x7B00);
    CHECK(GREF(short, rows + 19 * 12) == 0x7B01);
    CHECK(GREF(short, rows + 20 * 12) == 0);
    CHECK(std::strcmp(static_cast<char*>(G(rac1_ship_destination_text(0x7B00))), "Harbor") == 0);
    CHECK(rac1_ship_destination_text(20001) == 0);
    // The template's own row keeps its name through a text of its own; the game's id behind it.
    CHECK(GREF(short, rows) == 0x7B00 + 0x80 + 1);
    CHECK(GREF(short, rows + 12) == 20002);
    CHECK(rac1_ship_text_id(0x7B00 + 0x80 + 1) == 20001);
    CHECK(rac1_ship_text_id(20002) == 20002);
    GREF(int, menu + 0x40) = 18;  // the cursor on "Harbor": the template's name reads as it
    GREF(int, 0x001DE0C0 + 12 + 4) = 30001;
    CHECK(std::strcmp(static_cast<char*>(G(rac1_ship_destination_text(30001))), "Harbor") == 0);
    CHECK(static_cast<char*>(G(rac1_ship_destination_text(20001)))[0] == 0);
    GREF(int, menu + 0x40) = 0;
    CHECK(rac1_ship_destination_text(30001) == 0);
    GREF(int, menu + 0x40) = 18;
    GREF(int, 0x0013CC04) = 0x40;
    func_0021BB90(menu);
    func_L00_00290030(1);
    CHECK(mount == 0 && requested == 1);  // logical 19 never reaches the guest loader
    CHECK(GREF(gaddr, 0x001602E0) == old_indices);
    func_00222848(menu);
    GREF(int, menu + 0x40) = 19;
    func_0021BB90(menu);
    func_L00_00290030(1);
    CHECK(mount == 1 && requested == 1);  // destination 20 shares code, not its world, with 19
    CHECK(std::strcmp(static_cast<char*>(G(rac1_ship_destination_text(0x7B01))), "Arena") == 0);
    func_00222848(menu);
    GREF(int, menu + 0x40) = 0;
    func_0021BB90(menu);
    func_L00_00290030(1);
    CHECK(mount == -1 && requested == 1);  // same template, different world
    func_L00_00290030(2);
    CHECK(vanilla_calls == 2);  // ordinary planet-to-planet travel keeps its original code
    CHECK(GREF(int, 0x0013D618) == 1);
    CHECK(GREF(int, 0x0013D618 + 18 * 4) == 0);
    func_00222848(menu);
    GREF(int, menu + 0x40) = 18;
    GREF(gaddr, page + 0x40) = 0;  // an unfocused list cannot request a map
    func_0021BB90(menu);
    func_L00_00290030(1);
    CHECK(vanilla_calls == 3 && mount == -1);
    GREF(gaddr, page + 0x40) = menu;
    func_0021BB90(menu);
    result = -1;  // cancelling must discard a previously selected custom destination
    func_0021BB90(menu);
    result = 0;
    func_L00_00290030(1);
    CHECK(vanilla_calls == 4 && mount == -1);
    // The flight's lump: a texture table (count, 19 pictures, 6 languages of 19 captions).
    constexpr gaddr lump = 0x40000, table = lump + 0x200;
    GREF(int, lump + 4) = 0x100;
    GREF(int, lump + 0x4C) = 0x100;
    GREF(int, table) = 133;
    const gaddr own_picture = table + 0x400, own_caption = own_picture + 0x4420;
    for (int i = 1; i <= 133; ++i) {
        GREF(int, table + i * 4) = static_cast<int>((i < 20 ? own_picture : own_caption) - table);
    }
    const int texture[][6] = {{128, 128, 0x13, 0, 0, 1}, {256, 32, 0x13, 0, 0, 1}};
    std::memcpy(G(own_picture + 8), texture[0], sizeof(texture[0]));
    std::memcpy(G(own_caption + 8), texture[1], sizeof(texture[1]));
    const uint32_t made = own_caption + 0x2420 - lump;
    openrac_game_active_destination = -1;
    CHECK(rac1_flight_pictures(lump, made) == 0);  // the campaign's flights keep the disc's
    openrac_game_active_destination = 0;
    const int added = rac1_flight_pictures(lump, made);
    CHECK(added == 0x4420 + 0x2420);
    const gaddr picture = table + GREF(int, table + 2 * 4);
    CHECK(picture == lump + made && GREF(int, picture + 8) == 128 && GREF(int, picture + 16) == 0x13);
    CHECK(GREF(uint32_t, picture + 0x20) == (200u | 0x80u << 24) && GREF(uint8_t, picture + 0x420) == 0);
    for (int language = 0; language < 6; ++language) {
        CHECK(GREF(int, table + (0x14 + language * 19 + 1) * 4) == static_cast<int>(picture + 0x4420 - table));
    }
    CHECK(GREF(int, table + 3 * 4) == static_cast<int>(own_picture - table));  // other planets keep theirs
    const gaddr caption = picture + 0x4420;
    CHECK(GREF(uint8_t, caption + 0x420) == 0 && GREF(uint8_t, caption + 0x421) == 1);
    CHECK(GREF(uint32_t, caption + 0x20 + 1 * 4) == (255u | 0x80u << 24));
    CHECK(rac1_flight_pictures(0x80000, 0x1000) == 0);  // any other WAD as it is
    return openrac::test::result();
}
