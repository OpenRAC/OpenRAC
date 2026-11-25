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

// A game function by its name. Going Commando's functions are named by address, so a hook point's
// name finds its function too.
void* api_function(const char* name) {
    if (openrac_host_fn fn = openrac_guest_function_named(name)) {
        return reinterpret_cast<void*>(fn);
    }
    const HookPoint* p = find_hook_point(name);
    return p != nullptr && p->symbol != nullptr
               ? reinterpret_cast<void*>(openrac_guest_function_named(p->symbol))
               : nullptr;
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

// ---- What mods offer each other (interface 1.40) ----
//
// A service is a table of a mod's own, under a name that starts with its id, handed to the mods
// that ask for it by name and version; the port keeps only the pointer. Services are offered during
// a mod's load and stay until the mods' reset (the program's end); a mod whose load fails takes its
// own with it (undo) before its library closes, and no other mod can have asked for them by then:
// none runs during another's load. on_mods_loaded tells each mod that asked once every mod has
// loaded, the first moment every service there will be is there.

namespace {

struct Service {
    std::string name;
    std::uint32_t version;
    const void* table;
    std::uint32_t size;
    const openrac_mod* mod;
};

std::vector<Service> g_services;
std::vector<FrameCallback> g_mods_loaded;

// `name` is `<id>/` and then one or more of a-z 0-9 - _ . /, at most OPENRAC_SERVICE_NAME_MAX.
bool service_name_of(const openrac_mod& mod, std::string_view name) {
    if (name.size() > OPENRAC_SERVICE_NAME_MAX || name.size() <= mod.id.size() + 1
        || !name.starts_with(mod.id) || name[mod.id.size()] != '/') {
        return false;
    }
    return std::all_of(
        name.begin() + static_cast<std::ptrdiff_t>(mod.id.size()) + 1, name.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'
                   || c == '.' || c == '/';
        }
    );
}

}  // namespace

int api_on_mods_loaded(openrac_mod* mod, openrac_frame_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_E_TOO_LATE;
    }
    g_mods_loaded.push_back({fn, user, mod});
    return OPENRAC_OK;
}

int api_service_provide(
    openrac_mod* mod, const char* name, std::uint32_t version, const void* table, std::uint32_t size
) {
    if (mod == nullptr || mod != g_loading) {
        return OPENRAC_E_TOO_LATE;
    }
    if (name == nullptr || table == nullptr || size == 0 || version == 0
        || !service_name_of(*mod, name)) {
        log::warn(
            "mod {}: service \"{}\" refused: a name of the form <its id>/<name>, a table, a "
            "size and a version are needed",
            mod->id,
            name != nullptr ? name : ""
        );
        return OPENRAC_E_UNKNOWN;
    }
    if (std::any_of(g_services.begin(), g_services.end(), [&](const Service& s) {
            return s.name == name;
        })) {
        log::warn("mod {}: service {} is offered already: the first stays", mod->id, name);
        return OPENRAC_E_TAKEN;
    }
    g_services.push_back({name, version, table, size, mod});
    log::info("mod {} offers service {} {}", mod->id, name, version);
    return OPENRAC_OK;
}

int api_service_find(
    openrac_mod* mod, const char* name, std::uint32_t min_version, openrac_service* out
) {
    static_cast<void>(mod);
    if (out == nullptr || out->size < sizeof(openrac_service)) {
        return OPENRAC_E_UNKNOWN;
    }
    const std::uint32_t size = out->size;
    *out = openrac_service{};
    out->size = size;
    if (name == nullptr) {
        return OPENRAC_E_UNKNOWN;
    }
    const auto found = std::find_if(g_services.begin(), g_services.end(), [&](const Service& s) {
        return s.name == name;
    });
    if (found == g_services.end()) {
        return OPENRAC_E_UNKNOWN;
    }
    out->version = found->version;
    out->provider = found->mod->id.c_str();
    if (found->version < min_version) {
        return OPENRAC_E_SIGNATURE;
    }
    out->table_size = found->size;
    out->table = found->table;
    return OPENRAC_OK;
}

void drop_services(const openrac_mod* mod) {
    std::erase_if(g_services, [&](const Service& s) { return mod == nullptr || s.mod == mod; });
    std::erase_if(g_mods_loaded, [&](const FrameCallback& c) {
        return mod == nullptr || c.mod == mod;
    });
}

void run_mods_loaded() {
    // No load runs now, so no callback can add to the list (on_mods_loaded: too late).
    for (const FrameCallback& c : g_mods_loaded) {
        c.fn(c.user);
    }
}

}  // namespace openrac::mods::detail
