// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "saves": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

int api_save_data_set(openrac_mod* mod, const void* bytes, std::uint32_t size) {
    if (mod == nullptr || size > OPENRAC_SAVE_DATA_MAX || (bytes == nullptr && size != 0)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (size == 0) {
        g_save_data.erase(mod->id);
        return OPENRAC_HOOK_OK;
    }
    const auto* b = static_cast<const std::uint8_t*>(bytes);
    g_save_data[mod->id].assign(b, b + size);
    return OPENRAC_HOOK_OK;
}

std::uint32_t api_save_data_get(openrac_mod* mod, void* out, std::uint32_t size) {
    const auto data = mod != nullptr ? g_save_data.find(mod->id) : g_save_data.end();
    if (data == g_save_data.end()) {
        return 0;
    }
    const auto length = static_cast<std::uint32_t>(data->second.size());
    if (out != nullptr && size >= length) {
        std::memcpy(out, data->second.data(), length);
    }
    return length;
}

int api_on_save_data(openrac_mod* mod, openrac_frame_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    g_save_data_callbacks.push_back({fn, user, mod});
    return OPENRAC_HOOK_OK;
}

// A save file is read whole: none of a game's is larger.
constexpr std::uintmax_t kSaveMax = 1u << 20;

// The mod that holds the card filter, what it asked, and the card folder its writes go to.
const openrac_mod* g_card_mod = nullptr;

std::string g_card_folder;

// A mod's own card folder's name at most, and the note beside a save (card_note_write).
constexpr std::size_t kCardFolderMost = 16;

constexpr std::size_t kCardNoteMost = 1024;

// The card folder a mod's folder name means: the game's own for "", else the game's own with the
// name after it ("coop": BESCES-50916RATCHET-coop). Empty for a name that is not one word, so a mod
// reaches nothing outside the card.
std::string card_folder(const char* name) {
    if (g_play == nullptr || g_play->save_folder == nullptr || name == nullptr) {
        return {};
    }
    const std::string word = name;
    if (word.size() > kCardFolderMost
        || !std::all_of(word.begin(), word.end(), [](unsigned char c) {
               return std::isalnum(c) != 0 || c == '_';
           })) {
        return {};
    }
    return word.empty() ? std::string(g_play->save_folder) : g_play->save_folder + ("-" + word);
}

// The path of save `slot` in the card folder `folder`; empty past the last slot.
std::filesystem::path save_path(const std::string& folder, int slot) {
    if (g_play == nullptr || g_play->save_file == nullptr || folder.empty() || slot < 0
        || slot >= g_play->save_slots) {
        return {};
    }
    char name[64];
    std::snprintf(name, sizeof name, g_play->save_file, slot);
    return g_cards / folder / name;
}

// The bytes of the file at `path`; empty for none.
std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    std::error_code e;
    const std::uintmax_t size = std::filesystem::file_size(path, e);
    if (e || size == 0 || size > kSaveMax) {
        return {};
    }
    std::ifstream in(path, std::ios::binary);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size))) {
        return {};
    }
    return bytes;
}

// The bytes of save `slot` of the card folder `folder`; empty for none there. A write stopped
// between its two renames (lib/mc.c) leaves the save before it beside it.
std::vector<std::uint8_t> read_save(const std::string& folder, int slot) {
    const std::filesystem::path path = save_path(folder, slot);
    if (path.empty()) {
        return {};
    }
    std::vector<std::uint8_t> bytes = read_file(path);
    if (bytes.empty() && folder != g_play->save_folder) {
        std::filesystem::path previous = path;
        previous += ".bak";
        bytes = read_file(previous);
    }
    return bytes;
}

std::vector<std::uint8_t> read_save(int slot) {
    return g_play != nullptr && g_play->save_folder != nullptr
               ? read_save(g_play->save_folder, slot)
               : std::vector<std::uint8_t>{};
}

// The note beside a save in a mod's folder (card_note_write).
std::filesystem::path note_path(const std::string& folder, int slot) {
    std::filesystem::path path = save_path(folder, slot);
    if (!path.empty()) {
        path += ".party";
    }
    return path;
}

size_t api_save_capture(void* out, size_t size) {
    if (g_play == nullptr) {
        return 0;
    }
    const auto room = static_cast<std::uint32_t>(
        std::min<size_t>(size, std::numeric_limits<std::uint32_t>::max())
    );
    return g_play->save_capture(out != nullptr ? static_cast<std::uint8_t*>(out) : nullptr, room);
}

int api_save_slot_current(void) {
    return g_play != nullptr && g_play->save_slot_current != nullptr ? g_play->save_slot_current()
                                                                     : -1;
}

int api_save_slot(int slot, openrac_save_info* out) {
    if (g_play == nullptr || slot < 0 || slot >= g_play->save_slots) {
        return -1;
    }
    if (out == nullptr || out->size < sizeof(openrac_save_info)) {
        return 0;
    }
    const std::vector<std::uint8_t> bytes = read_save(slot);
    return save_info(bytes.data(), bytes.size(), out) ? 1 : 0;
}

size_t api_save_read(int slot, void* out, size_t size) {
    const std::vector<std::uint8_t> bytes = read_save(slot);
    if (bytes.empty() || !save_info(bytes.data(), bytes.size(), nullptr)) {
        return 0;
    }
    if (out != nullptr && size >= bytes.size()) {
        std::memcpy(out, bytes.data(), bytes.size());
    }
    return bytes.size();
}

int api_card_filter(openrac_mod* mod, int mode, const char* folder) {
    if (mod == nullptr || g_play == nullptr || g_play->card_session == nullptr
        || g_play->card_filter == nullptr || (g_card_mod != nullptr && g_card_mod != mod)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (mode == OPENRAC_CARD_PASS) {
        // Only once no write of the party's can reach the player's saves: the mod asks again.
        if (g_card_mode != OPENRAC_CARD_PASS && g_play->card_session(0) == 0) {
            return OPENRAC_HOOK_TAKEN;
        }
        g_play->card_filter(OPENRAC_CARD_PASS, "");
        g_card_mod = nullptr;
        g_card_mode = OPENRAC_CARD_PASS;
        g_card_folder.clear();
        return OPENRAC_HOOK_OK;
    }
    const std::string named = mode == OPENRAC_CARD_REDIRECT ? card_folder(folder) : std::string();
    if ((mode == OPENRAC_CARD_REDIRECT && (named.empty() || named == g_play->save_folder))
        || g_play->card_filter(mode, named.c_str()) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_card_mod = mod;
    g_card_mode = mode;
    g_card_folder = named;
    g_play->card_session(1);
    log::info("mods: {} keeps the game's saves off the player's card", mod_id(mod));
    return OPENRAC_HOOK_OK;
}

int api_card_slot_hint(openrac_mod* mod, int slot) {
    if (g_play == nullptr || mod != g_card_mod || slot < -1 || slot >= g_play->save_slots) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_play->card_slot(slot);
    return OPENRAC_HOOK_OK;
}

int api_coop_save_write(openrac_mod* mod, int slot, const void* bytes, std::uint32_t size) {
    // Bytes from anywhere, another player's game among them: only what the game would read.
    if (g_play == nullptr || g_play->save_check == nullptr || mod != g_card_mod
        || g_card_mode != OPENRAC_CARD_REDIRECT || slot < 0 || slot >= g_play->save_slots
        || bytes == nullptr || size == 0 || size > kSaveMax
        || g_play->save_check(static_cast<const std::uint8_t*>(bytes), size) == 0) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_play->card_write(slot, static_cast<const std::uint8_t*>(bytes), size) != 0
               ? OPENRAC_HOOK_OK
               : OPENRAC_HOOK_UNKNOWN;
}

int api_save_slot2(const char* folder, int slot, openrac_save_info* out) {
    if (g_play == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (slot < 0 || slot >= g_play->save_slots) {
        return -1;
    }
    if (out == nullptr || out->size < sizeof(openrac_save_info)) {
        return 0;
    }
    const std::vector<std::uint8_t> bytes = read_save(card_folder(folder), slot);
    return save_info(bytes.data(), bytes.size(), out) ? 1 : 0;
}

std::size_t api_save_read2(const char* folder, int slot, void* out, std::size_t size) {
    const std::vector<std::uint8_t> bytes = read_save(card_folder(folder), slot);
    if (bytes.empty() || !save_info(bytes.data(), bytes.size(), nullptr)) {
        return 0;
    }
    if (out != nullptr && size >= bytes.size()) {
        std::memcpy(out, bytes.data(), bytes.size());
    }
    return bytes.size();
}

std::uint32_t api_card_saves() {
    return g_play != nullptr && g_play->card_saves != nullptr ? g_play->card_saves() : 0;
}

int api_card_note_write(openrac_mod* mod, int slot, const char* text) {
    const std::filesystem::path path = mod != nullptr && mod == g_card_mod
                                           ? note_path(g_card_folder, slot)
                                           : std::filesystem::path{};
    if (path.empty() || text == nullptr || std::strlen(text) >= kCardNoteMost) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // A note half written would lose a party its slot: a whole one or the one before.
    std::error_code e;
    std::filesystem::create_directories(path.parent_path(), e);
    std::filesystem::path temp = path;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!(out << text)) {
            return OPENRAC_HOOK_UNKNOWN;
        }
    }
    std::filesystem::rename(temp, path, e);
    return e ? OPENRAC_HOOK_UNKNOWN : OPENRAC_HOOK_OK;
}

int api_card_note_read(const char* folder, int slot, char* out, std::uint32_t size) {
    const std::string named = card_folder(folder);
    if (out == nullptr || size == 0 || named.empty() || named == g_play->save_folder) {
        return 0;
    }
    // (card_folder is empty without a game, so g_play is there.)
    std::ifstream in(note_path(named, slot), std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.empty() || text.size() >= size) {
        return 0;
    }
    std::memcpy(out, text.c_str(), text.size() + 1);
    return static_cast<int>(text.size());
}

int api_mission_state_read(int level, std::uint8_t* out) {
    if (g_play == nullptr || g_play->mission_state_read == nullptr || out == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_read(level, out) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_mission_state_write(int level, const std::uint8_t* in) {
    if (g_play == nullptr || g_play->mission_state_write == nullptr || in == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_write(level, in) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_mission_state_clear(int level, const std::uint8_t* flags) {
    if (g_play == nullptr || g_play->mission_state_clear == nullptr || flags == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return g_play->mission_state_clear(level, flags) != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
