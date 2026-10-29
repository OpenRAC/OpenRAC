// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/players.h"

#include <algorithm>

namespace openrac::platform {
namespace {

std::string_view trimmed(std::string_view s) {
    while (!s.empty() && s.front() == ' ') {
        s.remove_prefix(1);
    }
    while (!s.empty() && s.back() == ' ') {
        s.remove_suffix(1);
    }
    return s;
}

bool is_controller(std::string_view name) {
    return name != "keyboard" && name != "auto" && name != "none";
}

}  // namespace

Players default_players() {
    Players players;
    players.wants[0] = {"keyboard", "auto"};
    players.wants[1] = {"auto"};
    return players;
}

std::string player_key(int player) {
    return "player" + std::to_string(player + 1);
}

std::vector<std::string> read_player(std::string_view value) {
    std::vector<std::string> names;
    for (std::size_t at = 0; at <= value.size();) {
        const std::size_t comma = value.find(',', at);
        const std::size_t end = comma == std::string_view::npos ? value.size() : comma;
        const std::string_view name = trimmed(value.substr(at, end - at));
        if (!name.empty() && name != "none"
            && std::find(names.begin(), names.end(), name) == names.end()) {
            names.emplace_back(name);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        at = comma + 1;
    }
    return names;
}

std::string player_value(const std::vector<std::string>& names) {
    std::string out;
    for (const std::string& name : names) {
        out += out.empty() ? name : ", " + name;
    }
    return out.empty() ? std::string("none") : out;
}

Assignment assign(const Players& players, std::span<const std::string> connected) {
    Assignment out;
    std::vector<bool> taken(connected.size(), false);
    // The keyboard is the first player's that names it.
    for (int p = 0; p < kPlayers && out.keyboard < 0; ++p) {
        const auto& wants = players.wants[static_cast<std::size_t>(p)];
        if (std::find(wants.begin(), wants.end(), "keyboard") != wants.end()) {
            out.keyboard = p;
        }
    }
    // Named controllers first, so a player's own controller is theirs whatever order it came in.
    for (int p = 0; p < kPlayers; ++p) {
        for (const std::string& name : players.wants[static_cast<std::size_t>(p)]) {
            if (!is_controller(name)) {
                continue;
            }
            const auto it = std::find(connected.begin(), connected.end(), name);
            const auto index = static_cast<std::size_t>(it - connected.begin());
            if (it != connected.end() && !taken[index]) {
                out.pad[static_cast<std::size_t>(p)] = static_cast<int>(index);
                taken[index] = true;
                break;
            }
        }
    }
    // Then "auto": the first controller left, in connection order.
    for (int p = 0; p < kPlayers; ++p) {
        const auto& wants = players.wants[static_cast<std::size_t>(p)];
        if (out.pad[static_cast<std::size_t>(p)] >= 0
            || std::find(wants.begin(), wants.end(), "auto") == wants.end()) {
            continue;
        }
        const auto free = std::find(taken.begin(), taken.end(), false);
        if (free != taken.end()) {
            *free = true;
            out.pad[static_cast<std::size_t>(p)] = static_cast<int>(free - taken.begin());
        }
    }
    return out;
}

std::string controller_id(
    std::string_view guid, std::string_view serial, std::span<const std::string> taken
) {
    std::string base(guid);
    if (!serial.empty()) {
        base += "-";
        base += serial;
    }
    if (std::find(taken.begin(), taken.end(), base) == taken.end()) {
        return base;
    }
    // Another of the same model and no serial to tell them apart: the next number free.
    for (int n = 2;; ++n) {
        std::string id = base + "#" + std::to_string(n);
        if (std::find(taken.begin(), taken.end(), id) == taken.end()) {
            return id;
        }
    }
}

}  // namespace openrac::platform
