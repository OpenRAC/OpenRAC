// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/frame/frame_record.h"

#include <algorithm>

namespace openrac::renderer {

const char* frame_op_name(FrameOp op) {
    // Names follow the native command vocabulary, not the residual packet encoding.
    switch (op) {
        case FrameOp::World:
            return "World";
        case FrameOp::WorldMobys:
            return "WorldMobys";
        case FrameOp::Mobys:
            return "Mobys";
        case FrameOp::UiQuad:
            return "UiQuad";
        case FrameOp::UiItem:
            return "UiItem";
        case FrameOp::EffectQuad:
            return "EffectQuad";
        case FrameOp::Fill:
            return "Fill";
        case FrameOp::Clear:
            return "Clear";
        case FrameOp::Picture:
            return "Picture";
        case FrameOp::Snapshot:
            return "Snapshot";
        case FrameOp::TargetBegin:
            return "TargetBegin";
        case FrameOp::TargetEnd:
            return "TargetEnd";
        case FrameOp::Scissor:
            return "Scissor";
        case FrameOp::WorldShrubs:
            return "WorldShrubs";
        case FrameOp::EffectParent:
            return "EffectParent";
        case FrameOp::HudSide:
            return "HudSide";
        case FrameOp::Wide:
            return "Wide";
        case FrameOp::PictureBand:
            return "PictureBand";
        case FrameOp::LastFrame:
            return "LastFrame";
    }

    return "Unknown";
}

bool FrameRecording::anchored() const {
    return std::any_of(commands.begin(), commands.end(), [](const FrameCommand& c) {
        return c.anchor != 0;
    });
}

void FrameRecording::clear() {
    serial = 0;
    commands.clear();
    ui_quads.clear();
    ui_items.clear();
    images.clear();
}

std::uint32_t FrameRecorder::push(FrameCommand command) {
    // A game that records and is never captured (no window frame for a long while) keeps the last
    // ones only.
    if (m_open.commands.size() >= kMostCommands) {
        close();
    }
    command.anchor = m_anchor;
    m_open.commands.push_back(command);
    return static_cast<std::uint32_t>(m_open.commands.size() - 1);
}

void FrameRecorder::close() {
    m_anchor = 0;
    if (m_open.empty()) {
        m_open.clear();
        return;
    }
    // The oldest kept goes.
    auto oldest = std::min_element(
        m_kept.begin(), m_kept.end(), [](const FrameRecording& a, const FrameRecording& b) {
            return a.serial < b.serial;
        }
    );
    *oldest = std::move(m_open);
    oldest->serial = ++m_serial;
    m_open = {};
}

const FrameRecording* FrameRecorder::match(std::span<const std::uint32_t> tags) const {
    if (tags.empty()) {
        return nullptr;
    }
    std::vector<std::uint32_t> sorted(tags.begin(), tags.end());
    std::sort(sorted.begin(), sorted.end());
    const FrameRecording* best = nullptr;
    for (const FrameRecording& r : m_kept) {
        if (r.serial == 0 || r.empty() || (best != nullptr && r.serial < best->serial)) {
            continue;
        }
        const bool found =
            std::any_of(r.commands.begin(), r.commands.end(), [&](const FrameCommand& c) {
                return c.anchor != 0 && std::binary_search(sorted.begin(), sorted.end(), c.anchor);
            });
        if (found) {
            best = &r;
        }
    }
    return best;
}

const FrameRecording* FrameRecorder::latest_unanchored() const {
    const FrameRecording* best = nullptr;
    for (const FrameRecording& r : m_kept) {
        if (r.serial != 0 && !r.empty() && !r.anchored()
            && (best == nullptr || r.serial > best->serial)) {
            best = &r;
        }
    }
    return best;
}

void FrameRecorder::reset() {
    m_open.clear();
    for (FrameRecording& r : m_kept) {
        r.clear();
    }
    m_anchor = 0;
}

std::vector<std::size_t> place_commands(
    const FrameRecording& recording, std::span<const std::uint32_t> tags
) {
    std::vector<std::size_t> out;
    out.reserve(recording.commands.size());
    std::size_t at = 0;
    std::size_t previous = 0;

    // Search forward in walk order; paging can place lower addresses after higher ones.
    for (const FrameCommand& c : recording.commands) {
        // An unanchored command inherits even an unreachable predecessor's placement.
        if (c.anchor == 0) {
            out.push_back(previous);
            continue;
        }

        std::size_t found = kDropped;

        // A tag reached only before the previous successful placement cannot draw this command.
        for (std::size_t t = at; t < tags.size(); ++t) {
            if (tags[t] == c.anchor) {
                found = t;
                break;
            }
        }

        // A missing anchor must not prevent a later reachable anchor from resuming the recording.
        if (found != kDropped) {
            at = found;
        }

        previous = found;
        out.push_back(found);
    }
    return out;
}

}  // namespace openrac::renderer
