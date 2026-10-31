// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>

#include "scene/level.h"

namespace openrac::enhanced::fsr3_detail {
// Last displayed object poses, retaining nodes only while their identities remain present.
class ObjectHistory {
public:
    struct Changes {
        std::size_t inserted = 0, erased = 0;
    };

    Changes update(std::span<const scene::Instance> instances, bool deformation_active);
    const scene::Instance* find(std::uint64_t identity) const;

    void clear() { m_entries.clear(); }

    std::size_t size() const { return m_entries.size(); }

private:
    struct Entry {
        Entry(const scene::Instance& source, bool deformation_active) : instance(source) {
            instance.floats_on_water = source.floats_on_water && deformation_active;
        }

        scene::Instance instance;
        bool mark = false;
    };

    std::unordered_map<std::uint64_t, Entry> m_entries;
    bool m_mark = false;
};
}  // namespace openrac::enhanced::fsr3_detail
