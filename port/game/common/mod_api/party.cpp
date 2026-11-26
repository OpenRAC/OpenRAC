// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "party": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

openrac_mod* loaded_mod(int index) {
    return index >= 0 && static_cast<std::size_t>(index) < g_loaded.size()
               ? g_loaded[static_cast<std::size_t>(index)].get()
               : nullptr;
}

int api_mod_count() {
    return static_cast<int>(g_loaded.size());
}

int api_mod_entry(int index, openrac_mod_entry* out) {
    const openrac_mod* mod = loaded_mod(index);
    if (mod == nullptr || out == nullptr || out->size < sizeof(openrac_mod_entry)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    out->id = mod->id.c_str();
    out->name = mod->manifest.name.c_str();
    out->version = mod->version.c_str();
    out->party = mod->manifest.party;
    out->settings = static_cast<std::int32_t>(mod->manifest.settings.size());
    return OPENRAC_HOOK_OK;
}

// The same in a player's words: a switch's on or off, a choice's name.
std::string setting_shown(const Setting& s, const SettingValue& v) {
    if (s.type == SettingType::Bool) {
        return v.b ? "on" : "off";
    }
    if (s.type == SettingType::Choice) {
        const auto at = std::find(s.choices.begin(), s.choices.end(), v.s);
        if (at != s.choices.end()) {
            return s.choice_names[static_cast<std::size_t>(at - s.choices.begin())];
        }
    }
    return setting_text(v);
}

int api_mod_setting(int index, int setting, openrac_mod_setting* out) {
    openrac_mod* mod = loaded_mod(index);
    if (mod == nullptr || out == nullptr || out->size < sizeof(openrac_mod_setting) || setting < 0
        || static_cast<std::size_t>(setting) >= mod->manifest.settings.size()) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    if (mod->party_settings.empty()) {
        for (const Setting& s : mod->manifest.settings) {
            const auto v = mod->settings.find(s.key);
            const SettingValue& value = v != mod->settings.end() ? v->second : s.fallback;
            mod->party_settings.push_back({setting_text(value), setting_shown(s, value)});
        }
    }
    const Setting& s = mod->manifest.settings[static_cast<std::size_t>(setting)];
    const openrac_mod::PartySetting& text = mod->party_settings[static_cast<std::size_t>(setting)];
    out->key = s.key.c_str();
    out->name = s.name.c_str();
    out->value = text.value.c_str();
    out->shown = text.shown.c_str();
    out->party = s.party;
    out->is_default = text.value == setting_text(s.fallback) ? 1 : 0;
    return OPENRAC_HOOK_OK;
}

int api_party_rules_publish(openrac_mod* mod, const void* data, uint32_t size, const char* text) {
    if (mod == nullptr || (data == nullptr && size != 0) || size > OPENRAC_PARTY_RULES_MAX) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    mod->party_rules.assign(bytes, bytes + size);
    mod->party_text = text != nullptr ? text : "";
    mod->has_party_rules = true;
    ++mod->party_version;
    return OPENRAC_HOOK_OK;
}

int api_party_rules_read(int index, void* out, uint32_t size, uint32_t* version) {
    const openrac_mod* mod = loaded_mod(index);
    if (mod == nullptr || !mod->has_party_rules) {
        return -1;
    }
    if (out != nullptr && !mod->party_rules.empty()) {
        std::memcpy(
            out, mod->party_rules.data(), std::min<std::size_t>(size, mod->party_rules.size())
        );
    }
    if (version != nullptr) {
        *version = mod->party_version;
    }
    return static_cast<int>(mod->party_rules.size());
}

const char* api_party_rules_text(int index) {
    const openrac_mod* mod = loaded_mod(index);
    return mod != nullptr ? mod->party_text.c_str() : "";
}

int api_on_party_rules(openrac_mod* mod, openrac_party_rules_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    mod->party_fn = fn;
    mod->party_user = user;
    return OPENRAC_HOOK_OK;
}

int api_party_rules_deliver(int index, const void* data, uint32_t size) {
    const openrac_mod* mod = loaded_mod(index);
    if (mod == nullptr || mod->party_fn == nullptr || size > OPENRAC_PARTY_RULES_MAX
        || (data == nullptr && size != 0)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    mod->party_fn(mod->party_user, data, size);
    return OPENRAC_HOOK_OK;
}

void api_party_replay(int on) {
    g_party_replay = on != 0;
}

int api_party_replaying() {
    return g_party_replay ? 1 : 0;
}

// The launcher's key for this installation (launcher/core/src/multiplayer.rs): a game started
// without the launcher has none, and cannot be banned.
int api_identity_key(char* out, std::uint32_t size) {
    const std::optional<std::string_view> key = env::value(env::Var::IdentityKey);
    if (out == nullptr || size == 0 || !key) {
        return 0;
    }
    const std::size_t length = key->size();
    // A key cut short is another key: none rather than that.
    if (length == 0 || length >= size) {
        return 0;
    }
    std::memcpy(out, key->data(), length);
    out[length] = '\0';
    return static_cast<int>(length);
}

int api_shared_read(int tag, void* out, std::uint32_t size) {
    if (g_play == nullptr || g_play->shared_read == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return out != nullptr ? g_play->shared_read(tag, out, size) : -1;
}

int api_shared_give(int tag, const void* data, std::uint32_t size) {
    if (g_play == nullptr || g_play->shared_give == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    return data != nullptr && g_play->shared_give(tag, data, size) != 0 ? OPENRAC_HOOK_OK
                                                                        : OPENRAC_HOOK_UNKNOWN;
}

int api_world_family(char* out, std::uint32_t size) {
    if (g_play == nullptr || g_play->world_family == nullptr || out == nullptr || size == 0) {
        return 0;
    }

    const std::size_t length = std::strlen(g_play->world_family);

    if (length >= size) {
        return 0;
    }

    std::memcpy(out, g_play->world_family, length + 1);
    return static_cast<int>(length);
}

int api_sim_digest(std::int32_t level, std::uint32_t* out) {
    if (g_play == nullptr || g_play->sim_digest == nullptr || out == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }

    *out = g_play->sim_digest(level);
    return *out != 0 ? OPENRAC_HOOK_OK : OPENRAC_HOOK_UNKNOWN;
}

}  // namespace openrac::mods::detail
