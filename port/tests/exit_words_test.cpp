// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The port's Exit texts (game/common/exit_words.h) in each of the game's five languages: each one
// there, in letters the game's font has (the list is options_menu.cpp's game_text, which drops any
// other), no longer than its box (exit_words.h, kLongest), and translated where the word differs.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "check.h"
#include "game/common/exit_words.h"

namespace {

using namespace openrac::frontend::exit_menu;

constexpr int kLanguages = 5;

// The accented letters the game's font has (options_menu.cpp, game_text).
constexpr char32_t kGameLetters[] = {
    U'á', U'à', U'â', U'ä', U'é', U'è', U'ê', U'ë', U'í', U'ì', U'î', U'ï', U'ó', U'ò', U'ô', U'ö',
    U'ú', U'ù', U'û', U'ü', U'Á', U'À', U'Â', U'Ä', U'É', U'È', U'Ê', U'Ë', U'Í', U'Ì', U'Î', U'Ï',
    U'Ó', U'Ò', U'Ô', U'Ö', U'Ú', U'Ù', U'Û', U'Ü', U'Ñ', U'ñ', U'Ç', U'ç', U'ß', U'¡', U'¿',
};

// Returns whether `letter` is one of the accented letters the game's font has.
bool in_font(char32_t letter) {
    return std::ranges::find(kGameLetters, letter) != std::end(kGameLetters);
}

// Returns the letters of UTF-8 `utf8` as the game counts them (one byte each), or -1 when one is
// not the game's: a control character, a letter the font lacks, a sequence of three bytes or more.
int game_letters(const char* utf8) {
    int count = 0;

    for (std::size_t i = 0; utf8[i] != 0; ++count) {
        const auto c = static_cast<unsigned char>(utf8[i]);

        // ASCII: the font's own, but for the control characters.
        if (c < 0x80) {
            // A control character: the game's are its pad glyphs and line breaks, not text.
            if (c < 0x20 || c == 0x7F) {
                return -1;
            }

            ++i;
            continue;
        }

        // Not a two-byte sequence: no letter the font has.
        if ((c & 0xE0) != 0xC0 || utf8[i + 1] == 0) {
            return -1;
        }

        const char32_t letter =
            ((c & 0x1Fu) << 6) | (static_cast<unsigned char>(utf8[i + 1]) & 0x3Fu);

        // An accented letter the font lacks.
        if (!in_font(letter)) {
            return -1;
        }

        i += 2;
    }

    return count;
}

// Every text has words in every language.
void test_every_language_has_each_text() {
    for (int t = 0; t < kTextCount; ++t) {
        for (int lang = 0; lang < kLanguages; ++lang) {
            const char* text = kWords[t].text[lang];
            CHECK(text != nullptr && text[0] != 0);
        }
    }
}

// Every text is in the game's letters and fits its box.
void test_each_text_fits_the_game() {
    for (int t = 0; t < kTextCount; ++t) {
        for (int lang = 0; lang < kLanguages; ++lang) {
            const int letters = game_letters(kWords[t].text[lang]);
            const bool fits = letters > 0 && letters <= static_cast<int>(kLongest[t]);
            CHECK(fits);

            // Says which one, for the log of a failed run.
            if (!fits) {
                std::fprintf(
                    stderr, "text %d, language %d: \"%s\"\n", t, lang, kWords[t].text[lang]
                );
            }
        }
    }
}

// Each language has its own words: only "No" is English's in Spanish and Italian.
void test_each_text_is_translated() {
    constexpr int kSpanish = 3;
    constexpr int kItalian = 4;

    for (int t = 0; t < kTextCount; ++t) {
        for (int lang = 1; lang < kLanguages; ++lang) {
            const bool same = std::strcmp(kWords[t].text[lang], kWords[t].text[0]) == 0;
            CHECK(!same || (t == kNo && (lang == kSpanish || lang == kItalian)));
        }
    }
}

}  // namespace

int main() {
    test_every_language_has_each_text();
    test_each_text_fits_the_game();
    test_each_text_is_translated();
    return openrac::test::result();
}
