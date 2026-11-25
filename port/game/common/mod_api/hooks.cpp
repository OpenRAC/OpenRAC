// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "hooks": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

// A hook point's signature as asked for: i32 and u32 are one, of the same width and passed the same
// way, and the discs' prototypes differ in them (docs/port/MODS.md, section 4).
std::string hook_signature_form(std::string signature) {
    for (size_t at = signature.find("i32"); at != std::string::npos;
         at = signature.find("i32", at)) {
        signature[at] = 'u';
    }
    return signature;
}

int api_hook(
    openrac_mod* mod, const char* name, const char* signature, void* replacement, void** original
) {
    if (mod == nullptr || mod != g_loading || g_host == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    if (name == nullptr || replacement == nullptr || original == nullptr) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    // Asked for by its id or any alias (an earlier name, a symbol): the hook point's id decides.
    const char* id = hook_id(*g_host, name);
    for (int i = 0; i < g_host->hook_count; ++i) {
        const HookPoint& p = g_host->hooks[i];
        if (p.name == nullptr || std::strcmp(hook_id(*g_host, p.name), id) != 0) {
            continue;
        }
        if (signature == nullptr
            || hook_signature_form(p.signature) != hook_signature_form(signature)) {
            mod->hook_refused = true;
            mod->failure = std::format(
                "hook point {} is {} here, not {}",
                name,
                p.signature,
                signature != nullptr ? signature : "(none)"
            );
            return OPENRAC_HOOK_SIGNATURE;
        }
        // Detours' way: the replacement gets what was there (an earlier mod's, or the game's C).
        void* before = *p.slot;
        *original = before != nullptr ? before : p.original;
        mod->taken.emplace_back(p.slot, before);
        *p.slot = replacement;
        mod->hooks.emplace_back(id);
        const auto listed = [&](const std::string& h) {
            return std::strcmp(hook_id(*g_host, h.c_str()), id) == 0;
        };
        if (std::none_of(mod->manifest.hooks.begin(), mod->manifest.hooks.end(), listed)) {
            log::warn(
                "mod {} takes hook point {}, which its manifest's \"hooks\" does not list",
                mod->id,
                name
            );
        }
        return OPENRAC_HOOK_OK;
    }
    mod->hook_refused = true;
    mod->failure = std::format(
        "this OpenRAC has no hook point {} for {}/{}", name, g_host->game, g_host->version
    );
    return OPENRAC_HOOK_UNKNOWN;
}

int api_on_frame(openrac_mod* mod, openrac_frame_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    g_frames.push_back({fn, user, mod});
    return OPENRAC_HOOK_OK;
}

int api_on_tick_boundary(openrac_mod* mod, openrac_tick_boundary_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    g_tick_boundaries.push_back({fn, user, mod});
    return OPENRAC_HOOK_OK;
}

const HookPoint* find_hook_point(const char* name) {
    if (g_host == nullptr || name == nullptr) {
        return nullptr;
    }
    const char* id = hook_id(*g_host, name);
    for (int i = 0; i < g_host->hook_count; ++i) {
        const HookPoint& p = g_host->hooks[i];
        if (p.name != nullptr && std::strcmp(hook_id(*g_host, p.name), id) == 0) {
            return &p;
        }
    }
    return nullptr;
}

// A hook point's entry (1.21): found as api_hook finds it, by its id or any alias, and its
// signature compared as api_hook compares it. The entry is the function the game calls by the
// point's symbol, hooks.c's, which goes through the slot: the mods' replacements, then the game's.
void* api_hook_entry(const char* name, const char* signature) {
    const HookPoint* p = signature != nullptr ? find_hook_point(name) : nullptr;
    if (p == nullptr || p->symbol == nullptr
        || hook_signature_form(p->signature) != hook_signature_form(signature)) {
        return nullptr;
    }
    return reinterpret_cast<void*>(openrac_guest_function_named(p->symbol));
}

int api_on_pad(openrac_mod* mod, openrac_pad_fn fn, void* user) {
    if (mod == nullptr || mod != g_loading || fn == nullptr) {
        return OPENRAC_HOOK_TOO_LATE;
    }
    g_pads.push_back({fn, user, mod});
    return OPENRAC_HOOK_OK;
}

}  // namespace openrac::mods::detail
