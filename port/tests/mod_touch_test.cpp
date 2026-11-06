// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the mods ask of the on-screen pad's buttons (game/common/mod_touch.h; docs/port/MODS.md,
// "Touch controls"): sets, icons, a pushed context and hints, and a manifest's touch file.

#include <filesystem>
#include <fstream>
#include <string>

#include "check.h"
#include "mod_touch.h"

using namespace openrac::mods;

namespace {

constexpr const char* kSet =
    R"({"contexts": {"kart": {"actions": [{"id": "go", "icon": "kart", "press": ["cross"]}]}}})";
constexpr const char* kSvg =
    R"(<svg viewBox="0 0 10 10"><rect width="10" height="10" fill="#fff"/></svg>)";

void write(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

}  // namespace

int main() {
    std::string error;
    // A set and an icon; what is not one is refused and changes nothing.
    const unsigned start = touch::snapshot().generation;
    CHECK(touch::define("a", kSet, error));
    CHECK(!touch::define("a", "{}", error) && !error.empty());
    CHECK(!touch::define("a", "not json", error));
    CHECK(touch::icon("a", "kart", kSvg, error));
    CHECK(!touch::icon("a", "", kSvg, error));
    CHECK(!touch::icon("a", "x", "text", error));
    touch::Snapshot s = touch::snapshot();
    CHECK(s.generation == start + 2);
    CHECK(s.sets.size() == 1 && s.sets[0].first == "a");
    CHECK(s.icons.size() == 1 && s.icons[0].first == "kart");
    // Asked again with nothing changed: no copies of the sets and icons.
    s = touch::snapshot(s.generation);
    CHECK(!s.files && s.sets.empty() && s.icons.empty());

    // The last mod's pushed context shows; "" gives it back.
    touch::push("a", "kart");
    touch::push("b", "plane");
    CHECK(touch::snapshot().context == "plane");
    touch::push("b", "");
    CHECK(touch::snapshot().context == "kart");

    // Hints: every mod's, once each.
    touch::hint("a", "near_kart", true);
    touch::hint("b", "near_kart", true);
    touch::hint("a", "ledge", true);
    s = touch::snapshot();
    CHECK(s.hints.size() == 2);
    touch::hint("a", "ledge", false);
    CHECK(touch::snapshot().hints.size() == 1);

    // A mod forgotten takes its sets, icons, context and hints with it.
    touch::forget("a");
    s = touch::snapshot();
    CHECK(s.sets.empty() && s.icons.empty() && s.context.empty());
    CHECK(s.hints.size() == 1 && s.hints[0] == "near_kart");
    touch::clear();
    s = touch::snapshot();
    CHECK(s.sets.empty() && s.icons.empty() && s.hints.empty());

    // A manifest's touch file and the icons it names; one icon missing is reported, the rest taken.
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "openrac_mod_touch_test";
    std::filesystem::remove_all(root);
    write(
        root / "touch/touch.json",
        R"({"contexts": {"kart": {"actions": []}}, "icons": {"kart": "touch/kart.svg", "gone": "touch/gone.svg"}})"
    );
    write(root / "touch/kart.svg", kSvg);
    CHECK(touch::load_file("kart", root, "touch/touch.json", error));
    CHECK(error.find("gone") != std::string::npos);
    s = touch::snapshot();
    CHECK(s.sets.size() == 1 && s.icons.size() == 1 && s.icons[0].first == "kart");
    CHECK(!touch::load_file("kart", root, "../touch.json", error));
    CHECK(!touch::load_file("kart", root, "missing.json", error));
    touch::clear();
    std::filesystem::remove_all(root);
    return openrac::test::result();
}
