// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "enhanced/fsr3/object_history.h"

#include "common/stats.h"
#include "renderer/render_opt.h"

namespace openrac::enhanced::fsr3_detail {
ObjectHistory::Changes ObjectHistory::update(
    std::span<const scene::Instance> instances, bool deformation_active
) {
    const stats::Scope timing("FSR object history");
    Changes changes;
    // Every unmarked node is erased below, so alternating a bit cannot retain an
    // entry from an older frame and needs no separate pass to reset all markers.
    m_mark = !m_mark;
    if (!renderer::render_opt::enabled(renderer::render_opt::Opt::FsrObjectHistoryReuse)) {
        changes.erased = m_entries.size();
        m_entries.clear();
    }
    for (const auto& instance : instances) {
        const auto [it, inserted] =
            m_entries.try_emplace(instance.temporal_id, instance, deformation_active);
        if (inserted) {
            ++changes.inserted;
        } else {
            // Duplicate identities preserve the reference's last-instance-wins behaviour.
            it->second.instance = instance;
            it->second.instance.floats_on_water = instance.floats_on_water && deformation_active;
        }
        it->second.mark = m_mark;
    }
    changes.erased +=
        std::erase_if(m_entries, [this](const auto& item) { return item.second.mark != m_mark; });
    return changes;
}

const scene::Instance* ObjectHistory::find(std::uint64_t identity) const {
    const auto it = m_entries.find(identity);
    return it == m_entries.end() ? nullptr : &it->second.instance;
}
}  // namespace openrac::enhanced::fsr3_detail
