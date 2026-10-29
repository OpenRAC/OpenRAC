// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "controls_store.h"

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

#include "common/log.h"

namespace openrac::controls {
namespace {

namespace fs = std::filesystem;

// The file's text, or empty for one that is missing or cannot be read.
std::string read_text(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

// When the file was last written, or the epoch for one that is missing.
fs::file_time_type written_at(const fs::path& path) {
    std::error_code error;
    const fs::file_time_type at = fs::last_write_time(path, error);
    return error ? fs::file_time_type{} : at;
}

}  // namespace

void Store::open(const fs::path& dir, std::string_view game, Apply apply) {
    m_dir = dir;
    m_game_id = game;
    m_apply = std::move(apply);
    read();
    if (m_global.read_only() || m_game.read_only()) {
        log::warn(
            "controls: a file in {} was written by a newer build; it is read, never written",
            dir.string()
        );
    }
}

fs::path Store::path(Scope scope) const {
    return m_dir / (scope == Scope::Global ? std::string("global.ini") : m_game_id + ".ini");
}

void Store::read() {
    m_global = platform::ControlsFile::parse(read_text(path(Scope::Global)));
    m_game = platform::ControlsFile::parse(read_text(path(Scope::Game)));
    m_global_time = written_at(path(Scope::Global));
    m_game_time = written_at(path(Scope::Game));
    compile();
}

void Store::compile() {
    const platform::BindingValues global = m_global.values();
    const platform::BindingValues game = m_game.values();
    m_bindings = platform::compile_layers(&global, &game, &m_origins);
    m_players = platform::default_players();
    for (int p = 0; p < platform::kPlayers; ++p) {
        if (const auto value = m_global.get(platform::player_key(p))) {
            m_players.wants[static_cast<std::size_t>(p)] = platform::read_player(*value);
        }
    }
    if (m_apply) {
        m_apply(m_bindings, m_players);
    }
}

bool Store::poll(double now) {
    if (m_dir.empty() || (m_polled >= 0.0 && now - m_polled < 1.0)) {
        return false;
    }
    m_polled = now;
    if (written_at(path(Scope::Global)) == m_global_time
        && written_at(path(Scope::Game)) == m_game_time) {
        return false;
    }
    log::info("controls: {} changed on disk; read again", m_dir.string());
    read();
    return true;
}

bool Store::write(Scope scope) {
    const fs::path target = path(scope);
    std::error_code error;
    fs::create_directories(m_dir, error);
    fs::path fresh = target;
    fresh += ".new";
    {
        std::ofstream out(fresh, std::ios::binary | std::ios::trunc);
        out << file_of(scope).text();
        if (!out) {
            log::warn("controls: {} cannot be written", fresh.string());
            return false;
        }
    }
    fs::rename(fresh, target, error);
    if (error) {
        log::warn("controls: {} cannot be replaced: {}", target.string(), error.message());
        return false;
    }
    (scope == Scope::Global ? m_global_time : m_game_time) = written_at(target);
    return true;
}

bool Store::set(Scope scope, std::string_view key, std::string_view value) {
    platform::ControlsFile& file = file_of(scope);
    if (file.read_only()) {
        return false;
    }
    if (file.get(key) == std::string(value)) {
        return true;
    }
    file.set(key, value);
    const bool written = write(scope);
    compile();
    return written;
}

bool Store::erase(Scope scope, std::string_view key) {
    platform::ControlsFile& file = file_of(scope);
    if (file.read_only() || !file.erase(key)) {
        return false;
    }
    const bool written = write(scope);
    compile();
    return written;
}

bool Store::reset(Scope scope) {
    platform::ControlsFile& file = file_of(scope);
    if (file.read_only()) {
        return false;
    }
    bool done = true;
    if (scope == Scope::Game) {
        std::error_code error;
        fs::remove(path(Scope::Game), error);
        done = !error;
        m_game = {};
        m_game_time = {};
    } else {
        file.erase_prefix(platform::kBindPrefix);
        file.erase_prefix(platform::kTuningPrefix);
        done = write(scope);
    }
    compile();
    return done;
}

void Store::remember(const std::vector<platform::Device>& devices) {
    if (m_dir.empty() || devices.empty()) {
        return;
    }
    const fs::path path = m_dir / "devices.ini";
    platform::ControlsFile file = platform::ControlsFile::parse(read_text(path));
    bool changed = false;
    for (const platform::Device& d : devices) {
        if (file.get(d.id) != d.name) {
            file.set(d.id, d.name);
            changed = true;
        }
    }
    if (!changed || file.read_only()) {
        return;
    }
    std::error_code error;
    fs::create_directories(m_dir, error);
    fs::path fresh = path;
    fresh += ".new";
    {
        std::ofstream out(fresh, std::ios::binary | std::ios::trunc);
        out << file.text();
    }
    fs::rename(fresh, path, error);
}

int Store::adopt(const platform::BindingValues& old) {
    if (m_game.read_only()) {
        return 0;
    }
    int moved = 0;
    for (const auto& [key, value] : old) {
        if (key.starts_with(platform::kBindPrefix) && !m_game.get(key)) {
            m_game.set(key, value);
            ++moved;
        }
    }
    if (moved > 0) {
        write(Scope::Game);
        compile();
    }
    return moved;
}

}  // namespace openrac::controls
