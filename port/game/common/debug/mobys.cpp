// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The debug overlay's mobys section and its moby inspector: the live mobys near the hero, and the
// fields of the one selected (debug/overlay.h).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <imgui.h>
#include <string>
#include <vector>

#include "debug/overlay.h"
#include "game/common/profile.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

namespace openrac::frontend::debug_overlay {

void mobys_section() {
    // Where this game's mobys keep the fields below: they differ between the games.
    const host::GameProfile& profile = host::game_profile(openrac_game.id);
    const std::uint32_t anim = profile.moby_anim_fields();
    const host::LiveMoby* h = hero();
    text("{} live mobys, {} drawn", s.state.mobys.size(), s.counts.mobys_drawn);
    ImGui::Checkbox("near the hero", &s.near_hero);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(dp(110.0f));
    ImGui::SliderFloat("radius", &s.radius, 2.0f, 200.0f, "%.0f");
    ImGui::Checkbox("labels in the picture", &s.labels);
    ImGui::SameLine();
    bool click_picks = picker_enabled();

    if (ImGui::Checkbox("click picks (mobys, ties, terrain)", &click_picks)) {
        set_picker_enabled(click_picks);
    }

    struct Row {
        const host::LiveMoby* moby;
        float distance;
    };

    std::vector<Row> rows;

    for (const host::LiveMoby& m : s.state.mobys) {
        const float d = h != nullptr ? distance(m, *h) : 0.0f;

        if (!s.near_hero || h == nullptr || d <= s.radius) {
            rows.push_back({&m, d});
        }
    }

    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.distance < b.distance;
    });
    text("{} listed{}", rows.size(), h == nullptr ? " (no hero: all of them)" : "");

    if (ImGui::BeginTable(
            "mobys",
            8,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, dp(190.0f))
        )) {
        ImGui::TableSetupScrollFreeze(0, 1);

        for (const char* column :
             {"address", "class", "state", "mode", "anim", "position", "dist", "update"}) {
            ImGui::TableSetupColumn(column);
        }

        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(rows.size()));

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                const host::LiveMoby& m = *rows[static_cast<std::size_t>(i)].moby;
                const std::uint32_t a = m.address;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if (ImGui::Selectable(
                        std::format("{:06X}", a).c_str(),
                        s.selected == a,
                        ImGuiSelectableFlags_SpanAllColumns
                    )) {
                    s.selected = a;
                    s.follow_hero = false;
                    s.inspector = true;
                }

                ImGui::TableNextColumn();
                const std::string name = class_name(m.class_id);
                text("{} {}", m.class_id, name);
                ImGui::TableNextColumn();
                text("{}", read<std::uint8_t>(a + 0x20));
                ImGui::TableNextColumn();
                text("{:04X}", read<std::uint16_t>(a + 0x34));
                ImGui::TableNextColumn();
                text("{}:{}", read<std::uint8_t>(a + anim + 2), read<std::uint8_t>(a + anim));
                ImGui::TableNextColumn();
                text("{:.1f} {:.1f} {:.1f}", m.matrix[12], m.matrix[13], m.matrix[14]);
                ImGui::TableNextColumn();
                text("{:.1f}", rows[static_cast<std::size_t>(i)].distance);
                ImGui::TableNextColumn();
                const std::uint32_t update = read<std::uint32_t>(a + profile.moby_update());
                const char* fn = update != 0 ? openrac_guest_function_name(update) : nullptr;
                ImGui::TextUnformatted(
                    update == 0     ? "-"
                    : fn != nullptr ? fn
                                    : std::format("{:#x}", update).c_str()
                );
            }
        }

        ImGui::EndTable();
    }
}

void inspector_window() {
    if (!s.inspector) {
        return;
    }

    // Where this game's mobys keep the fields below: they differ between the games.
    const host::GameProfile& profile = host::game_profile(openrac_game.id);
    const std::uint32_t anim = profile.moby_anim_fields();
    const std::uint32_t update_at = profile.moby_update();
    const std::uint32_t pvars_at = profile.moby_pvars();
    const std::uint32_t rotation_at = profile.moby_rotation();
    const std::uint32_t glow_at = profile.moby_glow_word();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Kept in the top right corner, whatever its size becomes.
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 10.0f, viewport->WorkPos.y + 10.0f),
        ImGuiCond_Always,
        ImVec2(1.0f, 0.0f)
    );
    ImGui::SetNextWindowBgAlpha(0.88f);

    if (ImGui::Begin(
            "Moby",
            &s.inspector,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
        )) {
        const host::LiveMoby* m = selected_moby();

        if (m == nullptr) {
            ImGui::TextUnformatted(
                s.selected == 0 ? "none selected: pick one in the Mobys list"
                                : "the selected moby is gone"
            );
        } else {
            const std::uint32_t a = m->address;
            const std::string name = class_name(m->class_id);
            text_coloured(
                kYellow, "{:#010x}   class {} {}", a, m->class_id, name.empty() ? "(no name)" : name
            );

            if (ImGui::BeginTable(
                    "fields", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg
                )) {
                // The sphere the game keeps at +0x00 is in 1/1024 units: the host divides it.
                const auto& sphere = m->world_sphere;
                row("+00 collision sphere",
                    sphere[3] > 0.0f ? std::format(
                                           "{:.2f} {:.2f} {:.2f} radius {:.2f}",
                                           sphere[0],
                                           sphere[1],
                                           sphere[2],
                                           sphere[3]
                                       )
                                     : std::string("none"));
                row("+10 position",
                    std::format(
                        "{:.3f} {:.3f} {:.3f}",
                        read<float>(a + 0x10),
                        read<float>(a + 0x14),
                        read<float>(a + 0x18)
                    ));
                row("+20 state", std::format("{}", read<std::uint8_t>(a + 0x20)));
                row("+2C scale", std::format("{:.4f}", read<float>(a + 0x2C)));
                row("+30 draw bytes",
                    std::format(
                        "{} {} {:#06x}",
                        read<std::uint8_t>(a + 0x30),
                        read<std::uint8_t>(a + 0x31),
                        read<std::uint16_t>(a + 0x32)
                    ));
                const std::uint16_t mode = read<std::uint16_t>(a + 0x34);
                row("+34 mode bits",
                    std::format(
                        "{:#06x}{}{}{}{}",
                        mode,
                        (mode & 1) != 0 ? " hidden" : "",
                        (mode & 0x10) != 0 ? " glow" : "",
                        (mode & 0x100) != 0 ? " own-rows" : "",
                        (mode & 0x8000) != 0 ? " mirrored" : ""
                    ));
                row("+38 light",
                    std::format(
                        "set {} / {} blend {}   ambient {} {} {}",
                        read<std::uint8_t>(a + 0x38),
                        read<std::uint8_t>(a + 0x39),
                        read<std::uint8_t>(a + 0x3A),
                        read<std::uint8_t>(a + 0x3C),
                        read<std::uint8_t>(a + 0x3D),
                        read<std::uint8_t>(a + 0x3E)
                    ));
                row(std::format("+{:02X} rotation", rotation_at).c_str(),
                    std::format(
                        "{:.3f} {:.3f} {:.3f}",
                        read<float>(a + rotation_at),
                        read<float>(a + rotation_at + 4),
                        read<float>(a + rotation_at + 8)
                    ));
                // The animation block: the frame and the next frame, the sequence and the next
                // one, then the time in the frame (one layout in every game, at another place).
                row(std::format("+{:02X} animation", anim).c_str(),
                    std::format(
                        "sequence {} frame {} -> {} frame {}, t {:.3f}",
                        read<std::uint8_t>(a + anim + 2),
                        read<std::uint8_t>(a + anim),
                        read<std::uint8_t>(a + anim + 3),
                        read<std::uint8_t>(a + anim + 1),
                        read<float>(a + anim + 4)
                    ));
                row(std::format("+{:02X} update", update_at).c_str(),
                    function_name(read<std::uint32_t>(a + update_at)));
                row(std::format("+{:02X} pvars", pvars_at).c_str(),
                    std::format("{:#x}", read<std::uint32_t>(a + pvars_at)));
                row(std::format("+{:02X} glow colour", glow_at).c_str(),
                    std::format("{:#08x}", read<std::uint32_t>(a + glow_at) & 0xFFFFFFu));
                row("class", std::format("{}", m->class_id));

                if (const host::LiveMoby* h = hero()) {
                    row("distance to the hero", std::format("{:.2f}", distance(*m, *h)));
                }

                ImGui::EndTable();
            }

            if (ImGui::Button("in the memory viewer")) {
                s.view_address = a;
                s.memory_viewer = true;
            }

            const std::uint32_t pvars = read<std::uint32_t>(a + pvars_at);

            if (pvars != 0 && in_ram(pvars, 0x40)) {
                ImGui::SameLine();

                if (ImGui::Button("its pvars")) {
                    s.view_address = pvars & 0x01FFFFF0;
                    s.memory_viewer = true;
                    s.view_words = true;
                }
            }

            if (ImGui::TreeNode("all 0x100 bytes, as words and floats")) {
                for (std::uint32_t at = 0; at < kMobySize; at += 16) {
                    std::string out = std::format("+{:02X} ", at);

                    for (std::uint32_t i = 0; i < 16; i += 4) {
                        out += std::format(" {:08X}", read<std::uint32_t>(a + at + i));
                    }

                    out += " ";

                    for (std::uint32_t i = 0; i < 16; i += 4) {
                        const float f = read<float>(a + at + i);
                        out += std::isfinite(f) && std::fabs(f) < 1.0e6f
                                       && (std::fabs(f) > 1.0e-4f || f == 0.0f)
                                   ? std::format(" {:9.3f}", f)
                                   : std::string("         .");
                    }

                    ImGui::TextUnformatted(out.c_str());
                }

                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
}

}  // namespace openrac::frontend::debug_overlay
