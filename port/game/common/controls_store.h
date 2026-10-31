// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The player's controls on disk: the global file, which every game reads, and the running game's
// own, which it reads over the global one (platform/bindings.h's layers; docs/port/CONTROLS.md):
//
//   <data home>/openrac/controls/global.ini
//   <data home>/openrac/controls/<game>.ini     rac1.ini serves both rac1-pal and rac1-ntsc
//
// The store reads both, compiles them over the port's defaults, and hands the bindings to whoever
// presses the pad (the `apply` given to open: Input, through the frontend). A change made here
// (the controls window) is written at once and applied; a change made to a file beside it (the
// launcher, an editor) is seen by poll() within a second and applied too. Writing goes through a
// .new file and a rename, as the settings file's does, and keeps every line it does not change.

#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "platform/bindings.h"
#include "platform/controls_file.h"
#include "platform/input.h"
#include "platform/players.h"

namespace openrac::controls {

// Which file a change goes to.
enum class Scope {
    Global,
    Game,
};

class Store {
public:
    using PlayerBindings = std::array<platform::Bindings, platform::kPlayers>;
    using Apply = std::function<void(const PlayerBindings&, const platform::Players&)>;

    // Reads the controls of `game` ("rac1") in `dir`, and applies them. A missing file is an empty
    // one, made when something is first written to it.
    void open(const std::filesystem::path& dir, std::string_view game, Apply apply);

    // Reads the files again if either changed on disk since they were last read or written, at
    // most once a second of `now` (seconds, any clock that goes forward). Returns whether they did.
    bool poll(double now);

    // Moves bindings the game's settings file held (bind_* keys, written before the controls had
    // files of their own) into the game's file, those of its keys the file does not already have.
    // Returns how many moved.
    int adopt(const platform::BindingValues& old);

    // What a player's pad is pressed from now (`player` 0 or 1), or what every player's is before
    // any key of one player's own (-1), and where each of it came from.
    const platform::Bindings& bindings(int player = -1) const {
        return player < 0 ? m_shared : m_player[static_cast<std::size_t>(player)];
    }

    const platform::Origins& origins(int player = -1) const {
        return player < 0 ? m_shared_origins : m_player_origins[static_cast<std::size_t>(player)];
    }

    // Which controller each player reads: the global file's player1 and player2, the defaults
    // where it has none. A game's file does not choose them.
    const platform::Players& players() const { return m_players; }

    // Remembers the controllers connected now in devices.ini beside the controls, by id and name,
    // with those seen before: the launcher offers them to the players by name.
    void remember(const std::vector<platform::Device>& devices);

    const platform::ControlsFile& file(Scope scope) const {
        return scope == Scope::Global ? m_global : m_game;
    }

    std::filesystem::path path(Scope scope) const;

    const std::string& game() const { return m_game_id; }

    // Sets `key` in `scope`'s file to `value`, writes it and applies. False when the file is a
    // newer build's (read only) or cannot be written.
    bool set(Scope scope, std::string_view key, std::string_view value);

    // Removes `key` from `scope`'s file, so the layer below shows through; writes and applies.
    bool erase(Scope scope, std::string_view key);

    // Puts `scope` back to what it inherits. The game's: its file goes. The global: its bindings
    // and controller settings go (bind_*, pad_*, and each player's own), the rest of the file (the
    // players' controllers) stays.
    bool reset(Scope scope);

    // Removes one player's own bindings and settings from `scope`'s file (p1.*, p2.*), so the
    // player reads what every player reads.
    bool reset_player(Scope scope, int player);

private:
    platform::ControlsFile& file_of(Scope scope) {
        return scope == Scope::Global ? m_global : m_game;
    }

    bool write(Scope scope);
    void read();
    void compile();

    std::filesystem::path m_dir;
    std::string m_game_id;
    Apply m_apply;
    platform::ControlsFile m_global;
    platform::ControlsFile m_game;
    std::filesystem::file_time_type m_global_time{};
    std::filesystem::file_time_type m_game_time{};
    double m_polled = -1.0;
    platform::Bindings m_shared = platform::default_bindings();
    platform::Origins m_shared_origins;
    PlayerBindings m_player{platform::default_bindings(), platform::default_bindings()};
    std::array<platform::Origins, platform::kPlayers> m_player_origins;
    platform::Players m_players = platform::default_players();
};

}  // namespace openrac::controls
