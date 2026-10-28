// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game program's command line and default paths (program/options.h): the parser, the usage
// it prints, and where a player's data lives when no option says (the folders the launcher uses).

#include "program/options.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include "common/env.h"
#include "common/json.h"
#include "common/log.h"
#include "openrac/game_host.h"

namespace openrac::program {
namespace {

// Logs `why` and the usage line, and ends the program with exit code 64 (EX_USAGE, the usage
// error of BSD's sysexits.h).
[[noreturn]] void usage(const std::string& why) {
    log::error("{}", why);
    log::error(
        "usage: {} --data <install>/active/{}/data [--cards DIR] [--frames N] [--stop-on-missing] "
        "[--level N] [--skip-movies] [--mods DIR] [--map] [--ship-map] [--destinations] "
        "[--settings FILE] "
        "[--plugins FILE] [--probes DIR] [--no-mods] [--debug-overlay] [--debug-dump FILE]",
        program_name(),
        openrac_game.game
    );
    std::exit(64);
}

// OpenRAC's folder in the player's data home, as the launcher finds it
// (launcher/core/src/saves.rs, data_home).
fs::path data_home() {
#if defined(__APPLE__)
    const std::optional<std::string> home = env::system_variable("HOME");
    fs::path base = fs::path(home.value_or(".")) / "Library/Application Support/OpenRAC";
#elif defined(_WIN32)
    // %LOCALAPPDATA%\openrac: the same folder whatever folder the game is started in.
    fs::path base;

    if (const std::optional<std::string> local = env::system_variable("LOCALAPPDATA");
        local && !local->empty()) {
        base = fs::path(*local) / "openrac";
    } else {
        // No LOCALAPPDATA (a stripped environment): the same folder under the user's profile.
        const std::optional<std::string> profile = env::system_variable("USERPROFILE");
        base = fs::path(profile.value_or(".")) / "AppData/Local/openrac";
    }
#else
    fs::path base;

    if (const std::optional<std::string> xdg = env::system_variable("XDG_DATA_HOME");
        xdg && !xdg->empty()) {
        base = fs::path(*xdg) / "openrac";
    } else {
        const std::optional<std::string> home = env::system_variable("HOME");
        base = fs::path(home.value_or(".")) / ".local/share/openrac";
    }
#endif
    return base;
}

#if defined(_WIN32)
// Moves the cards and settings of the folder Windows used before %LOCALAPPDATA% into `base`, the
// data home. That was the other systems' folder, HOME's .local/share/openrac, and with no HOME
// (the usual case there) the one under the folder the game was started in, which the launcher's
// Play does not keep the same. Copied without replacing what `base` has, then the old folder is
// renamed, so they come over once.
void move_old_data_home(const fs::path& base) {
    const std::optional<std::string> home = env::system_variable("HOME");
    const fs::path old = fs::path(home.value_or(".")) / ".local/share/openrac";
    std::error_code error;

    // Nothing to move: no old folder, or HOME points where the data home is.
    if (!fs::is_directory(old, error) || fs::equivalent(old, base, error)) {
        return;
    }

    fs::create_directories(base, error);
    fs::copy(old, base, fs::copy_options::recursive | fs::copy_options::skip_existing, error);

    // A copy that stopped part way leaves the old folder in place, to be tried at the next start.
    if (error) {
        log::warn(
            "the cards and settings in {} could not be copied to {}: {}",
            old.string(),
            base.string(),
            error.message()
        );
        return;
    }

    const fs::path moved = fs::path(old).concat(".moved");
    fs::rename(old, moved, error);

    // Kept under its name, the old folder is copied again next start, the new files left alone.
    if (error) {
        log::warn(
            "the cards and settings in {} are also in {}, but {} cannot be renamed {}: {}",
            old.string(),
            base.string(),
            old.string(),
            moved.filename().string(),
            error.message()
        );
        return;
    }

    log::info("the cards and settings in {} are now in {}", old.string(), base.string());
}
#endif

// Where the first memory card lives unless --cards says otherwise: the folder
// the launcher's save manager reads and backs up: <data home>/openrac/memcard/<serial>.
fs::path default_cards() {
    return data_home() / "memcard" / openrac_game.serial;
}

// The player's settings unless --settings says otherwise: <data home>/openrac/settings/<id>.ini.
fs::path default_settings() {
    return data_home() / "settings" / (std::string(openrac_game.id) + ".ini");
}

}  // namespace

std::string program_name() {
    return std::string("openrac-") + openrac_game.id;
}

std::string game_version() {
    const std::string id = openrac_game.id;
    const auto dash = id.find('-');
    return dash == std::string::npos ? id : id.substr(dash + 1);
}

fs::path levels_or_flat(const fs::path& own) {
    std::error_code ec;
    const fs::path flat = own.parent_path();

    if (game_version() == "pal" && own.filename() == "pal"
        && !fs::exists(own / "level_99" / "manifest.json", ec)
        && fs::exists(flat / "level_99" / "manifest.json", ec)) {
        log::warn(
            "levels: reading the ones extracted before each version had a folder, in {} (extract "
            "again to "
            "move them into {})",
            flat.string(),
            own.string()
        );
        return flat;
    }

    return own;
}

fs::path default_levels(const fs::path& data) {
    return levels_or_flat(data / "port" / game_version());
}

Options parse(int argc, char** argv) {
    Options o;

    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                usage("missing value for " + a);
            }

            return argv[++i];
        };

        if (a == "--data") {
            o.data = value();
        } else if (a == "--cards") {
            o.cards = value();
        } else if (a == "--no-card") {
            openrac_game_no_card = 1;
        } else if (a == "--window") {
            o.window = true;
        } else if (a == "--mods") {
            o.mods = value();
        } else if (a == "--map") {
            o.expect_map = true;
        } else if (a == "--settings") {
            o.settings = value();
        } else if (a == "--plugins") {
            o.plugins = value();
        } else if (a == "--probes") {
            o.probes = value();
        } else if (a == "--no-mods") {
            o.no_mods = true;
        } else if (a == "--levels") {
            o.levels = value();
        } else if (a == "--frames") {
            o.frames = std::stol(value());
        } else if (a == "--level") {
            openrac_game_start_level = std::stoi(value());
        } else if (a == "--skip-movies") {
            o.skip_movies = true;
        } else if (a == "--keep-going") {
            o.keep_going = true;
        } else if (a == "--stop-on-missing") {
            o.keep_going = false;
        } else if (a == "--debug-dump") {
            o.debug_dump = value();
        } else if (a == "--debug-overlay") {
            o.debug_overlay = true;
        } else if (a == "--ship-map") {
            o.ship_map = true;
        } else if (a == "--destinations") {
            o.expect_destinations = true;
        } else if (a == "--check-levels") {
            o.check_levels = true;
        } else if (a == "--help" || a == "-h") {
            usage(program_name() + ": " + openrac_game.title + ", native");
        } else {
            usage("unknown option " + a);
        }
    }

    if (o.data.empty()) {
        if (const std::optional<std::string_view> data = env::value(env::Var::Data)) {
            o.data = *data;
        } else {
            usage(
                std::string("no data folder: pass --data, the folder the extractor wrote for ")
                + openrac_game.game
            );
        }
    }

#if defined(_WIN32)
    // The defaults below are in the data home: what an older build kept elsewhere comes over first.
    if (o.cards.empty() || o.settings.empty()) {
        move_old_data_home(data_home());
    }

#endif
    if (o.cards.empty()) {
        o.cards = default_cards();
    }

    if (o.settings.empty()) {
        o.settings = default_settings();
    }

    if (o.plugins.empty()) {
        o.plugins = o.data / "plugins" / "plugins.json";
    }

    if (o.no_mods) {
        o.mods.clear();
    } else if (!o.mods.empty() && fs::exists(o.mods / "map.json")) {
        const fs::path path = o.mods / "map.json";
        std::ifstream file(path, std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(file)), {});
        CustomMap map;
        std::string error;

        if (!file || !parse_custom_map(text, openrac_game.game, game_version(), map, error)) {
            usage(path.string() + ": " + (error.empty() ? "cannot be read" : error));
        }

        o.custom_map = std::move(map);
    }

    if (o.expect_map && !o.custom_map) {
        usage("--map requires an active map pack in --mods (with mods enabled)");
    }
    if (!o.no_mods && !o.mods.empty() && fs::exists(o.mods / "destinations.json")) {
        if (o.custom_map) {
            usage("Standalone maps cannot be mixed with ship destinations");
        }
        std::ifstream file(o.mods / "destinations.json", std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(file)), {});
        std::string error;
        if (!file
            || !parse_destinations(
                text, openrac_game.game, game_version(), o.destinations, error
            )) {
            usage(error.empty() ? "Cannot read destinations.json" : error);
        }
        for (const auto& map : o.destinations) {
            if (!fs::is_regular_file(o.mods / "destinations" / map.id / "patches.tsv")) {
                usage("Destination mount is missing: " + map.id + "; prepare the mods again");
            }
        }
    }
    if (o.expect_destinations && o.destinations.empty()) {
        usage("--destinations requires prepared destination packs with mods enabled");
    }
    if (o.ship_map
        && (!o.custom_map || game_version() != "pal"
            || std::string_view(openrac_game.game) != "rac1")) {
        usage("--ship-map requires a RAC1 PAL map pack in --mods");
    }
    if (o.ship_map) {
        json::Value stack;
        std::string error;
        if (!json::parse_file((o.mods / "stack.json").string(), stack, error)
            || !stack["layers"].is_array() || stack["layers"].items().size() != 1
            || stack["layers"].items()[0]["id"].string() != o.custom_map->id) {
            usage("--ship-map needs a separate stack containing only the selected map pack");
        }
    }

    return o;
}

std::string command_line(int argc, char** argv) {
    std::string line;

    for (int i = 0; i < argc; ++i) {
        std::string a = i == 0 ? program_name() : std::string(argv[i]);

        if (a.find_first_of(" \"'") != std::string::npos || a.empty()) {
            a = "\"" + a + "\"";
        }

        line += (i == 0 ? "" : " ") + a;
    }

    return line;
}

}  // namespace openrac::program
