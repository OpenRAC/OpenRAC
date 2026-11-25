// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "system": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

// The data packs a party has to agree on: stack.json's layers that change the game, once. The
// strings live as long as the program (the SDK's openrac_pack_entry says so).
struct PackEntry {
    std::string id;
    std::string name;
    std::string version;
    std::uint32_t digest = 0;
};

const std::vector<PackEntry>& stacked_packs() {
    static std::vector<PackEntry> packs;
    static std::filesystem::path read;
    if (g_host == nullptr || g_host->stack == read) {
        return packs;
    }
    read = g_host->stack;
    packs.clear();
    json::Value stack;
    std::string error;
    if (read.empty() || !std::filesystem::exists(read)) {
        return packs;
    }
    if (!json::parse_file(read.string(), stack, error) || !stack["layers"].is_array()) {
        log::warn(
            "mods: {} cannot be read ({}): no data packs for a party to compare",
            log::tidy(read.string()),
            error
        );
        return packs;
    }
    for (const json::Value& layer : stack["layers"].items()) {
        if (!layer["gameplay"].boolean() || !layer["id"].is_string()) {
            continue;
        }
        PackEntry pack;
        pack.id = layer["id"].string();
        pack.name = layer["name"].is_string() ? layer["name"].string() : pack.id;
        pack.version = layer["version"].is_string() ? layer["version"].string() : "";
        // The hash's first bytes: what a party compares of it. A stack from before them has none.
        if (layer["digest"].is_string() && layer["digest"].string().size() >= 8) {
            pack.digest = static_cast<std::uint32_t>(
                std::strtoul(layer["digest"].string().substr(0, 8).c_str(), nullptr, 16)
            );
        }
        packs.push_back(std::move(pack));
    }
    return packs;
}

int api_packs_info(openrac_packs_info* out) {
    if (out == nullptr || out->size < sizeof(openrac_packs_info) || g_host == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    std::uint32_t hash = 2166136261u;
    const auto mix = [&hash](std::string_view text) {
        for (const char c : text) {
            hash = (hash ^ static_cast<std::uint8_t>(c)) * 16777619u;
        }
        hash = (hash ^ 0xFFu) * 16777619u;
    };
    const std::vector<PackEntry>& packs = stacked_packs();
    for (const PackEntry& pack : packs) {
        mix(pack.id);
        mix(pack.version);
        mix(std::to_string(pack.digest));
    }
    out->count = static_cast<std::int32_t>(packs.size());
    out->digest = hash;
    return OPENRAC_HOOK_OK;
}

int api_packs_entry(int index, openrac_pack_entry* out) {
    const std::vector<PackEntry>& packs = stacked_packs();
    constexpr std::size_t kWithoutName = offsetof(openrac_pack_entry, name);
    if (out == nullptr || out->size < kWithoutName || index < 0
        || static_cast<std::size_t>(index) >= packs.size()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const PackEntry& pack = packs[static_cast<std::size_t>(index)];
    out->id = pack.id.c_str();
    out->version = pack.version.c_str();
    out->digest = pack.digest;
    if (out->size >= sizeof(openrac_pack_entry)) {
        out->name = pack.name.c_str();
    }
    return OPENRAC_HOOK_OK;
}

int api_clipboard_set(const char* text) {
    if (text == nullptr || g_host == nullptr || g_host->set_clipboard == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_host->set_clipboard(text) ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

int api_open_link(const char* url) {
    if (url == nullptr || !is_mod_code_link(url) || g_host == nullptr
        || g_host->open_url == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_host->open_url(url) ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
