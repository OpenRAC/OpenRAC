// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#include "openrac/openrac_mod.h"

namespace openrac::mods {
// Registrations own no game state. Removal restores the game's default decisions.
class PlayQueries {
    struct Entry {
        openrac_mod* owner;
        uint32_t kinds;
        openrac_play_query_fn fn;
        void* user;
        bool disabled = false;
    };

    std::vector<Entry> entries_;
    bool dispatching_ = false;

public:
    int add(openrac_mod* owner, uint32_t kinds, openrac_play_query_fn fn, void* user) {
        if (!owner || !fn || !kinds || (kinds & ~OPENRAC_PLAY_QUERY_ALL)) {
            return OPENRAC_HOOK_UNKNOWN;
        }

        for (const auto& entry : entries_) {
            if (entry.kinds & kinds) {
                return OPENRAC_HOOK_TAKEN;
            }
        }

        entries_.push_back({owner, kinds, fn, user});
        return OPENRAC_HOOK_OK;
    }

    void remove(openrac_mod* owner) {
        std::erase_if(entries_, [owner](const Entry& entry) { return entry.owner == owner; });
    }

    int32_t query(
        uint32_t kind,
        int32_t level,
        uint32_t subject,
        int32_t index,
        int32_t original,
        int32_t alternate
    ) {
        if (dispatching_ || !kind || (kind & (kind - 1))) {
            return original;
        }

        for (auto& entry : entries_) {
            if (!(entry.kinds & kind) || entry.disabled) {
                continue;
            }

            openrac_play_query query{
                sizeof query, kind, level, subject, index, original, alternate, original
            };
            dispatching_ = true;

            try {
                entry.fn(entry.user, &query);
            } catch (...) {
                entry.disabled = true;
                query.value = original;
            }

            dispatching_ = false;
            const uint32_t booleans =
                OPENRAC_PLAY_QUERY_CHALLENGE_AVAILABLE | OPENRAC_PLAY_QUERY_ITEM_RESPAWN
                | OPENRAC_PLAY_QUERY_MENU_ACTION | OPENRAC_PLAY_QUERY_HERO_OVERRIDE;

            if ((kind & booleans) && query.value != 0 && query.value != 1) {
                return original;
            }

            if ((kind & (OPENRAC_PLAY_QUERY_REVISIT_BOLTS | OPENRAC_PLAY_QUERY_REVISIT_XP))
                && query.value < 0) {
                return original;
            }

            return query.value;
        }

        return original;
    }
};
}  // namespace openrac::mods
