// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's Exit texts (game/common/exit_words.h) in each of the game's five languages: there, in
// letters the game's font has (options_menu::game_text drops any other), and no longer than the
// box they are drawn in; and a translation for every language but where the word is the same.

#include <cstdio>
#include <cstring>
#include <string>

#include "check.h"
#include "game/common/exit_words.h"

namespace {

using namespace openrac::frontend::exit_menu;

// The accented letters the game's font has (options_menu.cpp, game_text).
constexpr char32_t kGameLetters[] = {
    U'á', U'à', U'â', U'ä', U'é', U'è', U'ê', U'ë', U'í', U'ì', U'î', U'ï', U'ó', U'ò', U'ô', U'ö',
    U'ú', U'ù', U'û', U'ü', U'Á', U'À', U'Â', U'Ä', U'É', U'È', U'Ê', U'Ë', U'Í', U'Ì', U'Î', U'Ï',
    U'Ó', U'Ò', U'Ô', U'Ö', U'Ú', U'Ù', U'Û', U'Ü', U'Ñ', U'ñ', U'Ç', U'ç', U'ß', U'¡', U'¿',
};

// The text's letters, or -1 when one is not the game's (a control character, a letter the font
// lacks, a sequence longer than two bytes).
int game_letters(const char* utf8) {
    int count = 0;
    for (std::size_t i = 0; utf8[i] != 0; ++count) {
        const auto c = static_cast<unsigned char>(utf8[i]);
        if (c < 0x80) {
            if (c < 0x20 || c == 0x7F) {
                return -1;
            }
            ++i;
            continue;
        }
        if ((c & 0xE0) != 0xC0 || utf8[i + 1] == 0) {
            return -1;
        }
        const char32_t letter = ((c & 0x1Fu) << 6) | (static_cast<unsigned char>(utf8[i + 1]) & 0x3Fu);
        bool known = false;
        for (const char32_t l : kGameLetters) {
            known |= l == letter;
        }
        if (!known) {
            return -1;
        }
        i += 2;
    }
    return count;
}

}  // namespace

int main() {
    for (int t = 0; t < kTextCount; ++t) {
        for (int lang = 0; lang < 5; ++lang) {
            const char* text = kWords[t].text[lang];
            CHECK(text != nullptr && text[0] != 0);
            if (text == nullptr) {
                continue;
            }
            const int letters = game_letters(text);
            CHECK(letters > 0);
            CHECK(letters <= static_cast<int>(kLongest[t]));
            if (letters <= 0 || letters > static_cast<int>(kLongest[t])) {
                std::fprintf(stderr, "text %d, language %d: \"%s\"\n", t, lang, text);
            }
        }
    }
    // Translated: each language's own, but for "No" in Spanish and Italian, which is English's.
    for (int t = 0; t < kTextCount; ++t) {
        for (int lang = 1; lang < 5; ++lang) {
            const bool same = std::strcmp(kWords[t].text[lang], kWords[t].text[0]) == 0;
            CHECK(!same || (t == kNo && (lang == 3 || lang == 4)));
        }
    }
    return openrac::test::result();
}
