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
    kExit,      // the item: the main menu's row, the pause menu's Options item
    kExitHint,  // the pause menu's hint box for it
    kAsk,       // the main menu's question page's title
    kAskHint,   // the pause menu's hint box while it asks
    kYes,
    kNo,
    kTextCount
};

struct Words {
    const char* text[5];
};

// The longest each text may be, in the game's letters (one byte each): the title in the large font
// and the rows in the frames they share with the game's own; the hint boxes as wide as the Video
// item's hint (options_menu.cpp).
constexpr std::size_t kLongest[kTextCount] = {16, 44, 12, 44, 8, 8};

// UTF-8; the accented letters are among options_menu::game_text's.
constexpr Words kWords[kTextCount] = {
    {{"Exit", "Quitter", "Beenden", "Salir", "Esci"}},
    {{"Close OpenRAC", "Fermer OpenRAC", "OpenRAC schließen", "Cerrar OpenRAC", "Chiudi OpenRAC"}},
    {{"Exit?", "Quitter ?", "Beenden?", "¿Salir?", "Uscire?"}},
    {{"Exit? Unsaved progress will be lost",
      "Quitter ? Progrès non sauvegardés perdus",
      "Beenden? Nicht Gespeichertes geht verloren",
      "¿Salir? Se pierde lo no guardado",
      "Uscire? Progressi non salvati persi"}},
    {{"Yes", "Oui", "Ja", "Sí", "Sì"}},
    {{"No", "Non", "Nein", "No", "No"}},
};

}  // namespace openrac::frontend::exit_menu
