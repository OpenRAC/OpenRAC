// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The native mod loader (mods.h, docs/port/MODS.md). The table's calls are in mod_api/<area>.cpp.

#include "mods.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "common/build_info.h"
#include "common/c_boundary.h"
#include "common/crash.h"
#include "common/env.h"
#include "common/json.h"
#include "common/log.h"
#include "mod_api/internal.h"
#include "mod_library.h"
#include "mod_manifest.h"
#include "mod_play.h"
#include "mod_scripts.h"
#include "mod_touch.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/memory.h"
#include "openrac/openrac_mod.h"
namespace fs = std::filesystem;

/* The mods loaded for this run, for lib/mc.c's save mark (game_host.h): set by load() below. */
extern "C" const char* openrac_game_loaded_mods = "[]";
/* The mods' data kept with the saves (game_host.h): set by load() below, called by lib/mc.c. */
extern "C" void (*openrac_game_save_written)(const char* path, int players) = nullptr;
extern "C" void (*openrac_game_save_read)(const char* path, int slot) = nullptr;
extern "C" void (*openrac_game_new_game)(void) = nullptr;
using namespace openrac;

namespace {

// openrac_script_hook_id's body: the binding's aliases are sorted, so by halves.
const char* permanent_hook_id(const openrac_script_binding* binding, const char* name) {
    if (binding == nullptr || name == nullptr || binding->aliases == nullptr) {
        return name;
    }
    int low = 0;
    int high = binding->alias_count - 1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        const int c = std::strcmp(binding->aliases[mid].alias, name);
        if (c == 0) {
            return binding->aliases[mid].id;
        }
        if (c < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return name;
}

}  // namespace

// A hook point's permanent id from any of its names (openrac/script_binding.h). An exception is
// reported to the log and gives `name` back, the contract's answer for a name with no alias
// (CODING_CONVENTIONS.md, section 13.4).
extern "C" const char* openrac_script_hook_id(
    const openrac_script_binding* binding, const char* name
) {
    return c_boundary::call("openrac_script_hook_id", name, [&] {
        return permanent_hook_id(binding, name);
    });
}

namespace openrac::mods {

using namespace detail;

bool g_frame_callbacks = false;

namespace detail {

const Host* g_host = nullptr;

// A hook point's id from one of its names, through the game's binding (none: the name).
const char* hook_id(const Host& host, const char* name) {
    return openrac_script_hook_id(host.scripts, name);
}

std::vector<OverlayCallback> g_overlays;

openrac_mod* g_loading = nullptr;  // the mod whose openrac_mod_load runs: hooks are taken then only
std::vector<std::unique_ptr<openrac_mod>> g_loaded;
std::vector<fs::path> g_asset_roots;  // the loaded mods' assets folders, the last loaded first
std::vector<FrameCallback> g_frames;
std::vector<PadCallback> g_pads;
std::vector<MenuCallback> g_menus;
std::vector<DeveloperMenu> g_developer_menus;
// The pages of the game's own menus and the main-menu items (interface 1.7); the page on the
// screen (-1 none), whether a question is open on it, whether the title's main menu is (since
// 1.9 a mod opens its main-menu item's page from it), and what the mods asked of the screen.
std::vector<UiPageCallback> g_ui_pages;
std::vector<UiMainItem> g_ui_main;
std::vector<UiPauseItem> g_ui_pause;
int g_ui_shown = -1;
bool g_ui_question = false;
bool g_ui_main_menu = false;
UiRequests g_ui_requests;
// The mod whose menu page callback runs now: its drawing reaches the page, anyone else's is ignored.
openrac_mod* g_drawing = nullptr;
std::vector<TickBoundaryCallback> g_tick_boundaries;
openrac_mod* g_tick_mod = nullptr;
bool g_in_tick_boundary = false;
bool g_state_restored = false;
// The views and the Vulkan device setup (interface 1.4): one mod's each, none while its mod is null.
openrac_view_source g_views{};
openrac_mod* g_views_mod = nullptr;
openrac_vulkan_setup g_vulkan{};
openrac_mod* g_vulkan_mod = nullptr;
bool g_vulkan_queue_declared = false;
std::vector<crash::ModImage> g_images;
openrac_api g_api;
openrac_api g_api_without_native_vulkan;
const std::uint64_t g_snapshot_session = [] {
    const auto clock =
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&g_api));
    return clock ^ address;
}();

struct SnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t header_size;
    std::uint64_t frame;
    std::uint64_t session;
    std::uint32_t metadata_size;
    openrac_guest_allocation_state allocations;
    gaddr stack_pointer;
    gaddr display_chain;
    unsigned draws;
    bool reports_draws;
    bool vsync_since_kick;
    std::int32_t overlay;
    char game[16];
    char disc[16];
    char build[32];
};

constexpr char kSnapshotMagic[8] = {'O', 'R', 'A', 'C', 'S', 'T', '1', '\0'};
constexpr std::uint32_t kSnapshotVersion = 5;

std::size_t snapshot_size() {
    if (g_host == nullptr || g_host->game != "rac1"
        || (g_host->version != "pal" && g_host->version != "ntsc")
        || g_host->capture_savestate_runtime == nullptr
        || g_host->restore_savestate_runtime == nullptr || g_host->savestate_extra_size == nullptr
        || g_host->savestate_extra_save == nullptr || g_host->savestate_extra_load == nullptr
        || g_host->prepare_cold_savestate == nullptr) {
        return 0;
    }
    return sizeof(SnapshotHeader) + runtime::Memory::snapshot_size()
           + openrac_guest_snapshot_metadata_capacity() + g_host->savestate_extra_size();
}

int snapshot_save(void* output, std::size_t size) {
    if (!g_in_tick_boundary || g_tick_mod == nullptr || output == nullptr || snapshot_size() == 0
        || size != snapshot_size() || g_host == nullptr || g_host->loaded_level == nullptr) {
        return -1;
    }
    SavestateRuntime state{};
    if (!g_host->capture_savestate_runtime(&state)) {
        return -1;
    }
    SnapshotHeader header{};
    std::memcpy(header.magic, kSnapshotMagic, sizeof(kSnapshotMagic));
    header.version = kSnapshotVersion;
    header.header_size = sizeof(header);
    header.frame = state.frame;
    header.session = g_snapshot_session;
    openrac_guest_get_allocation_state(&header.allocations);
    header.stack_pointer = openrac_guest_sp;
    header.display_chain = state.display_chain;
    header.draws = state.draws;
    header.reports_draws = state.reports_draws;
    header.vsync_since_kick = state.vsync_since_kick;
    header.overlay = state.overlay;
    std::snprintf(header.game, sizeof(header.game), "%s", g_host->game.c_str());
    std::snprintf(header.disc, sizeof(header.disc), "%s", g_host->version.c_str());
    std::snprintf(header.build, sizeof(header.build), "%s", build_info::commit());
    auto* bytes = static_cast<std::uint8_t*>(output);
    const std::size_t memory_size = runtime::Memory::snapshot_size();
    std::size_t metadata_size = 0;
    const std::size_t allocation_capacity = openrac_guest_snapshot_metadata_capacity();
    const std::size_t extra_size = g_host->savestate_extra_size();
    if (!runtime::Memory::get().save_snapshot(std::span(bytes + sizeof(header), memory_size))
        || openrac_guest_snapshot_metadata_save(
               bytes + sizeof(header) + memory_size, allocation_capacity, &metadata_size
           ) != 0
        || g_host->savestate_extra_save(
               bytes + sizeof(header) + memory_size + allocation_capacity, extra_size
           ) != 0) {
        return -1;
    }
    if (metadata_size > std::numeric_limits<std::uint32_t>::max()) {
        return -1;
    }
    header.metadata_size = static_cast<std::uint32_t>(metadata_size);
    std::memcpy(bytes, &header, sizeof(header));
    return 0;
}

int snapshot_load(const void* input, std::size_t size) {
    if (!g_in_tick_boundary || g_tick_mod == nullptr || input == nullptr || snapshot_size() == 0
        || g_host == nullptr || g_host->loaded_level == nullptr
        || g_host->savestate_extra_size == nullptr || g_host->savestate_extra_load == nullptr
        || g_host->prepare_cold_savestate == nullptr
        || size < sizeof(SnapshotHeader) + runtime::Memory::snapshot_size()
        || size > sizeof(SnapshotHeader) + runtime::Memory::snapshot_size()
                      + openrac_guest_snapshot_metadata_capacity() + g_host->savestate_extra_size()
        || g_host->restore_savestate_runtime == nullptr) {
        return -1;
    }
    SnapshotHeader header{};
    std::memcpy(&header, input, sizeof(header));
    if (std::memcmp(header.magic, kSnapshotMagic, sizeof(kSnapshotMagic)) != 0
        || header.version != kSnapshotVersion || header.header_size != sizeof(header)
        || std::strncmp(header.game, g_host->game.c_str(), sizeof(header.game)) != 0
        || std::strncmp(header.disc, g_host->version.c_str(), sizeof(header.disc)) != 0
        || std::strncmp(header.build, build_info::commit(), sizeof(header.build)) != 0
        || header.metadata_size < sizeof(std::uint32_t)
        || header.metadata_size > openrac_guest_snapshot_metadata_capacity()
        || size
               != sizeof(SnapshotHeader) + runtime::Memory::snapshot_size()
                      + openrac_guest_snapshot_metadata_capacity() + g_host->savestate_extra_size()
        || header.frame > static_cast<std::uint64_t>(std::numeric_limits<long>::max())
        || header.stack_pointer < GUEST_STACK_BOTTOM || header.stack_pointer > GUEST_STACK_TOP) {
        return -1;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(input);
    const bool cold = header.session != g_snapshot_session;
    const std::size_t memory_size = runtime::Memory::snapshot_size();
    const auto* allocation_metadata = bytes + sizeof(header) + memory_size;
    const std::size_t allocation_capacity = openrac_guest_snapshot_metadata_capacity();
    const auto* extra = allocation_metadata + allocation_capacity;
    if (cold) {
        // Validate and install the target allocation maps before changing the
        // current overlay or guest memory. The loader rejects malformed
        // metadata without leaving the running game half-switched.
        if (openrac_guest_snapshot_metadata_load(
                allocation_metadata, header.metadata_size, &header.allocations
            )
            != 0) {
            return -1;
        }
        g_host->prepare_cold_savestate(header.overlay);
        if (header.overlay >= 0 && !openrac_guest_prepare_level_program(header.overlay)) {
            return -1;
        }
        if (header.overlay >= 0 && openrac_guest_restore_level_program(header.overlay) != 0) {
            return -1;
        }
    }
    if (!runtime::Memory::get().restore_snapshot(
            std::span(bytes + sizeof(header), memory_size), header.allocations
        )) {
        return -1;
    }
    if (g_host->savestate_extra_load(extra, g_host->savestate_extra_size()) != 0) {
        return -1;
    }
    openrac_guest_sp = header.stack_pointer;
    const SavestateRuntime state{
        header.frame,
        header.display_chain,
        header.draws,
        header.reports_draws,
        header.vsync_since_kick,
        header.overlay,
    };
    g_host->restore_savestate_runtime(&state);
    g_state_restored = true;
    return 0;
}

// ---- The interface's functions ----

const SettingValue* setting(openrac_mod* mod, const char* key, SettingType a, SettingType b) {
    if (mod == nullptr || key == nullptr) {
        return nullptr;
    }
    const auto it = mod->settings.find(key);
    if (it == mod->settings.end() || (it->second.type != a && it->second.type != b)) {
        return nullptr;
    }
    return &it->second;
}

void run_pads(int port, unsigned char* report, int size) {
    for (const PadCallback& c : g_pads) {
        c.fn(c.user, port, report, size);
    }
}

// The mod's developer menu item of `action`, or null.
DeveloperMenuItem* developer_menu_item(openrac_mod* mod, std::uint32_t action) {
    for (DeveloperMenu& menu : g_developer_menus) {
        if (menu.mod != mod) {
            continue;
        }
        const auto item = std::find_if(
            menu.items.begin(), menu.items.end(), [action](const DeveloperMenuItem& entry) {
                return entry.action == action;
            }
        );
        if (item != menu.items.end()) {
            return &*item;
        }
    }
    return nullptr;
}

bool native_vulkan_allowed(const mods::Manifest& manifest) {
    return manifest.api_minor < 20 || manifest.vulkan_queue;
}

const openrac_graphics* api_graphics_without_native_vulkan() {
    static const openrac_graphics vulkan = [] {
        openrac_graphics info{};
        info.size = sizeof(openrac_graphics);
        info.backend = OPENRAC_GRAPHICS_VULKAN;
        return info;
    }();
    const openrac_graphics* graphics = api_graphics();
    return graphics->backend == OPENRAC_GRAPHICS_VULKAN ? &vulkan : graphics;
}

// `mod`'s page `page`, or null.
UiPageCallback* mod_page(openrac_mod* mod, int page) {
    if (mod == nullptr || page < 0 || page >= static_cast<int>(g_ui_pages.size())) {
        return nullptr;
    }
    UiPageCallback& p = g_ui_pages[static_cast<std::size_t>(page)];
    return p.mod == mod ? &p : nullptr;
}

// A cursor kept on an item it takes: the last when past the end, else the nearest below, else
// above; 0 when it takes none.
int settle_cursor(const UiPage& page, int cursor) {
    const int count = static_cast<int>(page.items.size());
    cursor = std::clamp(cursor, 0, std::max(count - 1, 0));
    for (int i = cursor; i < count; ++i) {
        if (takes_cursor(page.items[static_cast<std::size_t>(i)])) {
            return i;
        }
    }
    for (int i = cursor - 1; i >= 0; --i) {
        if (takes_cursor(page.items[static_cast<std::size_t>(i)])) {
            return i;
        }
    }
    return 0;
}

// ---- The mods a party has to agree on (interface 1.14) ----

// While on, what the mods make the game do is another player's doing passed on.
bool g_party_replay = false;

// A setting's value as text: the same value, the same text in every game. A float by the shortest
// text that reads back as it, so nothing of a machine's formatting comes into it.
std::string setting_text(const SettingValue& v) {
    switch (v.type) {
        case SettingType::Bool:
            return v.b ? "true" : "false";
        case SettingType::Int:
            return std::to_string(v.i);
        case SettingType::Float:
            return std::format("{}", v.f);
        default:
            return v.s;
    }
}

// ---- Interface 1.14: calls of the port as a whole that a shared game needs. Each answers
// "unknown" until its package fills it in. ----

// ---- A mod's settings set from the game, and the launcher's changes (interface 1.15) ----

std::vector<SettingsCallback> g_settings_callbacks;
// The list's time when the port last read or wrote it, and the ticks until it looks again (about a
// second at 60 ticks a second).
std::optional<fs::file_time_type> g_list_seen;
int g_list_wait = 0;
constexpr int kListTicks = 60;
bool g_list_unwritten_said = false;

std::string quoted(std::string_view s);
bool read_text(const fs::path& path, std::string& out);

// A JSON number as text: an integer without a fraction, else the shortest text that reads back.
std::string number_text(double d) {
    if (std::isfinite(d) && std::floor(d) == d && std::fabs(d) < 1e15) {
        return std::format("{}", static_cast<std::int64_t>(d));
    }
    return std::format("{}", d);
}

// `v` as JSON text, two spaces a level from `depth`; `replace` (a value inside `v`) written as
// `replacement` instead.
void write_json(
    std::string& out,
    const json::Value& v,
    int depth,
    const json::Value* replace,
    const std::string& replacement
) {
    if (&v == replace) {
        out += replacement;
        return;
    }
    const std::string in(static_cast<std::size_t>(2 * (depth + 1)), ' ');
    const std::string at(static_cast<std::size_t>(2 * depth), ' ');
    switch (v.type()) {
        case json::Value::Type::Null:
            out += "null";
            break;
        case json::Value::Type::Bool:
            out += v.boolean() ? "true" : "false";
            break;
        case json::Value::Type::Number:
            out += number_text(v.number());
            break;
        case json::Value::Type::String:
            out += mods::quoted(v.string());
            break;
        case json::Value::Type::Array: {
            if (v.items().empty()) {
                out += "[]";
                break;
            }
            out += "[\n";
            for (std::size_t i = 0; i < v.items().size(); ++i) {
                out += in;
                write_json(out, v.items()[i], depth + 1, replace, replacement);
                out += i + 1 < v.items().size() ? ",\n" : "\n";
            }
            out += at + "]";
            break;
        }
        case json::Value::Type::Object: {
            if (v.members().empty()) {
                out += "{}";
                break;
            }
            out += "{\n";
            std::size_t i = 0;
            for (const auto& [key, member] : v.members()) {
                out += in + mods::quoted(key) + ": ";
                write_json(out, member, depth + 1, replace, replacement);
                out += ++i < v.members().size() ? ",\n" : "\n";
            }
            out += at + "}";
            break;
        }
    }
}

// The list's text with mod `id`'s setting `key` set to `literal` (JSON text), every other mod,
// setting and member as it was (an object's members in the order the reader keeps them); empty
// when the list has no entry for the mod.
std::string list_with(
    const json::Value& list, std::string_view id, const std::string& key, const std::string& literal
) {
    const json::Value* entry = nullptr;
    for (const json::Value& m : list["mods"].items()) {
        if (m["id"].is_string() && m["id"].string() == id) {
            entry = &m;  // the first place counts, as parse_list has it
            break;
        }
    }
    if (entry == nullptr || !entry->is_object()) {
        return {};
    }
    // The entry written again at its place (two levels down: the list, "mods"), its settings with
    // the key set, added when it has none.
    std::map<std::string, std::string, std::less<>> settings;
    if (const json::Value& had = (*entry)["settings"]; had.is_object()) {
        for (const auto& [name, member] : had.members()) {
            std::string value;
            write_json(value, member, 4, nullptr, {});
            settings[name] = value;
        }
    }
    settings[key] = literal;
    std::map<std::string, std::string, std::less<>> members;
    for (const auto& [name, member] : entry->members()) {
        std::string value;
        write_json(value, member, 3, nullptr, {});
        members[name] = value;
    }
    std::string settings_text = "{\n";
    std::size_t i = 0;
    for (const auto& [name, value] : settings) {
        settings_text +=
            "        " + mods::quoted(name) + ": " + value + (++i < settings.size() ? ",\n" : "\n");
    }
    members["settings"] = settings_text + "      }";
    std::string entry_text = "{\n";
    i = 0;
    for (const auto& [name, value] : members) {
        entry_text +=
            "      " + mods::quoted(name) + ": " + value + (++i < members.size() ? ",\n" : "\n");
    }
    entry_text += "    }";
    std::string out;
    write_json(out, list, 0, entry, entry_text);
    return out + "\n";
}

// Writes the mod's setting into the launcher's list; false, and why, when it cannot.
bool write_setting(
    const openrac_mod& mod, const std::string& key, const std::string& literal, std::string& why
) {
    if (g_host == nullptr || g_host->purist || g_host->list.empty()) {
        why = "no list of mods in this run";
        return false;
    }
    const fs::path& path = g_host->list;
    std::string text;
    json::Value list;
    std::string error;
    if (!read_text(path, text) || !json::parse(text, list, error) || !list["mods"].is_array()) {
        why = log::tidy(path.string()) + " cannot be read";
        return false;
    }
    const std::string next = list_with(list, mod.id, key, literal);
    if (next.empty()) {
        why = log::tidy(path.string()) + " has no entry for it";
        return false;
    }
    // Written beside and moved into place, as the launcher writes it, so neither reads half.
    fs::path temporary = path;
    temporary += ".port.tmp";
    {
        std::ofstream f(temporary, std::ios::binary | std::ios::trunc);
        f << next;
        if (!f) {
            why = log::tidy(temporary.string()) + " cannot be written";
            return false;
        }
    }
    std::error_code e;
    fs::rename(temporary, path, e);
    if (e) {
        fs::remove(temporary, e);
        why = log::tidy(path.string()) + " cannot be replaced";
        return false;
    }
    // The port's own write is not a change of the launcher's to tell.
    g_list_seen = fs::last_write_time(path, e);
    if (e) {
        g_list_seen.reset();
    }
    return true;
}

// Sets setting `key`, of type `a` or `b`, from `literal` (JSON text) for this run and in the list.
int set_setting(
    openrac_mod* mod, const char* key, SettingType a, SettingType b, const std::string& literal
) {
    if (mod == nullptr || key == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const std::vector<Setting>& declared = mod->manifest.settings;
    const auto it = std::find_if(declared.begin(), declared.end(), [&](const Setting& d) {
        return d.key == key;
    });
    if (it == declared.end() || (it->type != a && it->type != b)
        || (g_host != nullptr && !setting_for_game(*it, g_host->game))) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    json::Value v;
    std::string error;
    SettingValue value;
    if (!json::parse(literal, v, error) || !setting_value(*it, v, value)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    mod->settings[key] = value;
    if (mod->built_in) {
        if (g_host != nullptr && g_host->built_in_write != nullptr) {
            g_host->built_in_write(mod->id, key, setting_text(value));
        }
        return OPENRAC_HOOK_OK;
    }
    std::string why;
    if (!write_setting(*mod, key, literal, why) && !g_list_unwritten_said) {
        log::warn("mod {}: setting \"{}\" kept for this run only: {}", mod->id, key, why);
        g_list_unwritten_said = true;
    }
    return OPENRAC_HOOK_OK;
}

const std::map<std::string, SettingValue>* settings_of(openrac_mod* mod) {
    return mod != nullptr ? &mod->settings : nullptr;
}

bool same(const SettingValue& x, const SettingValue& y) {
    return x.type == y.type && x.b == y.b && x.i == y.i && x.f == y.f && x.s == y.s;
}

// Gives `mod` the settings `now` and tells it, when they differ from its own.
void take_settings(openrac_mod* mod, std::map<std::string, SettingValue> now, const char* where) {
    const bool changed =
        now.size() != mod->settings.size()
        || !std::equal(
            now.begin(), now.end(), mod->settings.begin(), [](const auto& x, const auto& y) {
                return x.first == y.first && same(x.second, y.second);
            }
        );
    if (!changed) {
        return;
    }
    mod->settings = std::move(now);
    log::info("mod {}: its settings changed in {}", mod->id, where);
    for (const SettingsCallback& c : g_settings_callbacks) {
        if (c.mod == mod) {
            c.fn(c.user);
        }
    }
}

std::map<std::string, json::Value> built_in_values(
    const Manifest& manifest, const std::map<std::string, std::string>& text
);

// The built-ins' settings again when the launcher changed them while the game runs.
void follow_built_ins() {
    std::map<std::string, std::map<std::string, std::string>> text;
    if (g_host->built_in_reread == nullptr || !g_host->built_in_reread(text)) {
        return;
    }
    for (const auto& loaded : g_loaded) {
        openrac_mod* mod = loaded.get();
        const auto it = text.find(mod->id);
        if (!mod->built_in || it == text.end()) {
            continue;
        }
        std::vector<std::string> notes;
        take_settings(
            mod,
            resolve_settings(
                mod->manifest, built_in_values(mod->manifest, it->second), notes, g_host->game
            ),
            "the settings file"
        );
    }
}

// About once a second while a mod asked to be told: the list read again when it changed, and each
// such mod whose settings it now gives other values told, after they are its.
void follow_list() {
    if (g_settings_callbacks.empty() || g_host == nullptr || --g_list_wait > 0) {
        return;
    }
    g_list_wait = kListTicks;
    follow_built_ins();
    if (g_host->purist || g_host->list.empty()) {
        return;
    }
    std::error_code e;
    const fs::file_time_type stamp = fs::last_write_time(g_host->list, e);
    if (e || (g_list_seen && *g_list_seen == stamp)) {
        return;
    }
    std::string text;
    std::vector<ListEntry> list;
    std::string error;
    if (!read_text(g_host->list, text) || !parse_list(text, list, error)) {
        return;  // half written by the launcher: read again when it changes
    }
    g_list_seen = stamp;
    for (const auto& loaded : g_loaded) {
        openrac_mod* mod = loaded.get();
        const bool told = std::any_of(
            g_settings_callbacks.begin(),
            g_settings_callbacks.end(),
            [&](const SettingsCallback& c) { return c.mod == mod; }
        );
        const auto entry = std::find_if(list.begin(), list.end(), [&](const ListEntry& l) {
            return l.id == mod->id;
        });
        if (!told || entry == list.end()) {
            continue;
        }
        std::vector<std::string> notes;
        take_settings(
            mod,
            resolve_settings(mod->manifest, entry->settings, notes, g_host->game),
            "the launcher's list"
        );
    }
}

// ---- A mod's data kept with the game's saves (interface 1.16) ----
//
// Each mod's data for the game in progress lives here, by the mod's id. lib/mc.c tells of each save
// the game writes (save_written: every mod's data beside it, `<save>.moddata`) and of each save it
// reads whole (save_read: that file's data kept aside). The game in progress takes it once the
// game plays that save (follow_saves, after a tick: the play host's save_slot_current, which also
// says a new game started after it). The save file itself is never touched, and the file beside it
// is hidden from the game (mc.c's host_only), as the save mark is.
//
// The file: {"format": 1, "mods": {"<id>": {"version": "<version>", "data": "<base64>"}}}.

std::vector<SaveDataCallback> g_save_data_callbacks;
// The game in progress's data, by mod id; the data of the save the game read whole last, its slot,
// and how many such reads there were and which one the game in progress took.
std::map<std::string, std::vector<std::uint8_t>, std::less<>> g_save_data;
std::map<std::string, std::vector<std::uint8_t>, std::less<>> g_save_read_data;
int g_save_read_slot = -1;
std::uint32_t g_save_reads = 0;
std::uint32_t g_save_taken = 0;
// The slot of the save the game in progress plays, -1 for a new game (or none).
int g_save_playing = -1;
// A new game started (the host said so): the mods are told at the next tick.
bool g_save_new_game = false;

constexpr std::string_view kBase64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64(const std::vector<std::uint8_t>& bytes) {
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        const std::uint32_t n = (std::uint32_t{bytes[i]} << 16)
                                | (i + 1 < bytes.size() ? std::uint32_t{bytes[i + 1]} << 8 : 0u)
                                | (i + 2 < bytes.size() ? std::uint32_t{bytes[i + 2]} : 0u);
        out += kBase64[(n >> 18) & 63u];
        out += kBase64[(n >> 12) & 63u];
        out += i + 1 < bytes.size() ? kBase64[(n >> 6) & 63u] : '=';
        out += i + 2 < bytes.size() ? kBase64[n & 63u] : '=';
    }
    return out;
}

// False for text that is not base64 (padding only at the end).
bool unbase64(std::string_view text, std::vector<std::uint8_t>& out) {
    out.clear();
    if (text.size() % 4 != 0) {
        return false;
    }
    std::uint32_t n = 0;
    int bits = 0;
    std::size_t pad = 0;
    for (const char c : text) {
        if (c == '=') {
            ++pad;
            continue;
        }
        const std::size_t v = kBase64.find(c);
        if (v == std::string_view::npos || pad != 0) {
            return false;
        }
        n = (n << 6) | static_cast<std::uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((n >> bits) & 0xFFu));
        }
    }
    return pad <= 2;
}

fs::path save_data_path(const char* save) {
    return fs::path(save).concat(".moddata");
}

// lib/mc.c: the game has just written the save at `path`. Every loaded mod's data goes beside it,
// written whole (a hidden file renamed over it); a save with none, or a blank slot, keeps none.
void save_written(const char* path, int players) {
    const fs::path file = save_data_path(path);
    std::error_code e;
    std::string members;
    for (const auto& loaded : g_loaded) {
        const auto data = g_save_data.find(loaded->id);
        if (data == g_save_data.end() || data->second.empty()) {
            continue;
        }
        members += std::format(
            "{}\n    {}: {{\"version\": {}, \"data\": {}}}",
            members.empty() ? "" : ",",
            mods::quoted(loaded->id),
            mods::quoted(loaded->version),
            mods::quoted(base64(data->second))
        );
    }
    if (members.empty() || players == 0) {
        fs::remove(file, e);
        return;
    }
    // Hidden from the game while it is written (a leading dot), then in place whole.
    const fs::path temp = file.parent_path() / ("." + file.filename().string() + ".tmp");
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << "{\"format\": 1, \"mods\": {" << members << "\n}}\n";
        if (!out) {
            log::warn("the mods' data was not written beside {}", path);
            return;
        }
    }
    fs::rename(temp, file, e);
    if (e) {
        fs::remove(temp, e);
        log::warn("the mods' data was not written beside {}", path);
    }
}

// lib/mc.c: the game has just read the save at `path` whole (Load Game): the data beside it is
// kept aside until the game plays that save (follow_saves).
void save_read(const char* path, int slot) {
    g_save_read_data.clear();
    g_save_read_slot = slot;
    ++g_save_reads;
    std::string text;
    const fs::path file = save_data_path(path);
    std::error_code e;
    if (!fs::exists(file, e) || !read_text(file, text)) {
        return;  // saved without mods' data
    }
    json::Value v;
    std::string error;
    if (!json::parse(text, v, error) || !v["mods"].is_object()) {
        log::warn("the mods' data beside {} is not readable: {}", path, error);
        return;
    }
    for (const auto& [id, entry] : v["mods"].members()) {
        std::vector<std::uint8_t> bytes;
        if (!entry["data"].is_string() || !unbase64(entry["data"].string(), bytes)
            || bytes.size() > OPENRAC_SAVE_DATA_MAX) {
            log::warn("mod {}: its data beside {} is not readable", id, path);
            continue;
        }
        g_save_read_data[id] = std::move(bytes);
    }
}

// A game's host: a new game starts. Its data is empty from now on (the save the game writes right
// after takes none), and a save read before is not its; the mods are told at the next tick.
void new_game() {
    g_save_data.clear();
    g_save_taken = g_save_reads;
    g_save_playing = -1;
    g_save_new_game = true;
}

void tell_save_data() {
    for (const SaveDataCallback& c : g_save_data_callbacks) {
        c.fn(c.user);
    }
}

// After each tick, while a level plays: the game in progress takes the data of the save it now
// plays when it was read whole since, and a new game started (the play host has no save for it)
// starts with none.
void follow_saves() {
    // A new game the host announced: told whatever the phase, before the mods' own frame callbacks
    // could set the old game's data again.
    if (g_save_new_game) {
        g_save_new_game = false;
        log::info("a new game: the mods' data starts empty");
        tell_save_data();
    }
    if (g_api.play_status == nullptr || g_api.save_slot_current == nullptr) {
        return;
    }
    openrac_play_status status{};
    status.size = sizeof status;
    if (g_api.play_status(&status) != OPENRAC_HOOK_OK || status.phase != OPENRAC_PLAY_PLAYING) {
        return;
    }
    const int slot = g_api.save_slot_current();
    if (slot >= 0 && slot == g_save_read_slot && g_save_reads != g_save_taken) {
        g_save_taken = g_save_reads;
        g_save_playing = slot;
        g_save_data = g_save_read_data;
        log::info("the mods' data of save {} taken ({} mods)", slot, g_save_data.size());
        tell_save_data();
    } else if (slot < 0 && g_save_playing >= 0) {
        g_save_playing = -1;
        g_save_data.clear();
        log::info("a new game: the mods' data starts empty");
        tell_save_data();
    } else if (slot >= 0) {
        g_save_playing = slot;  // saved into, or the same save played on
    }
}

void reset_save_data() {
    g_save_data_callbacks.clear();
    g_save_data.clear();
    g_save_read_data.clear();
    g_save_read_slot = -1;
    g_save_reads = 0;
    g_save_taken = 0;
    g_save_playing = -1;
    g_save_new_game = false;
}

void fill_api(const Host& host) {
    static std::string game, version, openrac_version;
    game = host.game;
    version = host.version;
    openrac_version = host.openrac_version;
    g_api = openrac_api{};
    g_api.size = sizeof(openrac_api);
    g_api.api_major = OPENRAC_MOD_API_MAJOR;
    g_api.api_minor = OPENRAC_MOD_API_MINOR;
    g_api.game = game.c_str();
    g_api.version = version.c_str();
    g_api.openrac_version = openrac_version.c_str();
    g_api.platform = platform_key();
    g_api.memory = openrac_guest_base;
    g_api.log = api_log;
    g_api.fail = api_fail;
    g_api.setting_bool = api_setting_bool;
    g_api.setting_int = api_setting_int;
    g_api.setting_float = api_setting_float;
    g_api.setting_string = api_setting_string;
    g_api.hook = api_hook;
    g_api.on_frame = api_on_frame;
    g_api.on_pad = api_on_pad;
    g_api.function = api_function;
    g_api.function_at = api_function_at;
    g_api.relocate_data = api_relocate_data;
    g_api.relocate_level_data = api_relocate_level_data;
    g_api.loaded_level = api_loaded_level;
    g_api.stack_push = api_stack_push;
    g_api.stack_pop = api_stack_pop;
    g_api.input = api_input;
    g_api.take_input = api_take_input;
    g_api.capture_mouse = api_capture_mouse;
    g_api.graphics = api_graphics;
    g_api.on_views = api_on_views;
    g_api.setup_vulkan = api_setup_vulkan;
    g_api.menu_entry = api_menu_entry;
    g_api.menu_rect = api_menu_rect;
    g_api.menu_text = api_menu_text;
    g_api.on_tick_boundary = api_on_tick_boundary;
    g_api.developer_menu = api_developer_menu;
    g_api.take_developer_menu_action = api_take_developer_menu_action;
    g_api.developer_menu_set_label = api_developer_menu_set_label;
    g_api.developer_menu_switch = api_developer_menu_switch;
    g_api.developer_menu_choice = api_developer_menu_choice;
    g_api.developer_menu_value = api_developer_menu_value;
    g_api.developer_menu_look = api_developer_menu_look;
    g_api.hook_entry = api_hook_entry;
    g_api.data_dir = api_data_dir;
    g_api.savestate_size = snapshot_size;
    g_api.savestate_save = snapshot_save;
    g_api.savestate_load = snapshot_load;
    g_api.ui_page = api_ui_page;
    g_api.ui_main_item = api_ui_main_item;
    g_api.ui_set = api_ui_set;
    g_api.ui_open = api_ui_open;
    g_api.ui_confirm = api_ui_confirm;
    g_api.ui_cursor = api_ui_cursor;
    g_api.ui_sound = api_ui_sound;
    g_api.ui_pause_item = api_ui_pause_item;
    g_api.mod_count = api_mod_count;
    g_api.mod_entry = api_mod_entry;
    g_api.mod_setting = api_mod_setting;
    g_api.party_rules_publish = api_party_rules_publish;
    g_api.party_rules_read = api_party_rules_read;
    g_api.party_rules_text = api_party_rules_text;
    g_api.on_party_rules = api_on_party_rules;
    g_api.party_rules_deliver = api_party_rules_deliver;
    g_api.party_replay = api_party_replay;
    g_api.party_replaying = api_party_replaying;
    g_api.on_overlay = api_on_overlay;
    g_api.packs_info = api_packs_info;
    g_api.packs_entry = api_packs_entry;
    g_api.clipboard_set = api_clipboard_set;
    g_api.open_link = api_open_link;
    g_api.plugins_folder = api_plugins_folder;
    g_api.identity_key = api_identity_key;
    fill_play_api(g_api, host.play, host.cards);
    fill_motion_api(g_api, host);
    g_api.setting_set_bool = api_setting_set_bool;
    g_api.setting_set_int = api_setting_set_int;
    g_api.setting_set_float = api_setting_set_float;
    g_api.setting_set_string = api_setting_set_string;
    g_api.on_settings = api_on_settings;
    g_api.save_data_set = api_save_data_set;
    g_api.save_data_get = api_save_data_get;
    g_api.on_save_data = api_on_save_data;
    g_api.weapon_ignore_culling = api_weapon_ignore_culling;
    g_api_without_native_vulkan = g_api;
    g_api_without_native_vulkan.graphics = api_graphics_without_native_vulkan;
}

// ---- status.json ----

std::string quoted(std::string_view s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    out += std::format("\\u{:04x}", c);
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

struct Outcome {
    std::string id;
    std::string version;
    std::string state;  // loaded, refused, failed
    std::string reason;
    std::vector<std::string> hooks;
};

/* The loaded mods as the JSON array lib/mc.c writes beside each save, so a save says what it was
 * played with (openrac_game_loaded_mods, game_host.h). Built once, after loading, and kept for the
 * run: the save path reads the pointer and never builds anything. */
std::string g_loaded_mods_json = "[]";

void set_loaded_mods_json() {
    std::ostringstream o;
    o << "[";
    for (std::size_t i = 0; i < g_loaded.size(); ++i) {
        o << (i == 0 ? "" : ", ") << "{\"id\": " << quoted(g_loaded[i]->id)
          << ", \"version\": " << quoted(g_loaded[i]->version) << "}";
    }
    o << "]";
    g_loaded_mods_json = o.str();
    openrac_game_loaded_mods = g_loaded_mods_json.c_str();
    // The mods' data kept with the saves (1.16): lib/mc.c tells of the saves written and read.
    openrac_game_save_written = save_written;
    openrac_game_save_read = save_read;
    openrac_game_new_game = new_game;
}

void write_status(const Host& host, const std::vector<Outcome>& outcomes) {
    const fs::path folder = host.list.parent_path();
    std::error_code e;
    if (folder.empty() || !fs::is_directory(folder, e)) {
        return;
    }
    std::ostringstream o;
    o << "{\n  \"format\": 1,\n";
    o << "  \"time\": " << quoted(host.now) << ",\n";
    o << "  \"game\": " << quoted(host.game) << ", \"version\": " << quoted(host.version)
      << ", \"platform\": " << quoted(platform_key()) << ",\n";
    o << "  \"api\": " << quoted(std::format("{}.{}", OPENRAC_MOD_API_MAJOR, OPENRAC_MOD_API_MINOR))
      << ", \"openrac\": " << quoted(host.openrac_version) << ",\n";
    o << "  \"purist\": " << (host.purist ? "true" : "false") << ",\n";
    o << "  \"mods\": [";
    for (std::size_t i = 0; i < outcomes.size(); ++i) {
        const Outcome& m = outcomes[i];
        o << (i == 0 ? "\n" : ",\n") << "    {\"id\": " << quoted(m.id);
        if (!m.version.empty()) {
            o << ", \"version\": " << quoted(m.version);
        }
        o << ", \"state\": " << quoted(m.state);
        if (!m.reason.empty()) {
            o << ", \"reason\": " << quoted(m.reason);
        }
        if (!m.hooks.empty()) {
            o << ", \"hooks\": [";
            for (std::size_t h = 0; h < m.hooks.size(); ++h) {
                o << (h == 0 ? "" : ", ") << quoted(m.hooks[h]);
            }
            o << "]";
        }
        o << "}";
    }
    o << (outcomes.empty() ? "],\n" : "\n  ],\n");
    o << "  \"hook_points\": [";
    for (int i = 0; i < host.hook_count; ++i) {
        o << (i == 0 ? "" : ", ") << quoted(hook_id(host, host.hooks[i].name));
    }
    o << "]\n}\n";
    // Written beside and moved into place, so the launcher never reads half a file.
    const fs::path path = folder / "status.json";
    const fs::path temporary = folder / "status.json.tmp";
    {
        std::ofstream f(temporary, std::ios::binary | std::ios::trunc);
        f << o.str();
        if (!f) {
            log::warn("mods: {} cannot be written", log::tidy(path.string()));
            return;
        }
    }
    fs::rename(temporary, path, e);
    if (e) {
        log::warn("mods: {} cannot be written ({})", log::tidy(path.string()), e.message());
    }
}

bool read_text(const fs::path& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::ostringstream s;
    s << f.rdbuf();
    out = s.str();
    return true;
}

// What status.json says of each mod of the list, kept for a script mod that stops later.
std::vector<Outcome> g_outcomes;

// A script mod stopped while the game ran (an error, its budget): status.json says so from now on.
// Its hooks pass to their originals and its callbacks stop (mod_scripts.cpp).
void script_stopped(openrac_mod* mod, const std::string& reason) {
    if (mod == nullptr || g_host == nullptr) {
        return;
    }
    for (Outcome& o : g_outcomes) {
        if (o.id == mod->id) {
            o.state = "failed";
            o.reason = "stopped while the game ran: " + reason;
        }
    }
    write_status(*g_host, g_outcomes);
}

// Takes back what a mod did in a load that failed: its hooks (it was the last to take any), its
// callbacks.
void undo(openrac_mod* mod) {
    for (auto it = mod->taken.rbegin(); it != mod->taken.rend(); ++it) {
        *it->first = it->second;
    }
    mod->taken.clear();
    mod->hooks.clear();
    std::erase_if(g_frames, [&](const FrameCallback& c) { return c.mod == mod; });
    std::erase_if(g_settings_callbacks, [&](const SettingsCallback& c) { return c.mod == mod; });
    std::erase_if(g_save_data_callbacks, [&](const SaveDataCallback& c) { return c.mod == mod; });
    std::erase_if(g_pads, [&](const PadCallback& c) { return c.mod == mod; });
    std::erase_if(g_menus, [&](const MenuCallback& c) { return c.mod == mod; });
    std::erase_if(g_overlays, [&](const OverlayCallback& c) { return c.mod == mod; });
    std::erase_if(g_developer_menus, [&](const DeveloperMenu& menu) { return menu.mod == mod; });
    // Its main-menu and pause items, then its pages (the last made: no other mod's id moves).
    std::erase_if(g_ui_main, [&](const UiMainItem& m) { return mod_page(mod, m.page) != nullptr; });
    std::erase_if(g_ui_pause, [&](const UiPauseItem& m) {
        return mod_page(mod, m.page) != nullptr;
    });
    std::erase_if(g_ui_pages, [&](const UiPageCallback& c) { return c.mod == mod; });
    std::erase_if(g_tick_boundaries, [&](const TickBoundaryCallback& c) { return c.mod == mod; });
    drop_figures(mod);
    if (g_views_mod == mod) {
        g_views = {};
        g_views_mod = nullptr;
    }
    if (g_vulkan_mod == mod) {
        g_vulkan = {};
        g_vulkan_mod = nullptr;
    }
}

// Opens and loads one mod that passed the checks; false and why when it does not load.
bool load_one(const fs::path& folder, const Candidate& c, std::unique_ptr<openrac_mod>& out, std::string& state,
              std::string& reason) {
    auto mod = std::make_unique<openrac_mod>();
    mod->id = c.manifest.id;
    mod->dir = folder / c.manifest.id;
    mod->version = c.manifest.version_text;
    mod->manifest = c.manifest;
    std::vector<std::string> notes;
    mod->settings = resolve_settings(c.manifest, c.entry.settings, notes, g_host->game);
    for (const std::string& n : notes) {
        log::warn("mod {}: {}", mod->id, n);
    }
    if (c.manifest.script) {
        mod->script = true;
        mod->file = c.manifest.scripts.front();
        scripts::Mod s;
        s.handle = mod.get();
        s.id = mod->id;
        s.folder = folder / c.manifest.id;
        s.scripts = c.manifest.scripts;
        s.settings = mod->settings;
        s.game = g_host->game;
        s.version = g_host->version;
        s.platform = platform_key();
        s.api = std::format("{}.{}", OPENRAC_MOD_API_MAJOR, OPENRAC_MOD_API_MINOR);
        const scripts::Services services{
            api_hook,
            api_on_frame,
            api_on_pad,
            api_log,
            script_stopped,
            api_input,
            api_take_input,
            api_capture_mouse,
            api_setting_set_bool,
            api_setting_set_int,
            api_setting_set_float,
            api_setting_set_string,
            api_on_settings,
            settings_of
        };
        std::string why;
        g_loading = mod.get();
        const bool loaded = scripts::load(s, g_host->scripts, services, why);
        g_loading = nullptr;
        if (!loaded || mod->hook_refused) {
            undo(mod.get());
            scripts::close(mod.get());
            state = "failed";
            reason = !mod->failure.empty() ? mod->failure : why;
            return false;
        }
        out = std::move(mod);
        return true;
    }
    const fs::path path = folder / c.manifest.id / fs::path(c.manifest.plugin.at(platform_key()));
    mod->file = path.filename().string();
    std::string error;
    mod->library = library::open(path, error);
    if (mod->library == nullptr) {
        state = "failed";
        reason = "its library cannot be opened: " + error;
        return false;
    }
    auto query = reinterpret_cast<openrac_mod_query_fn>(library::symbol(mod->library, OPENRAC_MOD_QUERY));
    auto load = reinterpret_cast<openrac_mod_load_fn>(library::symbol(mod->library, OPENRAC_MOD_LOAD));
    mod->unload = reinterpret_cast<openrac_mod_unload_fn>(library::symbol(mod->library, OPENRAC_MOD_UNLOAD));
    auto give_up = [&](std::string why) {
        library::close(mod->library);
        state = "failed";
        reason = std::move(why);
        return false;
    };
    if (query == nullptr || load == nullptr) {
        return give_up("its library does not export openrac_mod_query and openrac_mod_load");
    }
    openrac_mod_info info{};
    info.size = sizeof(info);
    query(&info);
    if (info.api_major != static_cast<uint32_t>(c.manifest.api_major)
        || info.api_minor != static_cast<uint32_t>(c.manifest.api_minor)) {
        return give_up(std::format("its library was built for interface {}.{}, its manifest says {}.{}",
                                   info.api_major, info.api_minor, c.manifest.api_major, c.manifest.api_minor));
    }
    if (info.id == nullptr || mod->id != info.id) {
        return give_up(std::format("its library is mod \"{}\", its manifest {}", info.id != nullptr ? info.id : "",
                                   mod->id));
    }
    mod->base = library::base_of(reinterpret_cast<const void*>(load));
    g_loading = mod.get();
    play_loading(mod.get());
    const openrac_api& api =
        native_vulkan_allowed(mod->manifest) ? g_api : g_api_without_native_vulkan;
    const int result = load(&api, mod.get());
    g_loading = nullptr;
    play_loading(nullptr);
    // A hook point asked for and not there (or of another signature) fails the load even when the
    // mod carries on: it would run with part of itself missing.
    if (result != 0 || mod->hook_refused) {
        undo(mod.get());
        state = "failed";
        reason = !mod->failure.empty() ? mod->failure : std::format("its load returned {}", result);
        library::close(mod->library);
        return false;
    }
    out = std::move(mod);
    return true;
}

// A built-in's settings, as text (Host::built_in_settings), in the JSON a list entry would hold:
// a switch's 1|0|true|false as true or false, a number as a number, the rest as strings. A value
// that is not of its type is left out, and the setting is its default.
std::map<std::string, json::Value> built_in_values(
    const Manifest& manifest, const std::map<std::string, std::string>& text
) {
    std::map<std::string, json::Value> out;
    for (const Setting& s : manifest.settings) {
        const auto it = text.find(s.key);
        if (it == text.end()) {
            continue;
        }
        std::string literal;
        switch (s.type) {
            case SettingType::Bool:
                literal = it->second == "1" || it->second == "true"    ? "true"
                          : it->second == "0" || it->second == "false" ? "false"
                                                                       : "";
                break;
            case SettingType::Int:
            case SettingType::Float:
                literal = it->second;
                break;
            default:
                literal = mods::quoted(it->second);
                break;
        }
        json::Value v;
        std::string error;
        if (!literal.empty() && json::parse(literal, v, error)) {
            out[s.key] = std::move(v);
        }
    }
    return out;
}

// Loads one mod built into the program (BuiltIn): true when it is for this game and disc and
// loaded. `why` says why one for this disc did not load.
bool load_built_in(const BuiltIn& b, std::unique_ptr<openrac_mod>& out, std::string& why) {
    Manifest manifest;
    std::string error;
    if (!parse_manifest(b.manifest, manifest, error)) {
        why = "its manifest: " + error;
        return false;
    }
    const auto game = manifest.games.find(g_host->game);
    if (game == manifest.games.end()
        || std::find(game->second.begin(), game->second.end(), g_host->version)
               == game->second.end()) {
        return false;  // not for this disc: nothing to say
    }
    auto mod = std::make_unique<openrac_mod>();
    mod->id = manifest.id;
    mod->version = manifest.version_text;
    mod->file = "built in";
    mod->built_in = true;
    mod->manifest = manifest;
    std::vector<std::string> notes;
    const auto text = g_host->built_in_settings.find(manifest.id);
    mod->settings = resolve_settings(
        manifest,
        text != g_host->built_in_settings.end() ? built_in_values(manifest, text->second)
                                                : std::map<std::string, json::Value>{},
        notes,
        g_host->game
    );
    for (const std::string& n : notes) {
        log::warn("{}: {}", mod->id, n);
    }
    openrac_mod_info info{};
    info.size = sizeof(info);
    b.query(&info);
    if (info.id == nullptr || mod->id != info.id) {
        why = "its code and its manifest name other mods";
        return false;
    }
    mod->base = reinterpret_cast<const void*>(b.load);
    g_loading = mod.get();
    const int result = b.load(&g_api, mod.get());
    g_loading = nullptr;
    if (result != 0 || mod->hook_refused) {
        undo(mod.get());
        why = !mod->failure.empty() ? mod->failure : std::format("its load returned {}", result);
        return false;
    }
    out = std::move(mod);
    return true;
}

}  // namespace detail

const std::vector<fs::path>& asset_roots() {
    return g_asset_roots;
}

namespace detail {

openrac_mod* loaded_built_in(std::string_view id) {
    for (const auto& loaded : g_loaded) {
        if (loaded->built_in && loaded->id == id) {
            return loaded.get();
        }
    }
    return nullptr;
}

}  // namespace detail

std::vector<const Setting*> built_in_settings(std::string_view id) {
    std::vector<const Setting*> out;
    if (const openrac_mod* mod = loaded_built_in(id)) {
        for (const Setting& s : mod->manifest.settings) {
            if (g_host == nullptr || setting_for_game(s, g_host->game)) {
                out.push_back(&s);
            }
        }
    }
    return out;
}

std::vector<Preset> built_in_presets(std::string_view id) {
    std::vector<Preset> out;
    if (const openrac_mod* mod = loaded_built_in(id)) {
        for (Preset p : mod->manifest.presets) {
            std::erase_if(p.values, [&](const auto& v) {
                const auto s = std::find_if(
                    mod->manifest.settings.begin(),
                    mod->manifest.settings.end(),
                    [&](const Setting& x) { return x.key == v.first; }
                );
                return s == mod->manifest.settings.end()
                       || (g_host != nullptr && !setting_for_game(*s, g_host->game));
            });
            out.push_back(std::move(p));
        }
    }
    return out;
}

BuiltInSetting built_in_setting(std::string_view id, std::string_view key) {
    BuiltInSetting out;
    openrac_mod* mod = loaded_built_in(id);
    if (mod == nullptr) {
        return out;
    }
    for (const Setting& s : mod->manifest.settings) {
        if (s.key == key && (g_host == nullptr || setting_for_game(s, g_host->game))) {
            out.spec = &s;
            const auto it = mod->settings.find(s.key);
            out.value = it != mod->settings.end() ? it->second : s.fallback;
        }
    }
    return out;
}

bool set_built_in_setting(std::string_view id, std::string_view key, const std::string& literal) {
    openrac_mod* mod = loaded_built_in(id);
    const BuiltInSetting now = built_in_setting(id, key);
    if (mod == nullptr || now.spec == nullptr) {
        return false;
    }
    const std::string k(key);
    if (set_setting(mod, k.c_str(), now.spec->type, now.spec->type, literal) != OPENRAC_HOOK_OK) {
        return false;
    }
    for (const SettingsCallback& c : g_settings_callbacks) {
        if (c.mod == mod) {
            c.fn(c.user);
        }
    }
    return true;
}

std::string load(const Host& host) {
    static Host kept;
    kept = host;
    g_host = &kept;
    fill_api(kept);
    // The mods built into the program first, whatever the list says (BuiltIn).
    std::vector<std::string> built_in;
    if (!host.built_ins.empty()) {
        for (const BuiltIn& b : host.built_ins) {
            std::unique_ptr<openrac_mod> mod;
            std::string why;
            if (load_built_in(b, mod, why)) {
                log::info("{} {} built in", mod->id, mod->version);
                built_in.push_back(mod->id);
                g_loaded.push_back(std::move(mod));
            } else if (!why.empty()) {
                log::warn("a built-in mod did not load: {}", why);
            }
        }
    }
    std::vector<ListEntry> list;
    std::string text;
    std::string error;
    if (host.purist) {
        log::info("mods: purist mode (--no-mods), none loaded but the built-in ones");
    } else if (read_text(host.list, text) && !parse_list(text, list, error)) {
        log::warn(
            "mods: {} cannot be read ({}): none loaded", log::tidy(host.list.string()), error
        );
        list.clear();
    }
    if (list.empty() && built_in.empty()) {
        if (!host.purist && !text.empty() && error.empty()) {
            write_status(kept, {});
        }
        g_host = nullptr;
        return host.purist ? "none (purist mode)" : "none";
    }
    const fs::path folder = host.list.parent_path();
    std::vector<Candidate> enabled;
    std::vector<Outcome> outcomes;
    for (const ListEntry& e : list) {
        if (!e.enabled) {
            continue;
        }
        // An older copy of a mod the port now carries (the keyboard-mouse mod): not loaded.
        if (std::find(built_in.begin(), built_in.end(), e.id) != built_in.end()) {
            log::info("mod {} not loaded: it is built into OpenRAC now", e.id);
            outcomes.push_back({e.id, "", "refused", "it is built into OpenRAC now", {}});
            continue;
        }
        Candidate c;
        c.entry = e;
        std::string manifest;
        if (!read_text(folder / e.id / "openrac-mod.json", manifest)) {
            c.manifest_error = "it is not installed (no openrac-mod.json in its folder)";
        } else {
            c.has_manifest = parse_manifest(manifest, c.manifest, c.manifest_error);
            if (!c.has_manifest) {
                c.manifest_error = "its openrac-mod.json: " + c.manifest_error;
            }
        }
        enabled.push_back(std::move(c));
    }
    if (enabled.empty() && built_in.empty()) {
        write_status(kept, outcomes);
        g_outcomes = outcomes;
        return "none";
    }
    const Target target{host.game, host.version, platform_key(), OPENRAC_MOD_API_MAJOR, OPENRAC_MOD_API_MINOR,
                        host.scripts != nullptr};
    const auto decisions =
        plan(enabled, target, [&](const std::string& id, const std::string& path) {
            std::error_code e;
            return fs::is_regular_file(folder / id / fs::path(path), e);
        });
    std::vector<std::string> not_loaded;  // refused or failed here: what requires them goes too
    for (std::size_t i = 0; i < enabled.size(); ++i) {
        const Candidate& c = enabled[i];
        const Decision& d = decisions[i];
        Outcome o{c.entry.id, c.has_manifest ? c.manifest.version_text : "", "loaded", "", {}};
        for (const std::string& w : d.warnings) {
            log::warn("mod {}: {}", d.id, w);
        }
        if (!d.load) {
            o.state = "refused";
            o.reason = d.reason;
        } else {
            for (const Requirement& r : c.manifest.requirements) {
                if (std::find(not_loaded.begin(), not_loaded.end(), r.id) != not_loaded.end()) {
                    o.state = "refused";
                    o.reason = std::format("needs mod {}, which did not load", r.id);
                    break;
                }
            }
            if (o.state == "loaded") {
                std::unique_ptr<openrac_mod> mod;
                if (load_one(folder, c, mod, o.state, o.reason)) {
                    o.hooks = mod->hooks;
                    log::info("mod {} {} loaded ({}{})", mod->id, mod->version, mod->file,
                              mod->hooks.empty() ? std::string() : ", hooks " + [&] {
                                  std::string s;
                                  for (const std::string& h : mod->hooks) {
                                      s += (s.empty() ? "" : " ") + h;
                                  }
                                  return s;
                              }());
                    g_loaded.push_back(std::move(mod));
                    // Its assets (MODS.md, section 3): searched before the levels' own files.
                    if (!c.manifest.assets.empty()) {
                        std::error_code e;
                        const fs::path assets = folder / c.manifest.id / fs::path(c.manifest.assets);
                        if (fs::is_directory(assets, e)) {
                            g_asset_roots.insert(g_asset_roots.begin(), assets);
                        } else {
                            log::warn("mod {}: its assets folder {} is missing", c.manifest.id,
                                      c.manifest.assets);
                        }
                    }
                    // Its on-screen pad's buttons (MODS.md, "Touch controls").
                    if (!c.manifest.touch.empty()) {
                        std::string why;
                        if (!touch::load_file(
                                c.manifest.id, folder / c.manifest.id, c.manifest.touch, why
                            )
                            || !why.empty()) {
                            log::warn("mod {}", why);
                        }
                    }
                }
            }
        }
        if (o.state != "loaded") {
            not_loaded.push_back(o.id);
            log::warn("mod {} not loaded: {}", o.id, o.reason);
        }
        outcomes.push_back(std::move(o));
    }
    g_frame_callbacks = !g_frames.empty() || !g_settings_callbacks.empty();
    if (host.pad_hook != nullptr) {
        *host.pad_hook = g_pads.empty() ? nullptr : run_pads;
    }
    if (g_views_mod != nullptr && host.set_views != nullptr) {
        log::info("mod {} draws views of the game", g_views_mod->id);
        host.set_views(&g_views);
    }
    if (g_vulkan_mod != nullptr && host.set_vulkan_setup != nullptr) {
        log::info("mod {} makes the Vulkan device", g_vulkan_mod->id);
        host.set_vulkan_setup(&g_vulkan);
    }
    for (const auto& mod : g_loaded) {
        if (mod->manifest.vulkan_queue) {
            g_vulkan_queue_declared = true;
            log::warn(
                "mod {} requires native Vulkan access; Vulkan runs on the game's thread", mod->id
            );
        }
    }
    g_outcomes = outcomes;
    if (!host.purist && !host.list.empty()) {
        write_status(kept, outcomes);
    }
    g_images.clear();
    for (const auto& m : g_loaded) {
        g_images.push_back({m->id.c_str(), m->version.c_str(), m->file.c_str(), m->base});
    }
    crash::set_mods(g_images.data(), static_cast<int>(g_images.size()));
    set_loaded_mods_json();
    std::string line;
    for (const auto& m : g_loaded) {
        line += (line.empty() ? "" : ", ") + m->id + " " + m->version
                + (m->library == nullptr && !m->script ? " (built in)" : "");
    }
    const std::size_t refused = outcomes.size() - (g_loaded.size() - built_in.size());
    if (line.empty()) {
        line = "none";
    }
    if (refused > 0) {
        line += std::format(" ({} not loaded: plugins/status.json)", refused);
    }
    return line;
}

VulkanStartup vulkan_startup(const fs::path& list_path, bool purist) {
    std::string text;
    std::vector<ListEntry> list;
    std::string error;
    if (purist || !read_text(list_path, text) || !parse_list(text, list, error)) {
        return {};
    }
    VulkanStartup startup;
    const fs::path folder = list_path.parent_path();
    for (const ListEntry& e : list) {
        std::string manifest_text;
        Manifest manifest;
        if (e.enabled && read_text(folder / e.id / "openrac-mod.json", manifest_text)
            && parse_manifest(manifest_text, manifest, error)) {
            startup.creates_device |= manifest.vulkan_device;
            startup.uses_queue |= manifest.vulkan_queue;
        }
    }
    return startup;
}

bool vulkan_needs_game_thread() {
    return g_vulkan_queue_declared
           || (g_views_mod != nullptr && g_views_mod->manifest.api_minor < 20);
}

bool view_source_can_use_vulkan() {
    return g_views_mod != nullptr && native_vulkan_allowed(g_views_mod->manifest);
}

void unload() {
    for (auto it = g_loaded.rbegin(); it != g_loaded.rend(); ++it) {
        if ((*it)->unload != nullptr) {
            (*it)->unload();
        }
    }
}

void run_frame_callbacks() {
    // The game in progress first (1.16): a mod's frame callback then sees its new data.
    follow_saves();
    for (const FrameCallback& c : g_frames) {
        c.fn(c.user);
    }
    follow_list();
}

int menu_entry_count() {
    return static_cast<int>(g_menus.size());
}

std::string menu_entry_label(int index) {
    // An index past the list names nothing.
    if (index < 0 || index >= menu_entry_count()) {
        return {};
    }
    return g_menus[static_cast<std::size_t>(index)].label;
}

bool is_mod_code_link(std::string_view url) {
    constexpr std::string_view kPrefix = "openrac://modset/";
    // A long code in a link: what the launcher's own limit on a set makes of it, and some room.
    constexpr std::size_t kLinkMax = 16 * 1024;
    if (!url.starts_with(kPrefix) || url.size() == kPrefix.size() || url.size() > kLinkMax) {
        return false;
    }
    // A short code (K7M2-9QXR) or a long one (ORMS1. and base64url): nothing a handler could read
    // as more than a code.
    const std::string_view code = url.substr(kPrefix.size());
    return std::all_of(code.begin(), code.end(), [](char c) {
        const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool digit = c >= '0' && c <= '9';
        return letter || digit || c == '-' || c == '_' || c == '.';
    });
}

bool overlay_wanted() {
    return !g_overlays.empty() && overlay_shows_level();
}

void run_overlays(const openrac_overlay& overlay) {
    for (const OverlayCallback& c : g_overlays) {
        c.fn(c.user, &overlay);
    }
}

bool run_menu_page(int index, const openrac_menu_frame& frame) {
    // A page that is no longer there (its mod was taken back) is closed.
    if (index < 0 || index >= menu_entry_count()) {
        return true;
    }
    const MenuCallback& c = g_menus[static_cast<std::size_t>(index)];
    g_drawing = c.mod;
    const int result = c.fn(c.user, &frame);
    g_drawing = nullptr;
    return result != OPENRAC_MENU_STAY;
}

bool takes_cursor(const UiItem& item) {
    return item.kind != OPENRAC_UI_TEXT && (item.flags & OPENRAC_UI_DISABLED) == 0;
}

int ui_page_count() {
    return static_cast<int>(g_ui_pages.size());
}

const UiPage* ui_page(int page) {
    // An id past the pages names none.
    if (page < 0 || page >= ui_page_count()) {
        return nullptr;
    }
    return &g_ui_pages[static_cast<std::size_t>(page)].page;
}

int ui_main_item_count() {
    return static_cast<int>(g_ui_main.size());
}

const UiMainItem* ui_main_item(int index) {
    // An index past the items names none.
    if (index < 0 || index >= ui_main_item_count()) {
        return nullptr;
    }
    return &g_ui_main[static_cast<std::size_t>(index)];
}

int ui_pause_item_count() {
    return static_cast<int>(g_ui_pause.size());
}

const UiPauseItem* ui_pause_item(int index) {
    // An index past the items names none.
    if (index < 0 || index >= ui_pause_item_count()) {
        return nullptr;
    }
    return &g_ui_pause[static_cast<std::size_t>(index)];
}

void ui_set_shown(int page, bool question_open) {
    g_ui_shown = page >= 0 && page < ui_page_count() ? page : -1;
    g_ui_question = g_ui_shown >= 0 && question_open;
}

void ui_set_main_menu(bool up) {
    g_ui_main_menu = up;
}

void ui_set_cursor(int page, int item) {
    // Only a page there is, onto an item it has.
    if (page >= 0 && page < ui_page_count()) {
        UiPage& p = g_ui_pages[static_cast<std::size_t>(page)].page;
        p.cursor = settle_cursor(p, item);
    }
}

void ui_set_choice(int page, int item, std::uint32_t choice) {
    // Only a choice of a page there is, to one of its values.
    if (page < 0 || page >= ui_page_count()) {
        return;
    }
    UiPage& p = g_ui_pages[static_cast<std::size_t>(page)].page;
    if (item >= 0 && item < static_cast<int>(p.items.size())) {
        UiItem& it = p.items[static_cast<std::size_t>(item)];
        if (it.kind == OPENRAC_UI_CHOICE && choice < it.choices.size()) {
            it.choice = choice;
        }
    }
}

void ui_send(int page, openrac_ui_event event) {
    // A page that is no longer there (its mod was taken back) hears nothing.
    if (page < 0 || page >= ui_page_count()) {
        return;
    }
    const UiPageCallback& c = g_ui_pages[static_cast<std::size_t>(page)];
    event.size = sizeof(openrac_ui_event);
    event.cursor = c.page.cursor;
    c.fn(c.user, page, &event);
}

UiRequests take_ui_requests() {
    return std::exchange(g_ui_requests, UiRequests{});
}

void tick_boundary(std::uint32_t tick) {
    if (g_tick_boundaries.empty()) {
        return;
    }
    g_in_tick_boundary = true;
    g_state_restored = false;
    for (const TickBoundaryCallback& callback : g_tick_boundaries) {
        g_tick_mod = callback.mod;
        callback.fn(callback.user, tick);
        if (g_state_restored) {
            break;
        }
    }
    g_tick_mod = nullptr;
    g_in_tick_boundary = false;
}

const std::vector<DeveloperMenu>& developer_menus() {
    return g_developer_menus;
}

void queue_developer_menu_action(openrac_mod* mod, std::uint32_t action) {
    if (mod == nullptr || action == 0) {
        return;
    }
    for (DeveloperMenu& menu : g_developer_menus) {
        if (menu.mod != mod) {
            continue;
        }
        // A locked row (1.28) does nothing, and a plain row to read is not pressed.
        const bool pressable = std::any_of(
            menu.items.begin(), menu.items.end(), [action](const DeveloperMenuItem& item) {
                return item.action == action && !item.locked
                       && !(item.info && item.kind == MenuItemKind::Press);
            }
        );
        if (pressable) {
            menu.actions.push_back(action);
        }
        return;
    }
}

void set_developer_menu_value(openrac_mod* mod, std::uint32_t action, int value) {
    DeveloperMenuItem* item = mod != nullptr ? developer_menu_item(mod, action) : nullptr;
    if (item == nullptr || item->locked) {
        return;
    }
    const bool fits = item->kind == MenuItemKind::Switch
                          ? value == 0 || value == 1
                          : item->kind == MenuItemKind::Choice && value >= 0
                                && value < static_cast<int>(item->values.size());
    if (!fits) {
        return;
    }
    item->value = value;
    queue_developer_menu_action(mod, action);
}

int loaded_count() {
    return static_cast<int>(g_loaded.size());
}

const openrac_api& api_for_tests() {
    return g_api;
}

const char* mod_id(const openrac_mod* mod) {
    return mod != nullptr ? mod->id.c_str() : "a mod";
}

bool holds_hook(const openrac_mod* mod, const char* name) {
    if (mod == nullptr || name == nullptr) {
        return false;
    }
    const char* id = g_host != nullptr ? hook_id(*g_host, name) : name;
    return std::any_of(mod->hooks.begin(), mod->hooks.end(), [&](const std::string& h) {
        return h == id;
    });
}

fs::path mod_file(const openrac_mod* mod, const char* path) {
    if (mod == nullptr || path == nullptr || mod->dir.empty() || !folder_path(path)) {
        return {};
    }
    const fs::path file = mod->dir / fs::path(std::u8string(path, path + std::strlen(path)));
    std::error_code e;
    return fs::is_regular_file(file, e) ? file : fs::path();
}

void reset() {
    unload();
    for (auto it = g_loaded.rbegin(); it != g_loaded.rend(); ++it) {
        undo(it->get());
        if ((*it)->library != nullptr) {
            library::close((*it)->library);
        }
    }
    scripts::reset();
    g_outcomes.clear();
    g_loaded.clear();
    g_vulkan_queue_declared = false;
    g_asset_roots.clear();
    g_frames.clear();
    g_settings_callbacks.clear();
    g_list_seen.reset();
    reset_save_data();
    g_list_wait = 0;
    g_list_unwritten_said = false;
    g_pads.clear();
    g_menus.clear();
    g_overlays.clear();
    g_developer_menus.clear();
    g_ui_pages.clear();
    g_ui_main.clear();
    g_ui_pause.clear();
    g_ui_shown = -1;
    g_ui_question = false;
    g_ui_main_menu = false;
    g_ui_requests = UiRequests{};
    reset_play();
    g_party_replay = false;
    g_tick_boundaries.clear();
    g_tick_mod = nullptr;
    g_in_tick_boundary = false;
    g_state_restored = false;
    g_frame_callbacks = false;
    if (g_host != nullptr && g_host->pad_hook != nullptr) {
        *g_host->pad_hook = nullptr;
    }
    if (g_host != nullptr && g_host->set_views != nullptr) {
        g_host->set_views(nullptr);
    }
    if (g_host != nullptr && g_host->set_vulkan_setup != nullptr) {
        g_host->set_vulkan_setup(nullptr);
    }
    crash::set_mods(nullptr, 0);
    g_images.clear();
    g_host = nullptr;
}

}  // namespace openrac::mods
