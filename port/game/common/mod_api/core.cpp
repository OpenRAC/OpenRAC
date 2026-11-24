// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "core": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

void api_log(openrac_mod* mod, int level, const char* text) {
    const std::string_view id = mod != nullptr ? std::string_view(mod->id) : std::string_view("?");
    const char* t = text != nullptr ? text : "";
    switch (level) {
        case OPENRAC_LOG_DEBUG:
            log::debug("[mod {}] {}", id, t);
            break;
        case OPENRAC_LOG_WARN:
            log::warn("[mod {}] {}", id, t);
            break;
        case OPENRAC_LOG_ERROR:
            log::error("[mod {}] {}", id, t);
            break;
        default:
            log::info("[mod {}] {}", id, t);
            break;
    }
}

void api_fail(openrac_mod* mod, const char* reason) {
    if (mod != nullptr && reason != nullptr) {
        mod->failure = reason;
    }
}

// openrac_api.game_data_dir (interface 1.33): the data folder of an installed game, as game_data_folder
// finds it. A game the manifest does not declare (reads_games) is refused; the running game is not.
int api_game_data_dir(openrac_mod* mod, const char* game, char* out, std::uint32_t size) {
    if (mod == nullptr || game == nullptr || out == nullptr || size == 0 || g_host == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::string_view id = game;
    if (!valid_game_id(id)) {
        return 0;
    }
    if (id != g_host->game
        && std::none_of(
            mod->manifest.reads_games.begin(),
            mod->manifest.reads_games.end(),
            [id](const std::string& g) { return g == id; }
        )) {
        return OPENRAC_HOOK_REFUSED;
    }
    const fs::path data = game_data_folder(*g_host, id);
    if (data.empty()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (!holds_game_data(data)) {
        return 0;
    }
    const std::u8string text = data.u8string();
    std::string path;
    path.assign(text.begin(), text.end());
    if (path.size() + 1 > size) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::memcpy(out, path.c_str(), path.size() + 1);
    return static_cast<int>(path.size());
}

// The mod's own folder (1.11): <mod data>/<its id>, made on the first ask. An id that is not a plain
// name (a separator, "." or "..") has none.
const char* api_data_dir(openrac_mod* mod) {
    if (mod == nullptr || g_host == nullptr || g_host->mod_data.empty() || mod->id.empty()
        || mod->id == "." || mod->id == ".."
        || mod->id.find_first_of("/\\:") != std::string::npos) {
        return nullptr;
    }
    if (mod->data_dir.empty()) {
        const std::filesystem::path dir = g_host->mod_data / mod->id;
        std::error_code e;
        std::filesystem::create_directories(dir, e);
        if (e || !std::filesystem::is_directory(dir, e)) {
            return nullptr;
        }
        const std::u8string path = dir.u8string();
        mod->data_dir.assign(path.begin(), path.end());
    }
    return mod->data_dir.c_str();
}

void* api_function(const char* name) {
    return reinterpret_cast<void*>(openrac_guest_function_named(name));
}

void* api_function_at(openrac_gaddr address) {
    return reinterpret_cast<void*>(openrac_guest_function(address));
}

openrac_gaddr api_relocate_data(openrac_gaddr address) {
    return OPENRAC_DATA(address);
}

openrac_gaddr api_relocate_level_data(int level, openrac_gaddr address) {
    return openrac_relocate_level_data(level, address);
}

int api_loaded_level(void) {
    return g_host != nullptr && g_host->loaded_level != nullptr ? g_host->loaded_level() : -1;
}

openrac_gaddr api_stack_push(uint32_t size, openrac_gaddr* saved) {
    if (saved != nullptr) {
        *saved = openrac_guest_sp;
    }
    return openrac_guest_frame(size);
}

void api_stack_pop(openrac_gaddr saved) {
    openrac_guest_sp = saved;
}

int api_plugins_folder(char* out, std::uint32_t size) {
    if (out == nullptr || size == 0 || g_host == nullptr) {
        return 0;
    }
    const std::string folder = g_host->list.parent_path().string();
    // A path cut short is another path: none rather than that.
    if (folder.empty() || folder.size() >= size) {
        return 0;
    }
    std::memcpy(out, folder.c_str(), folder.size() + 1);
    return static_cast<int>(folder.size());
}

const char* pack_folder() {
    return g_host != nullptr && g_host->pack_folder != nullptr ? g_host->pack_folder() : nullptr;
}

// The edited levels' pack this game reads (--mods, or the destination being played): its level_NN/
// folders hold what the editor packed, cutscenes among them.
int api_pack_folder(char* out, std::uint32_t size) {
    const char* pack = pack_folder();
    if (out == nullptr || size == 0 || pack == nullptr) {
        return 0;
    }
    const std::string folder = pack;
    if (folder.empty() || folder.size() >= size) {
        return 0;
    }
    std::memcpy(out, folder.c_str(), folder.size() + 1);
    return static_cast<int>(folder.size());
}

// The language the game shows its text in, as a code: "en", "fr", "de", "es", "it" (the PAL
// disc's five), "ja" for a Japanese disc.
int api_game_language(char* out, std::uint32_t size) {
    static constexpr const char* kCodes[] = {"en", "fr", "de", "es", "it"};
    if (out == nullptr || size < 3) {
        return 0;
    }
    const std::string_view version = g_host != nullptr ? std::string_view(g_host->version) : "";
    const int lang = g_host != nullptr && g_host->language != nullptr ? g_host->language() : 0;
    const char* code = "en";
    if (version.find("jp") != std::string_view::npos) {
        code = "ja";
    } else if (lang >= 0 && lang < 5) {
        code = kCodes[lang];
    }
    std::memcpy(out, code, 3);
    return 2;
}

}  // namespace openrac::mods::detail
