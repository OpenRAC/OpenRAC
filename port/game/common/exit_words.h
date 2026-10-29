// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The texts of the port's Exit item and its question (exit_menu.h), in the game's five languages:
// English, then French, German, Spanish and Italian, the PAL disc's (options_menu.cpp's order).
// Header only, so that every text is checked without the game (tests/exit_words_test.cpp): each
// language has one, short enough for its box, in letters the game's font has.
#pragma once

#include <cstddef>

namespace openrac::frontend::exit_menu {

enum Text {
    kExit,  // the main menu's row
    kAsk,   // the question page's title
    kYes,
    kNo,
    kTextCount
};

struct Words {
    const char* text[5];
};

// The longest each text may be, in the game's letters (one byte each): the title in the large font
// and the rows in the frames they share with the game's own.
constexpr std::size_t kLongest[kTextCount] = {16, 12, 8, 8};

// UTF-8; the accented letters are among options_menu::game_text's.
constexpr Words kWords[kTextCount] = {
    {{"Exit", "Quitter", "Beenden", "Salir", "Esci"}},
    {{"Exit?", "Quitter ?", "Beenden?", "¿Salir?", "Uscire?"}},
    {{"Yes", "Oui", "Ja", "Sí", "Sì"}},
    {{"No", "Non", "Nein", "No", "No"}},
};

}  // namespace openrac::frontend::exit_menu
