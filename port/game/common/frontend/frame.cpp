// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// One tick of the windowed game, a slice of frontend/state.h: frame() and its steps, in the order
// it runs them. It reads the window's events, uses the world the game draws, reads the game's state
// and its camera, follows a scripted run (OPENRAC_TRACE, OPENRAC_CONTROL_FRAMES), logs and dumps
// what the switches ask for, poses the live mobys, lights the scene, keeps the tick (Kept) with
// its 2D path (direct_packets, the display list's direct GIF data), its effects and its quads, and
// draws the tick's own frame or leaves it to between (frontend/interpolation.cpp draws the kept
// tick). The game addresses it reads are rac1's until each game's profile answers them.
//
// Only the thread that opened the window runs it, once a tick, as with the state it reads
// (state.h).

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "common/env.h"
#include "common/log.h"
#include "common/parallel.h"
#include "common/profiler.h"
#include "common/stats.h"
#include "debug_menu.h"
#include "debug_overlay.h"
#ifdef OPENRAC_LIGHT_AUTHORING
#include "enhanced/lighting/object_lights.h"
#endif
#include "frontend.h"
#include "game/common/frontend/draw_coverage.h"
#include "game/common/frontend/effects_dump.h"
#include "game/common/frontend/hero_shadows.h"
#include "game/common/frontend/menu_layers.h"
#include "game/common/frontend/state.h"
#include "game/common/frontend/water_interactions.h"
#include "mod_menu.h"
#include "mod_ui.h"
#include "mods.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "options_menu.h"
#include "platform/performance_hint.h"
#include "platform/pump.h"
#include "renderer/direct.h"
#include "renderer/renderer.h"
#include "renderer/shadows.h"
#include "renderer/world/fog.h"
#include "scene/hero_shadow.h"
#include "scene/level_renderer.h"
#include "scene/moby_pose.h"
#include "scene/moby_shadow.h"

namespace openrac::frontend {

namespace {

// Bound consecutive omitted pictures so a heavily loaded game still presents regularly.
constexpr int kMostSkipped = 3;

// A range of frames, both ends included.
struct FrameRange {
    std::uint64_t first = 0;
    std::uint64_t last = 0;

    // Returns whether `frame` is in the range.
    [[nodiscard]] bool holds(std::uint64_t frame) const { return frame >= first && frame <= last; }
};

// Returns the range `var` names, N or N-M, read as std::strtoull reads each end; nothing when it is
// unset. For OPENRAC_DUMP_DRAWS and OPENRAC_FRAME_LOG.
std::optional<FrameRange> env_frames(env::Var var) {
    const std::optional<std::string> text = env_string(var);
    if (!text) {
        return std::nullopt;
    }

    // N alone is the range of one frame.
    char* end = nullptr;
    FrameRange range;
    range.first = std::strtoull(text->c_str(), &end, 10);
    range.last = *end == '-' ? std::strtoull(end + 1, nullptr, 10) : range.first;
    return range;
}

std::uint32_t word_at(std::span<const std::uint8_t> ram, std::uint32_t at) {
    std::uint32_t w = 0;
    at = openrac_guest_ram_address_for_size(at, static_cast<std::uint32_t>(ram.size()));
    if (at + 4 <= ram.size()) {
        std::memcpy(&w, ram.data() + at, 4);
    }
    return w;
}

/*
 * The GIF data a frame's display list sends straight to the GS (VIF DIRECT): the 2D path (HUD,
 * menus, text, fades). The chain is followed as the DMA controller follows a source chain (the
 * tag's upper half goes to VIF, as the game's lists are sent with TTE), and the VIF codes that feed
 * VU1 (UNPACK, MPG and the rest) are stepped over by their sizes: what VU1 draws, the world, has
 * renderers of its own.
 */
// An effect quad's marker in the DIRECT data (openrac_game_effect_quad_marker) and the first
// context's TEST and ZBUF in effect there; zbuf_seen when the list had set ZBUF by then.
struct QuadMark {
    std::uint32_t marker;
    std::uint64_t test, zbuf;
    bool zbuf_seen;
};

std::vector<std::uint8_t> direct_packets(
    std::span<const std::uint8_t> ram,
    std::uint32_t chain,
    std::vector<renderer::EffectVertex>* strip_vertices = nullptr,
    std::vector<renderer::EffectStrip>* strips = nullptr,
    std::vector<renderer::ImageUpload>* uploads = nullptr,
    std::size_t* before_world = nullptr,
    std::size_t* first_offscreen = nullptr,
    std::size_t* world_marker = nullptr,
    std::size_t* moby_marker = nullptr,
    std::vector<QuadMark>* quad_marks = nullptr,
    std::vector<State::MarkedMobys>* texture_mobys = nullptr
) {
    std::vector<std::uint8_t> vif;
    std::uint32_t tag_at =
        openrac_guest_ram_address_for_size(chain, static_cast<std::uint32_t>(ram.size())) & ~0xFu;
    std::uint32_t stack[2] = {0, 0};
    int depth = 0;
    // For OPENRAC_DRAW_COVERAGE, where each run of the VIF data came from in game memory (the
    // scratchpad's as 0x70000000 on): its offset in `vif`, its address.
    const bool covering = strips != nullptr && draw_coverage::armed();
    std::vector<std::pair<std::size_t, std::uint32_t>> origins;
    auto append = [&](std::uint32_t at, std::uint32_t bytes) {
        at = openrac_guest_ram_address_for_size(at, static_cast<std::uint32_t>(ram.size()));
        if (at + bytes <= ram.size()) {
            if (covering) {
                origins.emplace_back(vif.size(), at);
            }
            vif.insert(vif.end(), ram.data() + at, ram.data() + at + bytes);
        }
    };
    // A tag whose address has bit 31 set sends its data from the scratchpad (16 KB at 0x70000000
    // in game memory), where the game builds some packets (the page menus' 2D setup among them).
    auto append_scratchpad = [&](std::uint32_t at, std::uint32_t bytes) {
        at &= 0x3FF0;
        const std::uint8_t* spr = static_cast<const std::uint8_t*>(G(0x70000000u));
        if (at + bytes <= 0x4000u) {
            if (covering) {
                origins.emplace_back(vif.size(), 0x70000000u | at);
            }
            vif.insert(vif.end(), spr + at, spr + at + bytes);
        }
    };
    for (int steps = 0; tag_at != 0 && steps < 20000; ++steps) {
        const std::uint32_t lo = word_at(ram, tag_at);
        const std::uint32_t raw_addr = word_at(ram, tag_at + 4);
        const bool from_spr = (raw_addr & 0x80000000u) != 0;
        const std::uint32_t addr = raw_addr & 0x7FFFFFF0;
        const std::uint32_t qwc = lo & 0xFFFF;
        const std::uint32_t id = (lo >> 28) & 7;
        append(tag_at + 8, 8);  // the two VIF codes in the tag
        const std::uint32_t after = tag_at + 16;
        if (from_spr && id != 0 && id != 3 && id != 4) {
            break;  // a chain continuing in the scratchpad: not followed
        }
        switch (id) {
            case 0:  // refe
                from_spr ? append_scratchpad(addr, qwc * 16) : append(addr, qwc * 16);
                tag_at = 0;
                break;
            case 1:  // cnt
                append(after, qwc * 16);
                tag_at = after + qwc * 16;
                break;
            case 2:  // next
                append(after, qwc * 16);
                tag_at = addr;
                break;
            case 3:  // ref
            case 4:  // refs
                from_spr ? append_scratchpad(addr, qwc * 16) : append(addr, qwc * 16);
                tag_at = after;
                break;
            case 5:  // call
                append(after, qwc * 16);
                if (depth < 2) {
                    stack[depth++] = after + qwc * 16;
                }
                tag_at = addr;
                break;
            case 6:  // ret
                append(after, qwc * 16);
                tag_at = depth > 0 ? stack[--depth] : 0;
                break;
            default:  // end
                append(after, qwc * 16);
                tag_at = 0;
                break;
        }
    }

    if (const std::optional<std::string> dump = env_string(env::Var::DumpVif)) {
        if (std::FILE* f = std::fopen(dump->c_str(), "wb")) {
            std::fwrite(vif.data(), 1, vif.size(), f);
            std::fclose(f);
        }
    }
    std::vector<std::uint8_t> gif;

    // The GS registers the strips are drawn with, as the DIRECT data before them leaves them (A+D
    // writes), and the strip being unpacked: its GIF tag at VU address 0, then ST (V2-32) at 1,
    // RGBA (V4-8) at 2 and positions (V3-32) at 3, drawn by an MSCAL (func_L00_001FDE48's packet).
    // The GS keeps two contexts of TEX0, TEX1, CLAMP, ALPHA and TEST: a strip whose PRIM has CTXT
    // set is drawn with the second (Oltanis's ice draws its second layer so, another texture
    // blended half over the first).
    struct Context {
        std::uint64_t tex0 = 0, tex1 = 0, clamp = 0, alpha = 0x44, test = 0, zbuf = 0;
        bool zbuf_seen = false;
    };

    std::array<Context, 2> contexts{};
    std::uint64_t reg_prim = 0, reg_fogcol = 0;
    bool strip_context = false;
    // The fog line the liquid grid's packet gives the strip program (build_interpolated_dma_packet,
    // func_L02_002A52D0, unpacks it to VU addresses 0x3AC to 0x3AF): the view context's fog_mul
    // (0x3AC.w), screen_bias with fog_add in w (0x3AE), then the far and near F (0x3AF.x, .y).
    // A strip whose GIF tag turns the GS fog on gets F per vertex from it, as the world programs
    // compute theirs (renderer/world/fog.h). Bit i of vu_fog_set: VU address 0x3AC + i unpacked.
    std::array<std::uint32_t, 16> vu_fog{};
    unsigned vu_fog_set = 0;
    bool strip_fog = false;
    bool strip_blend_disabled = false;
    const std::uint8_t* strip_st = nullptr;
    const std::uint8_t* strip_rgba = nullptr;
    const std::uint8_t* strip_xyz = nullptr;
    std::uint32_t strip_count = 0;
    bool strip_tag = false;
    std::uint32_t surface = 0;
    renderer::EffectUvMotion uv_motion{};
    std::array<float, 4> uv_offsets{};
    bool second_texture = false;
    // The images sent through the DIRECT data (BITBLTBUF, TRXREG, then IMAGE data), latest per
    // destination block: a strip takes the ones at its TEX0's blocks.
    std::uint64_t reg_bitbltbuf = 0, reg_trxreg = 0;
    std::unordered_map<std::uint32_t, renderer::ImageUpload> images;
    renderer::ImageUpload* filling = nullptr;
    std::size_t filling_bytes = 0;
    const auto start_image = [&]() {
        renderer::ImageUpload image;
        image.dbp = static_cast<std::uint32_t>((reg_bitbltbuf >> 32) & 0x3FFF);
        image.dbw = static_cast<std::uint32_t>((reg_bitbltbuf >> 48) & 0x3F);
        image.dpsm = static_cast<std::uint8_t>((reg_bitbltbuf >> 56) & 0x3F);
        image.width = static_cast<std::uint32_t>(reg_trxreg & 0xFFF);
        image.height = static_cast<std::uint32_t>((reg_trxreg >> 32) & 0xFFF);
        const std::uint32_t bits = image.dpsm == 0x00                         ? 32
                                   : image.dpsm == 0x01                       ? 24
                                   : image.dpsm == 0x02 || image.dpsm == 0x0A ? 16
                                   : image.dpsm == 0x13                       ? 8
                                   : image.dpsm == 0x14                       ? 4
                                                                              : 32;
        filling_bytes = static_cast<std::size_t>(image.width) * image.height * bits / 8;
        renderer::ImageUpload& slot = images[image.dbp];
        slot = std::move(image);
        filling = &slot;
    };
    // Each context's FRAME width in pixels, as the list sets it (the picture's until it does).
    std::array<std::uint32_t, 2> frame_width{512, 512};
    std::size_t image_left = 0;  // IMAGE data a tag announced that runs on into the next DIRECT
    // The strips so far at the world's marker and at the world's mobys' (OPENRAC_WORLD_MOBYS_MARKER):
    // the ones between were sent from the draw callback lists before the mobys.
    const std::size_t strips_at_start = strips != nullptr ? strips->size() : 0;
    std::optional<std::size_t> strips_at_world, strips_at_mobys;
    std::size_t direct_at = 0;   // where the DIRECT being read starts in the 2D data
    const auto track_direct = [&](const std::uint8_t* d, std::size_t size) {
        std::size_t p = 0;
        if (image_left > 0) {
            const std::size_t bytes = std::min(image_left, size);
            if (filling != nullptr && filling->data.size() < filling_bytes) {
                const std::size_t take = std::min(bytes, filling_bytes - filling->data.size());
                filling->data.insert(filling->data.end(), d, d + take);
            }
            image_left -= bytes;
            p = bytes;
        }
        while (p + 16 <= size) {
            std::uint64_t lo, hi;
            std::memcpy(&lo, d + p, 8);
            std::memcpy(&hi, d + p + 8, 8);
            p += 16;
            const std::uint32_t nloop = static_cast<std::uint32_t>(lo & 0x7FFF);
            const std::uint32_t flg = static_cast<std::uint32_t>((lo >> 58) & 3);
            const std::uint32_t nreg = static_cast<std::uint32_t>((lo >> 60) & 0xF) == 0
                                           ? 16
                                           : static_cast<std::uint32_t>((lo >> 60) & 0xF);
            if (flg == 0) {
                for (std::uint32_t i = 0; i < nloop * nreg && p + 16 <= size; ++i, p += 16) {
                    if (((hi >> (4 * (i % nreg))) & 0xF) != 0xE) {
                        continue;
                    }
                    std::uint64_t value, address;
                    std::memcpy(&value, d + p, 8);
                    std::memcpy(&address, d + p + 8, 8);
                    switch (address & 0xFF) {
                        case 0x7F:
                            if ((value >> 48) == renderer::GifInterpreter::kWorldMarker
                                && !strips_at_world) {
                                strips_at_world = strips->size();
                            }
                            if ((value >> 48) == OPENRAC_WORLD_MOBYS_MARKER && !strips_at_mobys) {
                                strips_at_mobys = strips->size();
                            }
                            if ((value >> 48) == renderer::GifInterpreter::kMobyMarker
                                && texture_mobys != nullptr) {
                                // Mobys drawn while the frame is a buffer narrower than the
                                // picture go into a texture (the cockpit screen's Fizzwidget,
                                // 256 x 256 at page 0x1DF), whatever their camera's field of view.
                                texture_mobys->push_back(
                                    {static_cast<std::uint32_t>(value),
                                     static_cast<std::uint32_t>(value >> 32) & 0xFFFFu,
                                     frame_width[(reg_prim >> 9) & 1] < 512}
                                );
                            }
                            if ((value >> 48) == OPENRAC_EFFECT_QUAD_MARKER) {
                                if (quad_marks != nullptr) {
                                    quad_marks->push_back(
                                        {static_cast<std::uint32_t>(value),
                                         contexts[0].test,
                                         contexts[0].zbuf,
                                         contexts[0].zbuf_seen}
                                    );
                                }
                            } else if ((value >> 48) == 0x5754) {
                                surface = static_cast<std::uint32_t>(value);
                                uv_motion = {};
                                uv_offsets = {};
                            } else if ((value >> 48) == 0x5755) {
                                const auto field = static_cast<unsigned>((value >> 32) & 0xFFFF);
                                const auto bits = static_cast<std::uint32_t>(value);
                                if (field == 0) {
                                    uv_motion.enabled = true;
                                    uv_motion.amplitude = std::bit_cast<float>(bits);
                                } else if (field == 1) {
                                    uv_motion.phase = bits;
                                } else if (field < 6) {
                                    uv_offsets[field - 2] = std::bit_cast<float>(bits);
                                }
                            }
                            break;
                        case 0x50:
                            reg_bitbltbuf = value;
                            break;
                        case 0x52:
                            reg_trxreg = value;
                            break;
                        case 0x53:
                            if ((value & 3) == 0) {
                                start_image();
                            }
                            break;
                        case 0x06:
                        case 0x07:
                            contexts[(address & 0xFF) - 0x06].tex0 = value;
                            break;
                        case 0x08:
                        case 0x09:
                            contexts[(address & 0xFF) - 0x08].clamp = value;
                            break;
                        case 0x14:
                        case 0x15:
                            contexts[(address & 0xFF) - 0x14].tex1 = value;
                            break;
                        case 0x00:
                            reg_prim = value;
                            break;
                        case 0x3D:
                            reg_fogcol = value;
                            break;
                        case 0x42:
                        case 0x43:
                            contexts[(address & 0xFF) - 0x42].alpha = value;
                            break;
                        case 0x47:
                        case 0x48:
                            contexts[(address & 0xFF) - 0x47].test = value;
                            break;
                        case 0x4E:
                        case 0x4F:
                            contexts[(address & 0xFF) - 0x4E].zbuf = value;
                            contexts[(address & 0xFF) - 0x4E].zbuf_seen = true;
                            break;
                        case 0x4C:
                        case 0x4D:
                            frame_width[(address & 0xFF) - 0x4C] =
                                static_cast<std::uint32_t>((value >> 16) & 0x3F) * 64;
                            // A FRAME away from the draw buffer (page 0) and the display buffer (page
                            // 0x70): the first render to a texture.
                            if (first_offscreen != nullptr && *first_offscreen == SIZE_MAX
                                && (value & 0x1FF) != 0 && (value & 0x1FF) != 0x70) {
                                *first_offscreen = direct_at;
                            }
                            break;
                        default:
                            break;
                    }
                }
            } else if (flg == 1) {
                p += ((static_cast<std::size_t>(nloop) * nreg + 1) / 2) * 16;
            } else {
                const std::size_t bytes = std::min(static_cast<std::size_t>(nloop) * 16, size - p);
                // Image data runs on into the next DIRECT only for a transfer the game opened (as
                // the direct renderer reads it): without one this is not GIF data, and the next
                // DIRECT is read from its first tag.
                const bool open = filling != nullptr && filling->data.size() < filling_bytes;
                image_left = open ? static_cast<std::size_t>(nloop) * 16 - bytes : 0;
                if (open) {
                    const std::size_t take = std::min(bytes, filling_bytes - filling->data.size());
                    filling->data.insert(filling->data.end(), d + p, d + p + take);
                }
                p += bytes;
            }
        }
    };
    const auto origin = [&](std::size_t offset) -> std::uint32_t {
        const auto it = std::upper_bound(
            origins.begin(), origins.end(), offset, [](std::size_t o, const auto& run) {
                return o < run.first;
            }
        );
        return it == origins.begin()
                   ? 0
                   : std::prev(it)->second
                         + static_cast<std::uint32_t>(offset - std::prev(it)->first);
    };
    std::size_t at = 0;
    while (at + 4 <= vif.size()) {
        std::uint32_t code;
        std::memcpy(&code, vif.data() + at, 4);
        at += 4;
        const std::uint32_t command = (code >> 24) & 0x7F;
        const std::uint32_t imm = code & 0xFFFF;
        const std::uint32_t num = (code >> 16) & 0xFF;
        if (command == 0x20) {
            at += 4;
        } else if (command == 0x30 || command == 0x31) {
            at += 16;
        } else if (command == 0x4A) {
            const std::size_t size = std::size_t{num == 0 ? 256u : num} * 8;
            if (covering && at + size <= vif.size()) {
                draw_coverage::program(imm, {vif.data() + at, size});
            }
            at += size;
        } else if (command == 0x50 || command == 0x51) {
            const std::size_t size = std::size_t{imm == 0 ? 65536u : imm} * 16;
            if (at + size > vif.size()) {
                break;
            }
            // Each DIRECT's data stays a DIRECT of its own (NOP NOP NOP DIRECT, then the data), so
            // the direct renderer reads every one from its first tag.
            const std::uint32_t head[4] = {0, 0, 0, 0x50000000u | (imm & 0xFFFF)};
            direct_at = gif.size();
            // The world's marker (renderer::GifInterpreter::kWorldMarker): one PACKED A+D write to
            // the NOP register, put where the game's terrain pass runs.
            if (world_marker != nullptr && *world_marker == SIZE_MAX && imm == 2) {
                std::uint64_t value, address;
                std::memcpy(&value, vif.data() + at + 16, 8);
                std::memcpy(&address, vif.data() + at + 24, 8);
                if ((address & 0xFF) == 0x7F
                    && (value >> 48) == renderer::GifInterpreter::kWorldMarker) {
                    *world_marker = direct_at;
                }
            }
            // The first moby marker (renderer::GifInterpreter::kMobyMarker): where the game drew
            // mobys of a list of their own (GameProfile::own_mobys_split_the_2d).
            if (moby_marker != nullptr && *moby_marker == SIZE_MAX && imm == 2) {
                std::uint64_t value, address;
                std::memcpy(&value, vif.data() + at + 16, 8);
                std::memcpy(&address, vif.data() + at + 24, 8);
                if ((address & 0xFF) == 0x7F
                    && (value >> 48) == renderer::GifInterpreter::kMobyMarker) {
                    *moby_marker = direct_at;
                }
            }
            gif.insert(
                gif.end(),
                reinterpret_cast<const std::uint8_t*>(head),
                reinterpret_cast<const std::uint8_t*>(head) + sizeof(head)
            );
            gif.insert(gif.end(), vif.data() + at, vif.data() + at + size);
            if (strips != nullptr) {
                track_direct(vif.data() + at, size);
            }
            at += size;
        } else if (command >= 0x60) {
            const std::uint32_t vn = (command >> 2) & 3;
            const std::uint32_t vl = command & 3;
            const std::uint32_t bits = vl == 0 ? 32 : vl == 1 ? 16 : vl == 2 ? 8 : 16;
            const std::uint32_t count = num == 0 ? 256 : num;
            const std::uint32_t components = vl == 3 ? 1 : vn + 1;
            const std::size_t size = ((count * components * bits + 31) / 32) * 4;
            if (covering && at + size <= vif.size()) {
                draw_coverage::unpack(
                    imm & 0x3FF, vn, vl, count, origin(at), {vif.data() + at, size}
                );
            }
            if (strips != nullptr && at + size <= vif.size()) {
                const std::uint32_t address = imm & 0x3FF;
                const std::uint8_t* data = vif.data() + at;
                if (vn == 3 && vl == 0 && count == 1 && address == 0) {
                    // A strip's GIF tag: a triangle strip (PRIM 4) of NLOOP vertices.
                    std::uint64_t tag;
                    std::memcpy(&tag, data, 8);
                    strip_tag = ((tag >> 47) & 7) == 4 && (tag & 0x7FFF) >= 3;
                    strip_count = static_cast<std::uint32_t>(tag & 0x7FFF);
                    second_texture = ((tag >> 56) & 1) != 0;
                    // PRIM's FGE (bit 5): the tag's PRIM when PRE is set, else the register's.
                    const std::uint64_t prim = ((tag >> 46) & 1) != 0 ? tag >> 47 : reg_prim;
                    strip_fog = ((prim >> 5) & 1) != 0;
                    strip_context = ((prim >> 9) & 1) != 0;
                    strip_blend_disabled = ((tag >> 46) & 1) != 0 && ((prim >> 6) & 1) == 0;
                    strip_st = strip_rgba = strip_xyz = nullptr;
                } else if (strip_tag && address == 1 && vn == 1 && vl == 0
                           && count == strip_count) {
                    strip_st = data;
                } else if (strip_tag && address == 2 && vn == 3 && vl == 2
                           && count == strip_count) {
                    strip_rgba = data;
                } else if (strip_tag && address == 3 && vn == 2 && vl == 0
                           && count == strip_count) {
                    strip_xyz = data;
                }
                if (vn == 3 && vl == 0) {
                    for (std::uint32_t q = 0; q < count; ++q) {
                        const std::uint32_t at_vu = (address + q) & 0x3FF;
                        if (at_vu >= 0x3AC && at_vu <= 0x3AF) {
                            const std::uint32_t i = at_vu - 0x3AC;
                            std::memcpy(&vu_fog[i * 4], data + q * 16, 16);
                            vu_fog_set |= 1u << i;
                        }
                    }
                }
            }
            at += size;
        } else if ((command == 0x14 || command == 0x15 || command == 0x17)
                   && before_world != nullptr && *before_world == SIZE_MAX) {
            // The first VU1 program the frame starts: the 2D data so far comes before the world.
            *before_world = gif.size();
        }
        if ((command == 0x14 || command == 0x15 || command == 0x17) && strips != nullptr) {
            const std::size_t strips_before = strips->size();
            const Context& gs = contexts[strip_tag && strip_context ? 1 : 0];
            if (strip_tag && strip_st != nullptr && strip_rgba != nullptr && strip_xyz != nullptr) {
                renderer::EffectStrip strip;
                strip.first = static_cast<std::uint32_t>(strip_vertices->size());
                strip.count = strip_count;
                strip.tex0 = gs.tex0;
                strip.tex1 = gs.tex1;
                strip.clamp = gs.clamp;
                strip.alpha = gs.alpha;
                strip.test = gs.test;
                strip.z_masked = gs.zbuf_seen && ((gs.zbuf >> 32) & 1) != 0;
                strip.z_written = gs.zbuf_seen && ((gs.zbuf >> 32) & 1) == 0;
                strip.blend_disabled = strip_blend_disabled;
                strip.surface = surface;
                strip.uv_motion = uv_motion;
                static const bool& strip_fog_off = stats::probe(
                    "OPENRAC_STRIP_FOG_OFF", "reference: the effect strips drawn without the GS fog"
                );
                if (strip_fog && (vu_fog_set & 0xD) == 0xD && !strip_fog_off) {
                    // UpdateViewContext's terms (renderer/world/fog.h): the slope per integer unit
                    // is fog_mul over the near plane, the offset fog_add.
                    const float fog_mul = std::bit_cast<float>(vu_fog[3]);
                    strip.fogged = true;
                    strip.fog =
                        {(1.0f / renderer::world::kNear) * fog_mul,
                         std::bit_cast<float>(vu_fog[11]),
                         std::bit_cast<float>(vu_fog[12]),
                         std::bit_cast<float>(vu_fog[13])};
                    strip.fog_colour = static_cast<std::uint32_t>(reg_fogcol & 0xFFFFFF);
                }
                const auto uv_at = second_texture ? 2u : 0u;
                strip.uv_motion.offset = {uv_offsets[uv_at], uv_offsets[uv_at + 1]};
                if (second_texture) {
                    strip.uv_motion.amplitude = -strip.uv_motion.amplitude;
                }
                if (uploads != nullptr) {
                    const std::uint32_t tbp = static_cast<std::uint32_t>(gs.tex0 & 0x3FFF);
                    const std::uint32_t cbp = static_cast<std::uint32_t>((gs.tex0 >> 37) & 0x3FFF);
                    for (const std::uint32_t block : {cbp, tbp}) {
                        auto found = images.find(block);
                        if (found != images.end() && found->second.data.size() > 0) {
                            if (strip.uploads < 0) {
                                strip.uploads = static_cast<int>(uploads->size());
                            }
                            uploads->push_back(found->second);
                            strip.upload_count++;
                        }
                    }
                }
                for (std::uint32_t k = 0; k < strip_count; ++k) {
                    renderer::EffectVertex v;
                    std::memcpy(&v.x, strip_xyz + k * 12, 12);
                    std::memcpy(&v.s, strip_st + k * 8, 8);
                    std::memcpy(&v.rgba, strip_rgba + k * 4, 4);
                    strip_vertices->push_back(v);
                }
                strips->push_back(strip);
            }
            if (covering) {
                draw_coverage::start(
                    command, imm, strips->size() > strips_before, gs.alpha, gs.test, gs.tex0
                );
            }
            strip_tag = false;
        }
    }
    // A game that marks its world's mobys (rac2/host/render.c): the strips between the world's
    // marker and theirs were drawn before the mobys.
    if (strips != nullptr && strips_at_mobys) {
        const std::size_t from = strips_at_world.value_or(strips_at_start);
        for (std::size_t i = from; i < *strips_at_mobys && i < strips->size(); ++i) {
            (*strips)[i].before_mobys = true;
        }
    }
    return gif;
}

// The inverse of a view matrix (a rotation and a translation, column-major).
renderer::Mat4 rigid_inverse(const renderer::Mat4& m) {
    renderer::Mat4 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r[4 * j + i] = m[4 * i + j];
        }
    }
    for (int i = 0; i < 3; ++i) {
        r[12 + i] = -(r[i] * m[12] + r[4 + i] * m[13] + r[8 + i] * m[14]);
    }
    r[15] = 1.0f;
    return r;
}

// The game mode word, as the game's front end names it (rac1: port/game/rac1/host/front_end.c); 6
// in the boot program is the flight between planets. 0 for a game without one.
std::uint32_t game_mode_address() {
    return openrac_game_front_end != nullptr ? openrac_game_front_end->game_mode : 0;
}

}  // namespace

// The generation the first native-quad marker in a 2D list names (ui_quad): an A+D write of
// kUiQuadMarker to the NOP register. None when the list has no marker.
std::optional<std::uint32_t> marker_generation(std::span<const std::uint8_t> packets) {
    for (std::size_t at = 0; at + 9 <= packets.size(); at += 8) {
        std::uint64_t value = 0;
        std::memcpy(&value, packets.data() + at, 8);
        if ((value >> 48) == renderer::GifInterpreter::kUiQuadMarker && packets[at + 8] == 0x7F) {
            return static_cast<std::uint32_t>(value >> 24) & 0x3F;
        }
    }
    return std::nullopt;
}

namespace {

// Reads the window's events: the game's input from them, the window's own keys, the overlay's
// and the debug menu's, then the settings their menus changed. False once the window was closed.
bool read_window_events() {
    SDL_Event event;
    while (platform::poll_event(event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        const bool consumed = window_key(event) || debug_overlay::event(event)
                              || debug_menu::event(event, openrac_game_loaded_overlay());
        debug_overlay::camera_debug_event(event);
        if (consumed) {
            continue;  // the window's own keys; the debug overlay's; the port's debug menu (debug_menu.h)
        }
        g->input.handle_event(event);
    }
    apply_mouse_capture();
    // A controls file changed beside the game (the launcher) is applied within a second.
    g->controls.poll(static_cast<double>(SDL_GetTicks()) / 1000.0);
    g->input.update();
    // The controllers connected are remembered beside the controls, for the launcher.
    if (g->input.take_devices_changed()) {
        g->controls.remember(g->input.devices());
    }
    // The port's rows on the game's Video options page (options_menu.h), and the overlay's drawer
    // and menus: a changed one is applied. Before the debug menu, which adds its entry to the
    // Options list options_menu gives the game.
    const bool changed = options_menu::update(openrac_game_loaded_overlay());
    if (debug_overlay::take_settings_changed() || changed) {
        apply_settings();
    }
    debug_menu::update(openrac_game_loaded_overlay(), g->index);
    // A start or travel a mod asked for (interface 1.8), where the developer menu starts a level.
    mods::play_update();
    debug_overlay::meat_proxy_review_update();
    // The mods' entries go after the Planets entry debug_menu put in the same list (mod_menu.h).
    mod_menu::update(openrac_game_loaded_overlay());
    // The mods' items in the main menu and their pages of the game's widgets (mod_ui.h).
    mod_ui::update(openrac_game_loaded_overlay());
    return true;
}

// Uses the world the game draws now: the level loaded (`level`, -1 the boot program), or one of
// the boot program's worlds.
int world_to_draw(std::span<const std::uint8_t> ram, int level) {
    if (openrac_game_flight != nullptr) {
        if (const int flight = openrac_game_flight(); flight >= 0) {
            return -2 - flight;
        }
    }
    // The boot program's worlds: the flight between planets (game mode 6), else the title world.
    // A flight between planets also plays inside a planet's program. The game draws it by the ship's
    // state (dispatch_game_state_update): the planet in the take-off (0), the landing (8) and while
    // the ship leaves along its path (3, 7); space only in state 4 (DrawSpaceFlightFrame), whose
    // world is the flight world's. The mode there is the level's own copy of the word
    // (openrac_game_mode), not the boot program's.
    int world = openrac_game_has_level(level)            ? level
                : word_at(ram, game_mode_address()) == 6 ? -2
                                                         : -1;
    std::uint32_t level_mode = 0xFFFFFFFFu;
    if (world >= 0 && openrac_game_front_end != nullptr && openrac_game_front_end->ship_state != 0
        && openrac_game_mode != nullptr && openrac_game_mode(&level_mode) != 0 && level_mode == 6) {
        const std::uint32_t ship = word_at(ram, openrac_game_front_end->ship_state);
        if (ship == 4) {
            world = -2;
        }
    }
    return world;
}

// Uses the requested world after graphics ownership returns.
void use_world(std::span<const std::uint8_t> ram, int level) {
    const int world = world_to_draw(ram, level);
    use_level(world);
    // The flight between planets plays while the game reads the next level from the disc; its
    // number is already the level word's. Its files are read meanwhile, not after the flight.
    if (world == -2 && openrac_game_front_end != nullptr && openrac_game_front_end->level != 0) {
        const std::uint32_t next = word_at(ram, openrac_game_front_end->level);
        if (next < static_cast<std::uint32_t>(openrac_game_front_end->levels)) {
            preload_level(static_cast<int>(next));
        }
    }
}

// Puts the camera the game's world renderers drew with this tick into `state`, if they ran.
void copy_world_camera(host::GameState& state) {
    if (g->world_camera) {
        state.camera_position = g->world_camera->camera_position;
        state.forward = g->world_camera->forward;
        state.left = g->world_camera->left;
        state.up = g->world_camera->up;
        state.tan_half_fov_x = g->world_camera->tan_half_fov_x;
        state.tan_half_fov_y = g->world_camera->tan_half_fov_y;
        state.fog_colour = g->world_camera->fog_colour;
        state.fog_near = g->world_camera->fog_near;
        state.fog_far = g->world_camera->fog_far;
        state.fog_near_f = g->world_camera->fog_near_f;
        state.fog_far_f = g->world_camera->fog_far_f;
        state.background_colour = g->world_camera->background_colour;
        state.moby_view = g->world_camera->moby_view;
        // A level's world under a menu's moby context: its mobys, not the menu's in the globals.
        if (g->world_under_menu) {
            state.mobys = g->world_camera->mobys;
            state.nanotech_orbs = g->world_camera->nanotech_orbs;
        }
    }
}

// Consumes the camera only when committing the captured tick.
void take_world_camera(host::GameState& state) {
    copy_world_camera(state);
    g->world_camera.reset();
    g->world_under_menu = false;
}

// Logs the camera this tick is drawn from (OPENRAC_CAM_LOG). `world_ran`: the game's world
// renderers ran this tick.
void log_camera(const host::GameState& state, bool world_ran) {
    // OPENRAC_CAM_LOG: the camera each frame is drawn from (position, forward, up), in the log.
    static const bool& cam_log =
        stats::probe("OPENRAC_CAM_LOG", "the camera each tick is drawn from, in the log");
    if (cam_log) {
        log::info(
            "cam {} {:.3f} {:.3f} {:.3f} f {:.4f} {:.4f} {:.4f} u {:.4f} {:.4f} {:.4f} world {}",
            g->index,
            state.camera_position[0],
            state.camera_position[1],
            state.camera_position[2],
            state.forward[0],
            state.forward[1],
            state.forward[2],
            state.up[0],
            state.up[1],
            state.up[2],
            world_ran ? 1 : 0
        );
    }
}

// Reads where a scripted run is (g_route): the level, the game mode and when the player got
// control.
void follow_route(std::span<const std::uint8_t> ram, int level) {
    // The game mode (the loaded level's copy, or the boot program's word) and control (Route).
    std::uint32_t mode = 0;
    const bool in_level = openrac_game_has_level(level);
    if (!(in_level && openrac_game_mode != nullptr && openrac_game_mode(&mode) != 0)) {
        mode = game_mode_address() != 0 ? word_at(ram, game_mode_address()) : 0;
    }
    g_route.level = in_level ? level : -1;
    g_route.mode = static_cast<int>(mode);
    if (!skippable()) {
        g_route.skippable_from = -1;
    } else if (g_route.skippable_from < 0) {
        g_route.skippable_from = static_cast<std::int64_t>(g->index);
    }
    if (g_route.control < 0) {
        g_route.streak = in_level && mode == 0 ? g_route.streak + 1 : 0;
        if (g_route.streak >= kControlHold) {
            g_route.control = static_cast<std::int64_t>(g->index);
            log::info("control at frame {} (level {})", g->index, level);
        }
    }
}

// Writes this tick's line of OPENRAC_TRACE. `draws`: the layers the game drew (frame()).
void trace_tick(const host::GameState& state, std::uint32_t draws) {
    // OPENRAC_TRACE=FILE: one JSON line a tick, for port/tools/regress (docs/port/REGRESSION.md):
    // f the window frame, c frames since control (null before), lv the level (-1 the boot
    // program), m the game mode, cam/fw the camera's position and forward row, hp/hr/hs the hero's
    // position, rotation and state, mb the live mobys, dr the layers the game drew.
    static std::FILE* trace = [] {
        const std::optional<std::string> path = env_string(env::Var::Trace);
        std::FILE* f = path && !path->empty() ? std::fopen(path->c_str(), "w") : nullptr;
        if (path && f == nullptr) {
            log::warn("OPENRAC_TRACE: cannot write {}", *path);
        }
        return f;
    }();
    if (trace != nullptr) {
        const auto num = [](float v) {
            return std::isfinite(v) ? std::format("{:.4f}", v) : std::string("null");
        };
        const auto vec = [&](const float* v) {
            return std::format("[{},{},{}]", num(v[0]), num(v[1]), num(v[2]));
        };
        std::string line = std::format(
            "{{\"f\":{},\"c\":{},\"lv\":{},\"m\":{},\"cam\":{},\"fw\":{}",
            g->index,
            g_route.control < 0
                ? std::string("null")
                : std::to_string(static_cast<std::int64_t>(g->index) - g_route.control),
            g_route.level,
            g_route.mode,
            vec(state.camera_position.data()),
            vec(state.forward.data())
        );
        float hp[3]{}, hr[3]{};
        std::int32_t hs = 0;
        if (openrac_game_hero != nullptr && openrac_game_hero(hp, hr, &hs) != 0) {
            line += std::format(",\"hp\":{},\"hr\":{},\"hs\":{}", vec(hp), vec(hr), hs);
        }
        line += std::format(",\"mb\":{},\"dr\":{}}}\n", state.mobys.size(), draws);
        std::fwrite(line.data(), 1, line.size(), trace);
        std::fflush(trace);  // a crash keeps every tick before it
    }
}

// Whether a scripted run is over: OPENRAC_CONTROL_FRAMES frames after control.
bool run_over() {
    // OPENRAC_CONTROL_FRAMES=N: the run ends N frames after control (a route's length).
    static const std::int64_t control_frames = env::integer(env::Var::ControlFrames, -1);
    if (control_frames >= 0 && g_route.control >= 0
        && static_cast<std::int64_t>(g->index) >= g_route.control + control_frames) {
        log::info("{} frames after control: the run ends", control_frames);
        return true;
    }
    return false;
}

// Writes the game's main memory at the frame OPENRAC_DUMP_RAM names.
void dump_ram(std::span<const std::uint8_t> ram) {
    // OPENRAC_DUMP_RAM=FRAME: the game's active main memory at that frame, to ram_FRAME.bin.
    static const std::optional<std::uint64_t> dump_ram = env_frame(env::Var::DumpRam);
    if (dump_ram && g->index == *dump_ram) {
        const std::string path = std::format("ram_{}.bin", g->index);
        if (std::FILE* f = std::fopen(path.c_str(), "wb")) {
            std::fwrite(ram.data(), 1, ram.size(), f);
            std::fclose(f);
            log::info("main memory written to {}", path);
        }
    }
}

// Logs the game's state for bring-up (OPENRAC_DEBUG, each 100 ticks): the level, the camera, the
// front end's words and the pad, the fog, the effects, the sky and the particles drawn.
void log_tick_state(
    std::span<const std::uint8_t> ram, const host::GameState& state, int level, std::uint32_t draws
) {
    log::info(
        "frame {}: level {}, camera {:.1f} {:.1f} {:.1f}, forward {:.2f} {:.2f} {:.2f}, "
        "fov {:.3f}, {} mobys, layers drawn {:#x}",
        g->index,
        level,
        state.camera_position[0],
        state.camera_position[1],
        state.camera_position[2],
        state.forward[0],
        state.forward[1],
        state.forward[2],
        state.tan_half_fov_y,
        state.mobys.size(),
        draws
    );
    if (const openrac_front_end* fe = openrac_game_front_end) {
        // The first pad's block: +0x100 the sticks, +0x194 its socket, +0x198 its state,
        // +0x1A0 the buttons held, +0x1A4 the raw word.
        // The level's own game mode (openrac_game_mode: rac1's level copy of its mode word, 6 in
        // the ship's landing, 2 in a cutscene, 0 in play), or -1 outside a level.
        std::uint32_t level_mode = 0xFFFFFFFFu;
        if (openrac_game_mode == nullptr || openrac_game_mode(&level_mode) == 0) {
            level_mode = 0xFFFFFFFFu;
        }
        log::info(
            "  mode {} level mode {} dialog kind {} step {} level word {} title exit {} pad {:#x}",
            static_cast<int>(word_at(ram, fe->game_mode)),
            static_cast<int>(level_mode),
            static_cast<int>(word_at(ram, fe->dialog)),
            static_cast<int>(word_at(ram, fe->dialog + 0x1C)),
            static_cast<int>(word_at(ram, fe->level)),
            static_cast<int>(word_at(ram, fe->title_exit)),
            word_at(ram, fe->pad + 0x1A4)
        );
        float axes[4]{};
        std::memcpy(axes, ram.data() + fe->pad + 0x100, sizeof axes);
        log::info(
            "  pad socket {} state {} held {:#x}, right {:.3f} {:.3f}, left {:.3f} {:.3f}",
            word_at(ram, fe->pad + 0x194),
            word_at(ram, fe->pad + 0x198),
            word_at(ram, fe->pad + 0x1A0),
            axes[0],
            axes[1],
            axes[2],
            axes[3]
        );
    }
    log::info(
        "  fog colour {:.0f} {:.0f} {:.0f}, depth {:.0f} to {:.0f}, F {:.0f} to {:.0f}",
        state.fog_colour[0] * 255,
        state.fog_colour[1] * 255,
        state.fog_colour[2] * 255,
        state.fog_near,
        state.fog_far,
        state.fog_near_f,
        state.fog_far_f
    );
    if (state.moby_view) {
        // How many of the live mobys MobyProc's distance test leaves undrawn, and fades.
        std::size_t culled = 0, fading = 0;
        for (const host::LiveMoby& m : state.mobys) {
            if (m.address + 0x100 > ram.size()) {
                continue;
            }
            const auto fade = g->profile->moby_distance_fade(ram.data() + m.address, state);
            culled += fade ? 0 : 1;
            fading += fade && *fade < 0x80 ? 1 : 0;
        }
        log::info(
            "  moby draw cap {} units, near plane {:.0f}: {} of {} mobys beyond their draw "
            "distance, {} fading",
            state.moby_view->draw_cap,
            state.moby_view->near_plane,
            culled,
            state.mobys.size(),
            fading
        );
    }
    static const std::optional<std::uint64_t> fx_dump = env_frame(env::Var::FxDump);
    if (fx_dump && g->index >= *fx_dump && g->index < *fx_dump + 100) {
        for (const renderer::EffectQuad& e : g->effects) {
            log::info(
                "    quad {:.1f} {:.1f} {:.1f} {:.2f} | {:.1f} {:.1f} {:.1f} | "
                "{:.1f} {:.1f} {:.1f} | {:.1f} {:.1f} {:.1f} rgba {:#x} "
                "st {:.2f},{:.2f} {:.2f},{:.2f} tex0 {:#x} alpha {:#x} clamp {:#x}",
                e.corner[0][0],
                e.corner[0][1],
                e.corner[0][2],
                e.corner[0][3],
                e.corner[1][0],
                e.corner[1][1],
                e.corner[1][2],
                e.corner[2][0],
                e.corner[2][1],
                e.corner[2][2],
                e.corner[3][0],
                e.corner[3][1],
                e.corner[3][2],
                e.rgba[0],
                e.st[0][0],
                e.st[0][1],
                e.st[3][0],
                e.st[3][1],
                e.tex0,
                e.alpha,
                e.clamp
            );
        }
    }
    if (!g->sky_sprites.empty()) {
        const auto& [r, tex0] = g->sky_sprites.front();
        float size;
        std::memcpy(&size, r.data() + 0x1C, 4);
        log::info(
            "  {} sky sprites; the first RGBA {:#x} size {:.3f} TEX0 {:#x} ALPHA {:#x}",
            g->sky_sprites.size(),
            r[4] | r[5] << 8 | r[6] << 16 | static_cast<std::uint32_t>(r[7]) << 24,
            size,
            tex0,
            r[3]
        );
    }
    if (!g->particles.empty()) {
        int kinds[4] = {0, 0, 0, 0};
        for (const State::Particle& p : g->particles) {
            kinds[p.record[1] & 3]++;
        }
        log::info(
            "  {} particles (sprites {}, flat {}, lines {}, ribbons {})",
            g->particles.size(),
            kinds[0],
            kinds[1],
            kinds[2],
            kinds[3]
        );
    }
    if (!g->effects.empty()) {
        const renderer::EffectQuad& e = g->effects.front();
        log::info(
            "  {} effect quads; the first at {:.1f} {:.1f} {:.1f}, "
            "TEX0 {:#x}, ALPHA {:#x}, RGBA {:#x}",
            g->effects.size(),
            e.corner[0][0],
            e.corner[0][1],
            e.corner[0][2],
            e.tex0,
            e.alpha,
            e.rgba[0]
        );
    }
    log::info(
        "  load stage {:#x} retries {} snd state {:#x}",
        word_at(ram, 0x0015EF48),
        word_at(ram, 0x0015EFBC),
        word_at(ram, word_at(ram, 0x001517D0) + 8) & 0xFFFF
    );
}

// Logs the mobys near the hero (OPENRAC_MOBY_LOG).
void log_mobys_near_hero(std::span<const std::uint8_t> ram, const host::GameState& state) {
    // OPENRAC_MOBY_LOG=N[:RADIUS]: every N ticks, the mobys within RADIUS game units of the hero
    // (default 20) in the log: class, address, state, mode bits, animation, position, the distance,
    // the collision radius and the update function, for seeing what an object is doing.
    static const std::optional<std::string_view> moby_log = env::value(env::Var::MobyLog);
    // N read as std::atoi read it, held to int's range, where atoi's answer was undefined.
    static const std::int64_t moby_every =
        std::min<std::int64_t>(env::read_integer(moby_log, 10, 0), std::numeric_limits<int>::max());
    if (moby_every > 0 && g->index % static_cast<std::uint64_t>(moby_every) == 0) {
        const std::size_t colon = moby_log->find(':');
        const float radius =
            colon != std::string_view::npos
                ? static_cast<float>(env::read_number(moby_log->substr(colon + 1), 0.0))
                : 20.0f;
        const host::LiveMoby* hero = nullptr;
        for (const host::LiveMoby& m : state.mobys) {
            if (m.class_id == 0) {
                hero = &m;
                break;
            }
        }
        for (const host::LiveMoby& m : state.mobys) {
            if (hero == nullptr || m.address + 0x100 > ram.size()) {
                continue;
            }
            const float dx = m.matrix[12] - hero->matrix[12], dy = m.matrix[13] - hero->matrix[13],
                        dz = m.matrix[14] - hero->matrix[14];
            const float d = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (d > radius) {
                continue;
            }
            const std::uint8_t* b = ram.data() + m.address;
            float sphere[4];
            float t = 0.0f;
            std::memcpy(sphere, b, sizeof sphere);
            std::memcpy(&t, b + 0x54, 4);
            log::info(
                "moby {} class {} at {:#x}: state {} mode {:#06x} seq {} frame {} t {:.2f} "
                "pos {:.2f} {:.2f} {:.2f} dist {:.2f} radius {:.2f} "
                "update {:#x} pvars {:#x} draw {} {} {:#x}",
                g->index,
                m.class_id,
                m.address,
                b[0x20],
                static_cast<unsigned>(b[0x34] | b[0x35] << 8),
                b[0x52],
                b[0x50],
                t,
                m.matrix[12],
                m.matrix[13],
                m.matrix[14],
                d,
                sphere[3],
                word_at(ram, m.address + 0x74),
                word_at(ram, m.address + 0x78),
                b[0x30],
                b[0x31],
                static_cast<unsigned>(b[0x32] | b[0x33] << 8)
            );
        }
    }
}

// Returns the third light the moby program gives a moby from the point lights (Going Commando's
// MobyProc): the eight after the light bank's 16 sets, {colour, falloff, position, reach}, a light
// whose x and y are both 0 left out (0x2A51C8). Each light within reach of the moby's sphere
// centre (+0x00, units of 1/1024) gives its colour times 1 - distance / reach, from it towards the
// moby (0x2A5810). One is the light as it is, its colour at least 0; several are summed, the
// directions weighted by each colour's (r + g + b) x 0.332 and normalised (0x2A5960). Returns the
// colour (w 0) and world direction (w 0), or nothing when no light reaches the moby.
std::optional<std::array<float, 8>> moby_point_light(
    std::span<const std::uint8_t> ram, gaddr bank, std::uint32_t moby
) {
    constexpr std::uint32_t kPoints = 16 * 0x40;
    if (bank == 0 || bank + kPoints + 8 * 0x20 > ram.size() || moby + 0x10 > ram.size()) {
        return std::nullopt;
    }
    std::array<float, 4> centre;
    std::memcpy(centre.data(), ram.data() + moby, sizeof(centre));
    std::array<float, 3> colour{}, direction{}, single{};
    int count = 0;
    for (std::uint32_t i = 0; i < 8; ++i) {
        std::array<float, 8> light;
        std::memcpy(light.data(), ram.data() + bank + kPoints + i * 0x20, sizeof(light));
        std::uint64_t xy;
        std::memcpy(&xy, ram.data() + bank + kPoints + i * 0x20 + 0x10, sizeof(xy));
        if (xy == 0) {
            continue;
        }
        const float reach = light[7] * 1024.0f;
        std::array<float, 3> away{};
        float squared = 0.0f;
        for (int c = 0; c < 3; ++c) {
            away[c] = centre[c] - light[4 + c] * 1024.0f;
            squared += away[c] * away[c];
        }
        if (!(squared - reach * reach <= 0.0f) || squared <= 0.0f) {
            continue;
        }
        const float distance = std::sqrt(squared);
        const float weight = 1.0f - distance * (1.0f / 1024.0f / light[7]);
        const float luminance =
            (light[0] * weight + light[1] * weight + light[2] * weight) * 0.33203125f;
        for (int c = 0; c < 3; ++c) {
            colour[c] += light[c] * weight;
            single[c] = away[c] / distance;
            direction[c] += single[c] * luminance;
        }
        ++count;
    }
    if (count == 0) {
        return std::nullopt;
    }
    if (count == 1) {
        direction = single;
    }
    std::array<float, 8> out{};
    for (int c = 0; c < 3; ++c) {
        out[c] = std::max(colour[c], 0.0f);
    }
    const float length = std::sqrt(
        direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]
    );
    if (!(length > 0.0f) || !std::isfinite(length)) {
        return std::nullopt;
    }
    for (int c = 0; c < 3; ++c) {
        out[4 + c] = direction[c] / length;
    }
    return out;
}

// The live mobys of one tick, as pose_mobys reads them for keep_tick: each one drawn and its
// address and joints, the joint palette they index, the hero's shadow, and the mobys the game
// drew into a texture.

// "Scenery detail: Original": ShrubProc's choice for the tick, from the camera the game's world
// renderers left in memory (`world_ran`, not under a menu); none to keep the last tick's, empty
// with the setting off.
std::optional<std::vector<host::ShrubDraw>> shrub_draws(
    std::span<const std::uint8_t> ram, bool world_ran
) {
    if (!settings::get().original_lod) {
        return std::vector<host::ShrubDraw>{};
    }
    if (!world_ran) {
        return std::nullopt;
    }
    const profiling::Zone trace("Shrub draws");
    return g->profile->shrub_draws(ram, openrac_game_loaded_overlay());
}

// Reads the live mobys this tick: which are drawn, how (blend, light, colour, the camera they
// were drawn with) and in what pose, and the hero's shadow. Mobys the game drew but did not list
// are added to `state`. `world_ran`, `only_own_camera`: the world renderers ran this tick, and
// only mobys with a camera of their own were drawn without them; `under_menu`: that world is a
// level's under a menu (world_under_menu); `debug`: OPENRAC_DEBUG.
// The ties and shrubs as the game holds them, one per instance the level loaded
// (GameProfile::scenery_matrices), when the world ran this tick; none for a game that never moves
// them, or when the words do not point at as many records in memory.
std::optional<scene::GameScenery> scenery_matrices(
    std::span<const std::uint8_t> ram, bool world_ran, const host::GameState& state
) {
    const auto words = g->profile->scenery_matrices();
    if (!world_ran || (words[0] == 0 && words[1] == 0)) {
        return std::nullopt;
    }
    // Where the word at an executable's address points, when `count` records of `size` fit there.
    auto table = [&](std::uint32_t address, std::size_t count, std::size_t size) -> gaddr {
        if (address == 0 || count == 0) {
            return 0;
        }
        const gaddr word = openrac_relocate_data(address);
        if (word + 4 > ram.size()) {
            return 0;
        }
        gaddr at;
        std::memcpy(&at, ram.data() + word, 4);
        return at != 0 && at % 16 == 0 && at + count * size <= ram.size() ? at : 0;
    };
    scene::GameScenery result;
    constexpr std::array<scene::Layer, 2> layers{scene::Layer::Ties, scene::Layer::Shrubs};
    for (std::size_t k = 0; k < 2; ++k) {
        const std::size_t count = g->level.instances[static_cast<std::size_t>(layers[k])].size();
        const gaddr at = table(words[k], count, 0x40);
        if (at == 0) {
            continue;
        }
        auto& out = result.matrices[k];
        out.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            const std::uint8_t* m = ram.data() + at + i * 0x40;
            for (std::size_t c = 0; c < 4; ++c) {
                std::memcpy(&out[i][c * 3], m + c * 0x10, 12);
            }
        }
    }
    // What the game's culls of the ties and shrubs test besides their hidden bits (Going Commando's
    // CullAndBinTieInstances 0x2F1010, CullAndEmitShrubs 0x2E1228): the frame's occlusion mask, the
    // camera's depth axis and near plane (GameState::moby_view, the view MobyProc reads too) and
    // the caps on their draw distances, words the level loader sets beside the records' words
    // (the ties' gp + 0x320C, 0x48 past theirs; the shrubs' gp + 0x30B8, 0x10 past theirs).
    const auto word_at_address = [&](std::uint32_t address) -> std::optional<std::uint32_t> {
        const gaddr at = openrac_relocate_data(address);
        if (address == 0 || at + 4 > ram.size()) {
            return std::nullopt;
        }
        std::uint32_t v;
        std::memcpy(&v, ram.data() + at, 4);
        return v;
    };
    std::optional<scene::Pvs::Mask> mask;
    if (occlusion_address() != 0) {
        if (const gaddr m = openrac_relocate_data(occlusion_address());
            m + sizeof(scene::Pvs::Mask) <= ram.size()) {
            mask.emplace();
            std::memcpy(mask->data(), ram.data() + m, mask->size());
        }
    }
    // Draw distance (settings.h): the game's reach times this; 0 none (Unlimited).
    const float scale = settings::get().draw_distance_scale();
    std::uint32_t tie_cap = 0;  // for the log below
    float shrub_cap = 0.0f;
    // The depth of a sphere's centre along the view, as the culls take it (the third column of
    // the view matrix), in game units; none without the view.
    const auto depth = [&](const float* centre) -> std::optional<float> {
        if (!state.moby_view) {
            return std::nullopt;
        }
        const auto& a = state.moby_view->depth_axis;
        const auto& eye = state.camera_position;
        return a[0] * (centre[0] - eye[0]) + a[1] * (centre[1] - eye[1])
               + a[2] * (centre[2] - eye[2]);
    };
    const float near_plane = state.moby_view ? state.moby_view->near_plane / 1024.0f : 0.0f;
    // Past `reach` along the view (game units; scaled by Draw distance, none when Unlimited), or
    // wholly nearer than the near plane: the culls' far and near tests on the sphere (x y z r).
    const auto beyond = [&](const float* sphere, float reach) {
        const auto z = depth(sphere);
        return z && ((scale > 0.0f && *z > reach * scale) || *z + sphere[3] <= near_plane);
    };
    const std::size_t ties =
        g->level.instances[static_cast<std::size_t>(scene::Layer::Ties)].size();
    if (const gaddr at = table(words[2], ties, 0x20); at != 0) {
        result.tie_spheres.resize(ties);
        result.tie_lights.resize(ties);
        result.tie_hidden.resize(ties);
        // The cap, in 16-unit steps (lw s1; srl 4): a tie's own distance is its record's byte at
        // +0x1C in the same steps, and the cull takes the smaller, shifted back (pminw; dsll32 4).
        const auto cap = words[2] != 0 ? word_at_address(words[2] + 0x48) : std::nullopt;
        const std::uint32_t cap_steps = cap ? *cap >> 4 : 0;
        tie_cap = cap_steps << 4;
        for (std::size_t i = 0; i < ties; ++i) {
            const std::uint8_t* r = ram.data() + at + i * 0x20;
            std::memcpy(result.tie_spheres[i].data(), r, 16);
            std::memcpy(&result.tie_lights[i], r + 0x1A, 2);
            // The occlusion bit at +0x10 of the mask's byte at +0x11 (lbu; and with the mask's).
            const bool occluded =
                mask && r[0x11] < mask->size() && ((*mask)[r[0x11]] & r[0x10]) == 0;
            const std::uint32_t steps = std::min<std::uint32_t>(r[0x1C], cap_steps);
            const bool far =
                cap_steps != 0 && beyond(result.tie_spheres[i].data(), float(steps << 4));
            result.tie_hidden[i] = (r[0x1E] & 4) || occluded || far;
        }
    }
    // The terrain's occlusion words: CullAndEmitTfrags (0x2EF480) walks the 0x40-byte tfrag records
    // from the word at gp + 0x3180 (0x44 before the ties' records' word), as many as gp + 0x3184
    // says, and leaves out one whose bit at +0x3A is clear in the mask's byte at +0x3B.
    if (words[2] != 0) {
        const auto count = word_at_address(words[2] - 0x40);
        if (count && *count > 0 && *count < 0x10000) {
            if (const gaddr at = table(words[2] - 0x44, *count, 0x40); at != 0) {
                // Its far cull (gp + 0x31B0, 0x14 before the ties' records' word), in 1/1024 units
                // as the records' spheres: past it when the sphere's near side is farther.
                float far_cull = 0.0f;
                if (const auto bits = word_at_address(words[2] - 0x14)) {
                    std::memcpy(&far_cull, &*bits, 4);
                }
                result.terrain_occlusion.resize(*count);
                result.terrain_far.resize(*count);
                for (std::size_t i = 0; i < *count; ++i) {
                    const std::uint8_t* r = ram.data() + at + i * 0x40;
                    result.terrain_occlusion[i] =
                        static_cast<std::uint16_t>(r[0x3B] << 8 | r[0x3A]);
                    float s[4];
                    std::memcpy(s, r, sizeof(s));
                    const float sphere[4] =
                        {s[0] / 1024.0f, s[1] / 1024.0f, s[2] / 1024.0f, s[3] / 1024.0f};
                    const auto z = depth(sphere);
                    result.terrain_far[i] = far_cull > 0.0f && scale > 0.0f && z
                                            && *z - sphere[3] > far_cull / 1024.0f * scale;
                }
            }
        }
    }
    const std::size_t shrubs =
        g->level.instances[static_cast<std::size_t>(scene::Layer::Shrubs)].size();
    if (const gaddr at = table(words[3], shrubs, 0x20); at != 0) {
        result.shrub_spheres.resize(shrubs);
        result.shrub_lights.resize(shrubs);
        result.shrub_hidden.resize(shrubs);
        // The cap, a float; a shrub's own distance is its record's float at +0x10 as the loader
        // left it, and the cull takes the smaller (pminw on their bits).
        float cap = 0.0f;
        if (const auto bits = words[3] != 0 ? word_at_address(words[3] + 0x10) : std::nullopt) {
            std::memcpy(&cap, &*bits, 4);
        }
        shrub_cap = cap;
        for (std::size_t i = 0; i < shrubs; ++i) {
            const std::uint8_t* r = ram.data() + at + i * 0x20;
            std::memcpy(result.shrub_spheres[i].data(), r, 16);
            std::memcpy(&result.shrub_lights[i], r + 0x1E, 2);
            float own;
            std::memcpy(&own, r + 0x10, 4);
            const float reach = std::min(own, cap);
            const bool far = cap > 0.0f && beyond(result.shrub_spheres[i].data(), reach);
            result.shrub_hidden[i] = (r[0x16] & 1) || far;
        }
    }
    // Once a level: which visibility the culls use and the caps they read, for a player's log.
    static int logged_level = -100;
    if (logged_level != g->loaded) {
        logged_level = g->loaded;
        log::info(
            "level {}: scenery culls from the game's records: occlusion mask {}, {} ties, {} "
            "shrubs, {} terrain fragments; caps {} (ties) and {:.0f} (shrubs); draw distance {}",
            g->loaded,
            mask ? "read" : "unavailable",
            result.tie_hidden.size(),
            result.shrub_hidden.size(),
            result.terrain_occlusion.size(),
            tie_cap,
            shrub_cap,
            settings::get().draw_distance == 0 ? std::string("Unlimited")
                                               : std::format("{}%", settings::get().draw_distance)
        );
    }
    // OPENRAC_SCENERY_LOG: what the culls above leave out, every 100 ticks.
    static const bool& scenery_log = stats::probe(
        "OPENRAC_SCENERY_LOG", "the ties, shrubs and terrain the game's culls leave out"
    );
    if (scenery_log && g->index % 100 == 0) {
        const auto out = [](const std::vector<std::uint8_t>& v) {
            return std::count_if(v.begin(), v.end(), [](std::uint8_t b) { return b != 0; });
        };
        std::size_t seen_bits = 0, terrain_seen = 0;
        if (mask) {
            for (const std::uint8_t b : *mask) {
                seen_bits += static_cast<std::size_t>(std::popcount(b));
            }
        }
        for (std::size_t i = 0; i < result.terrain_occlusion.size(); ++i) {
            terrain_seen += (!mask || scene::Pvs::visible(*mask, result.terrain_occlusion[i]))
                                    && !(i < result.terrain_far.size() && result.terrain_far[i])
                                ? 1
                                : 0;
        }
        log::info(
            "scenery {}: mask {} of 1024 bits; ties {} of {} left out; shrubs {} of {} left out; "
            "terrain {} of {} fragments drawn; caps {} and {:.0f}; draw distance {}%",
            g->index,
            seen_bits,
            out(result.tie_hidden),
            result.tie_hidden.size(),
            out(result.shrub_hidden),
            result.shrub_hidden.size(),
            terrain_seen,
            result.terrain_occlusion.size(),
            tie_cap,
            shrub_cap,
            settings::get().draw_distance
        );
    }
    return result;
}

TickMobys pose_mobys(
    std::span<const std::uint8_t> ram,
    host::GameState& state,
    bool world_ran,
    bool only_own_camera,
    bool under_menu,
    bool debug
) {
    const profiling::Zone trace("Pose mobys");
    // Draw distance (settings.h) for MobyProc's distance test (host::moby_proc_distance_fade).
    if (state.moby_view) {
        state.moby_view->scale = settings::get().draw_distance_scale();
    }

    // The live mobys, by class, each in the pose its animation fields give (scene/moby_pose.h):
    // its joint palette goes to the scene, which skins the class's mesh with it. OPENRAC_ANIM=0
    // draws every moby in its bind pose.
    static const bool animate = env::flag(env::Var::Anim);
    std::vector<scene::Instance> live;
    std::vector<std::uint32_t> live_addresses;
    std::vector<int> live_joints;
    std::vector<scene::LevelScene::JointColumns> palette;
    std::vector<renderer::ShadowCapsule> shadows;
    std::uint32_t shadow_moby = 0;
    scene::MobyShadowState shadow_state;
    std::vector<renderer::ShadowSet> shadow_sets;
    const bool every_shadow = g->profile->every_moby_casts_shadow();
    std::vector<Kept::TargetMoby> target_mobys;
    // The point lights' third light of each moby they reach, where the game lights mobys by them;
    // as many as the shader's moby_point_lights holds (mesh.vert).
    std::vector<std::array<float, 8>> point_lit;
    constexpr std::size_t kPointLitMobys = 32;
    const gaddr point_bank = g->profile->mobys_take_point_lights() && light_bank_address() != 0
                                 ? openrac_relocate_data(light_bank_address())
                                 : 0;

    // A moby to pose: its address and joints, and its instance (in target_mobys or in live).
    struct PoseJob {
        std::uint32_t address = 0;
        int joints = 0;
        bool target = false;
        std::size_t index = 0;
    };

    std::vector<PoseJob> pose_jobs;
    std::array<std::optional<renderer::Vec3>, 2> water_hands{};
    // Mobys the game drew into a texture, and the ones it drew that are not in its list. The game
    // renders some mobys into a target the 2D layer then copies onto the screen: the vendor's item
    // and salesman screens (ReRAC interaction.md 9.3), the page menus' widgets (the 3D Ratchet and
    // the item previews, ReRAC menu_models.rs). It sets a zoom of 1.0 for them instead of the
    // frame's 0.63: a moby drawn with a wider tangent than the frame's goes into the texture at its
    // marker. The zoom does not reach the moby program's scales: drawn with it, the models sat at
    // the targets' edges at a third of their size, as ReRAC found too; they are drawn with the
    // frame's focal lengths (the frame camera's GameState::focal_x). A moby not in the list (the
    // vendor's own) is read from its address; with the frame's camera (the item's hologram over the
    // vendor) it is drawn in the world.
    const std::size_t listed = state.mobys.size();
    std::unordered_map<std::uint32_t, const host::GameState*> into_texture;
    // Over a world drawn under a menu (`under_menu`): the menu's mobys, drawn on the screen at their
    // markers in the 2D path with their own cameras, after the world.
    std::unordered_map<std::uint32_t, const host::GameState*> on_screen;
    std::array<float, 2> frame_focal{};
    {
        const auto drawn = [&](std::uint32_t address) {
            return std::any_of(
                state.mobys.begin(), state.mobys.end(), [&](const host::LiveMoby& m) {
                    return m.address == address;
                }
            );
        };
        // A level's world under a menu is not the frame's camera: the menu's own cameras are
        // (frontend/menu_layers.h).
        std::vector<float> own_tans;
        own_tans.reserve(g->moby_cameras.size());
        for (const auto& [address, camera] : g->moby_cameras) {
            own_tans.push_back(camera.tan_half_fov_x);
        }
        const FrameZoom zoom = frame_zoom(world_ran && !under_menu, state.tan_half_fov_x, own_tans);
        if (zoom.camera == FrameZoom::kWorld) {
            frame_focal = {state.focal_x, state.focal_y};
        } else if (zoom.camera >= 0) {
            const host::GameState& narrowest =
                g->moby_cameras[static_cast<std::size_t>(zoom.camera)].second;
            frame_focal = {narrowest.focal_x, narrowest.focal_y};
        }
        // Mobys the 2D path drew with lists of their own (the last list read: Going Commando's is
        // a tick behind its mobys), into a narrow buffer (`narrow`) or anywhere.
        const auto marked = [&](std::uint32_t address, bool narrow) {
            return std::any_of(
                g->texture_mobys.begin(), g->texture_mobys.end(), [&](const State::MarkedMobys& m) {
                    return (m.narrow || !narrow) && address >= m.first
                           && address < m.first + m.count * 0x100u;
                }
            );
        };
        const bool narrow_buffers = g->profile->own_mobys_into_narrow_buffers();
        for (const auto& [address, camera] : g->moby_cameras) {
            OwnCameraLayer layer =
                narrow_buffers && marked(address, true) && frame_focal[0] > 0.0f
                    ? OwnCameraLayer::Texture
                    : own_camera_layer(
                          camera.tan_half_fov_x, zoom, frame_focal[0] > 0.0f, under_menu
                      );
            // A moby first drawn this tick, which the list (a tick behind) does not show yet: not
            // in the world this frame. Its layer is known from the next list. Placed in the world,
            // Fizzwidget showed for a frame as a head over the ship's canopy where level 0's closing
            // scene cuts to the cockpit; retail draws him only into the screen's buffer. Into the
            // texture it is drawn only at a marker, so it waits a frame.
            if (narrow_buffers && layer == OwnCameraLayer::World && !marked(address, false)) {
                layer = OwnCameraLayer::Texture;
            }
            switch (layer) {
                case OwnCameraLayer::Texture:
                    into_texture.emplace(address, &camera);
                    break;
                case OwnCameraLayer::Screen:
                    on_screen.emplace(address, &camera);
                    break;
                case OwnCameraLayer::World:
                    break;
            }
            if (!drawn(address)) {
                if (const auto extra = g->profile->read_moby(ram, address)) {
                    state.mobys.push_back(*extra);
                }
            }
        }
    }
    const auto hero_rig = read_hero_shadow_rig(
        openrac_game_front_end,
        g->loaded,
        g->scene.hero_shadow_enabled(),
        g->profile->shadow_directions() != 0
    );

    const bool inspect_bones = debug_overlay::picker_snapshot_needed();
#ifdef OPENRAC_LIGHT_AUTHORING
    auto attached_catalog =
        enhanced_graphics_available() ? enhanced::current_object_lights(g->level.game) : nullptr;
    if (attached_catalog && attached_catalog->objects.empty()) {
        attached_catalog.reset();
    }
#endif
    for (std::size_t index = 0; index < state.mobys.size(); ++index) {
        bool light_only = false;
        const host::LiveMoby& m = state.mobys[index];
        const bool extra = index >= listed;
        // Hidden (mode bits 0x81): MobyProc does not draw it (the parked ship while the landing one
        // comes down, Clank's spare parts, Umbris's water plates, whose wave grid is drawn instead).
        if (m.address + 0x36 <= ram.size() && (ram[m.address + 0x34] & 0x81) != 0) {
            continue;
        }
        // A draw distance (+0x32) of 0: MobyProc's distance test never passes, so the game never draws
        // it. Volumes and blockers the levels place as mobys (Veldin's grey box behind the start, the
        // walls of Kalebo III's landing) are such: untextured, they showed as solid grey walls.
        if (!extra && m.address + 0x34 <= ram.size()
            && (ram[m.address + 0x32] | ram[m.address + 0x33]) == 0) {
            continue;
        }
        // Beyond its draw distance, or wholly behind the near plane: MobyProc does not draw it
        // (GameProfile::moby_distance_fade, which gives the fade of the ones it draws). Pokitaru
        // places two mobys of class 1161 with a turn of -5 pi, past the reach of the game's sine;
        // their rows come out thousands long and their spheres thousands of units away, so the
        // game never draws them, while drawn here they covered the sky in cyan. A moby the game
        // draws with a camera of its own (the menus', the vendor's) is not the world camera's to
        // cull or fade.
        const bool own_camera =
            std::any_of(g->moby_cameras.begin(), g->moby_cameras.end(), [&](const auto& c) {
                return c.first == m.address;
            });
        std::optional<std::uint8_t> distance_fade = std::uint8_t{0x80};
        if (!extra && world_ran && m.address + 0x34 <= ram.size() && !own_camera) {
            if (state.moby_view && g->profile->keeps_game_draw_distance(m.class_id)) {
                host::GameState retail = state;
                retail.moby_view->scale = 1.0f;
                distance_fade = g->profile->moby_distance_fade(ram.data() + m.address, retail);
            } else {
                distance_fade = g->profile->moby_distance_fade(ram.data() + m.address, state);
            }
        }
        if (!distance_fade) {
#ifdef OPENRAC_LIGHT_AUTHORING
            // An unseen mesh may still carry a light whose sphere reaches visible receivers.
            const auto model = g->level.moby_classes.find(m.class_id);
            if (!attached_catalog || model == g->level.moby_classes.end()) {
                continue;
            }
            const auto key =
                enhanced::object_light_key(g->level, scene::Layer::Mobys, model->second.first);
            const auto lights = attached_catalog->objects.find(key);
            if (lights == attached_catalog->objects.end()
                || std::none_of(
                    lights->second.begin(), lights->second.end(), [](const auto& light) {
                        return light.enabled;
                    }
                )) {
                continue;
            }
            light_only = true;
#else
            continue;
#endif
        }
        if (only_own_camera
            && std::none_of(g->moby_cameras.begin(), g->moby_cameras.end(), [&](const auto& c) {
                   return c.first == m.address;
               })) {
            continue;
        }
        auto cls = g->level.moby_classes.find(m.model_class);
        if (cls == g->level.moby_classes.end() && !m.model_only) {
            cls = g->level.moby_classes.find(m.class_id);
        }
        if (cls == g->level.moby_classes.end()) {
            continue;
        }
        scene::Instance instance{cls->second.first, m.matrix, {1, 1, 1, 1}};
        instance.lighting_only = light_only;
        instance.gameplay_essential = g->profile->gameplay_essential(m.class_id);
        instance.occlusion = m.occlusion;
        if (extra || into_texture.contains(m.address)) {
            instance.occlusion = scene::Pvs::always;
        }
        // Authored bounds do not cover debug stand-ins or replacement meshes.
        if (!cls->second.second && g->level.overridden_models == 0) {
            instance.world_sphere = m.world_sphere;
        }
        // How MobyProc blends it (ReRAC's MobyBlend, from the game's moby draw). Its vertex alpha is
        // its alpha byte (+0x23) times its distance fade (GameProfile::moby_distance_fade: the last 16
        // units of its draw distance) >> 7. Mode bit 0x200 (+0x34) adds it; the fade below 0x80 or
        // mode bit 8 blends it with the fading alpha test (blend 3: Z written from As 0x08, so a
        // far moby comes in gradually instead of at once); else a vertex alpha below 0x80 blends it
        // at that alpha (the explosions' flashes start at 0x20..0x40 and fade out; a fireball can
        // be spawned below 0x80).
        if (m.address + 0x36 <= ram.size()) {
            const std::uint8_t* b = ram.data() + m.address;
            const unsigned mode = b[0x34] | b[0x35] << 8;
            const unsigned fade = distance_fade.value_or(0x80);
            const unsigned alpha = (fade * b[0x23]) >> 7;
            // Its shine distance (+0x73, 0x18 for a class with metal packets, 32 for the Gadgetron
            // logo) for the metal pass, in the tie-only `lights` slot as -1 - distance (mesh.vert).
            if (b[0x73] != 0) {
                instance.lights = -1 - static_cast<int>(b[0x73]);
            }
            if ((mode & 0x200) != 0) {
                instance.blend = 2;
                instance.tint[3] = static_cast<float>(alpha) / 128.0f;
            } else if (fade < 0x80 || (mode & 8) != 0) {
                instance.blend = 3;
                instance.tint[3] = static_cast<float>(alpha) / 128.0f;
            } else if (alpha < 0x80) {
                instance.blend = 1;
                instance.tint[3] = static_cast<float>(alpha) / 128.0f;
            }
        }
        // Its light word (+0x38: set 0, set 1, the cross-fade) and ambient colour (+0x3C), as
        // MobyProc reads them to light it.
        if (!g->level.light_sets.empty() && m.address + 0x100 <= ram.size()) {
            const std::uint8_t* b = ram.data() + m.address;
            instance.moby_light =
                {static_cast<float>(b[0x38]),
                 static_cast<float>(b[0x39]),
                 static_cast<float>(b[0x3A]) / 256.0f,
                 1.0f};
            // w: with mode bit 0x10, 1 + its glow word's RGB (+0x90 in Ratchet & Clank), which
            // MobyProc's glow list draws its glow packets in; 0 without (a 24-bit integer is exact
            // in a float).
            const std::uint32_t at = g->profile->moby_glow_word();
            const std::uint32_t glow =
                static_cast<std::uint32_t>(b[at] | b[at + 1] << 8 | b[at + 2] << 16);
            instance.moby_ambient =
                {b[0x3C] / 128.0f,
                 b[0x3D] / 128.0f,
                 b[0x3E] / 128.0f,
                 (b[0x34] & 0x10) != 0 ? static_cast<float>(glow) + 1.0f : 0.0f};
            if (point_bank != 0 && point_lit.size() < kPointLitMobys) {
                if (const auto light = moby_point_light(ram, point_bank, m.address)) {
                    point_lit.push_back(*light);
                    instance.moby_light[0] += 256.0f * static_cast<float>(point_lit.size());
                }
            }
        }
        // Preserve explicit gameplay colour writes while that RGB is still
        // current. Countdown crates restore their saved colour between flashes;
        // once restored they automatically resume normal GI. No class whitelist.
        if (m.address + 0x3f <= ram.size()) {
            const auto* b = ram.data() + m.address;
            const unsigned colour = b[0x3c] | b[0x3d] << 8 | b[0x3e] << 16;
            if (g->scripted_moby_colours.matches(g->loaded, m.address, colour)) {
                instance.moby_light[3] = -1.0f;
            }
        }
        // Drawn by the game with a camera of its own: placed so the world's camera sees it where
        // that camera did (M' = V_world^-1 V_own M), as ReRAC draws the menu's frame objects.
        for (const auto& [address, camera] : g->moby_cameras) {
            if (into_texture.contains(m.address)) {
                break;
            }
            if (address == m.address && instance.moby_light[3] == 1.0f) {
                // One of the 2D layer's: in a picture wider than the game's it is cut at the game's
                // picture's sides (mesh.vert), where those parked beside the screen stay unseen.
                instance.moby_light[3] = 2.0f;
            }
            if (address == m.address) {
                instance.world_sphere = {};  // this object belongs to another camera's space
                instance.occlusion = scene::Pvs::always;
            }
            // Drawn on the screen with its own camera: not placed in the world's.
            if (address == m.address && on_screen.contains(m.address)) {
                break;
            }
            if (address == m.address
                && (camera.camera_position != state.camera_position
                    || camera.forward != state.forward)) {
                instance.matrix = renderer::multiply(
                    rigid_inverse(state.view()), renderer::multiply(camera.view(), instance.matrix)
                );
                break;
            }
        }
        bool attached_bones = false;
#ifdef OPENRAC_LIGHT_AUTHORING
        if (attached_catalog && !attached_catalog->objects.empty()) {
            const auto lights = attached_catalog->objects.find(
                enhanced::object_light_key(g->level, scene::Layer::Mobys, instance.model)
            );
            attached_bones =
                lights != attached_catalog->objects.end()
                && std::any_of(lights->second.begin(), lights->second.end(), [](const auto& light) {
                       return light.enabled && !light.bone.empty();
                   });
        }
#endif

        // Unweighted joints are needed only by bone attachments or explicit developer inspection.
        const auto& model = g->level.models[instance.model];
        const int joints =
            std::max(model.joints, (inspect_bones || attached_bones) ? int(model.bones.size()) : 0);
        if (debug && g->index % 100 == 0 && m.class_id == 0) {
            // The hero, for bring-up: where he is and what his animation fields and pose say.
            const auto pose = scene::moby_palette(ram, m.address);
            const std::uint8_t* b = ram.data() + m.address;
            float t = 0.0f;
            std::memcpy(&t, b + 0x54, 4);
            bool finite = !pose.empty();
            for (const auto& f : pose) {
                for (const auto& row : f) {
                    for (float x : row) {
                        finite = finite && std::isfinite(x) && std::fabs(x) < 1.0e6f;
                    }
                }
            }
            log::info(
                "  hero at {:#x}: position {:.2f} {:.2f} {:.2f}, scale {:.3f}, "
                "keys {}:{} -> {}:{} t {:.3f}, {} joints posed ({} in the mesh), {}, mode {:#x}",
                m.address,
                m.matrix[12],
                m.matrix[13],
                m.matrix[14],
                m.matrix[0],
                b[0x52],
                b[0x50],
                b[0x53],
                b[0x51],
                t,
                pose.size(),
                joints,
                finite ? "finite" : "NOT finite",
                static_cast<unsigned>(b[0x34] | b[0x35] << 8)
            );
        }
        // Its pose, evaluated after the loop with the others' on several threads.
        if (animate && joints > 0) {
            const bool target = into_texture.contains(m.address) || on_screen.contains(m.address);
            pose_jobs.push_back(
                {m.address, joints, target, target ? target_mobys.size() : live.size()}
            );
        }
        // Only world instances attach to water; menu and render-to-texture previews keep their poses.
        instance.floats_on_water = g->profile->floats_on_water(m.class_id)
                                   && instance.moby_light[3] < 1.5f
                                   && !into_texture.contains(m.address);
        if (const auto t = into_texture.find(m.address); t != into_texture.end()) {
            target_mobys.push_back({m.address, instance, *t->second});
            target_mobys.back().camera.focal_x = frame_focal[0];
            target_mobys.back().camera.focal_y = frame_focal[1];
            continue;
        }
        if (const auto t = on_screen.find(m.address); t != on_screen.end()) {
            target_mobys.push_back({m.address, instance, *t->second, true});
            continue;
        }
        const bool owned_by_hero = hero_rig.owns(m.address);
        // Keep the body fallback for games without an attachment callback or during state transitions.
        instance.hero_shadow_anchor = hero_rig.anchors(m.address, m.class_id == 0 && !extra);
        instance.hero_shadow_caster = (owned_by_hero || instance.hero_shadow_anchor)
                                      && openrac_game_has_level(g->loaded)
                                      && instance.moby_light[3] < 1.5f && instance.tint[3] > 0;
        instance.gameplay_essential |= owned_by_hero || instance.hero_shadow_anchor;
        live_joints.push_back(instance.palette >= 0 ? joints : 0);
        if (every_shadow && g->profile->shadow_directions() != 0
            && openrac_game_has_level(g->loaded) && animate && !extra
            && instance.moby_light[3] < 1.5f && instance.tint[3] > 0.0f) {
            const auto directions = OPENRAC_DATA(g->profile->shadow_directions());
            const auto cast =
                g->profile->shadow_slab_on_up_axis()
                    ? scene::moby_shadow_state_on_axis(ram, m.address, directions, state.view())
                    : scene::moby_shadow_state(ram, m.address, directions, state.view());
            if (cast.enabled) {
                auto capsules = scene::moby_shadows(ram, m.address, m.matrix, cast.scale);
                if (debug && g->index % 100 == 0) {
                    log::info(
                        "Moby shadow {:#x} class {}: {} proxies, slot {}, extrusion {:.2f} {:.2f} "
                        "{:.2f}, slab {:.3f}..{:.3f}, scale {:.3f}",
                        m.address,
                        m.class_id,
                        capsules.size(),
                        cast.direction_index,
                        cast.extrusion[0],
                        cast.extrusion[1],
                        cast.extrusion[2],
                        cast.heights[0],
                        cast.heights[1],
                        cast.scale
                    );
                }
                if (!capsules.empty()) {
                    shadow_sets.push_back(
                        {m.address, std::move(capsules), cast.extrusion, cast.heights}
                    );
                }
            }
        }
        if (!every_shadow && g->profile->shadow_directions() != 0
            && openrac_game_has_level(g->loaded) && animate && instance.hero_shadow_anchor
            && instance.moby_light[3] < 1.5f && instance.tint[3] > 0.0f) {
            const auto directions = OPENRAC_DATA(g->profile->shadow_directions());
            shadow_state = scene::moby_shadow_state(ram, m.address, directions, state.view());
            if (shadow_state.enabled) {
                shadows = scene::moby_shadows(ram, m.address, m.matrix, shadow_state.scale);
                shadow_moby = m.address;
            }
            if (debug && g->index % 100 == 0) {
                log::info(
                    "Hero shadow: {} proxies, slot {}, table {:#x}, "
                    "extrusion {:.2f} {:.2f} {:.2f}, slab {:.3f}..{:.3f}, scale {:.3f}",
                    shadows.size(),
                    shadow_state.direction_index,
                    directions,
                    shadow_state.extrusion[0],
                    shadow_state.extrusion[1],
                    shadow_state.extrusion[2],
                    shadow_state.heights[0],
                    shadow_state.heights[1],
                    shadow_state.scale
                );
            }
        }
        if (animate && instance.hero_shadow_anchor && g->game.starts_with("rac1-") && g->loaded == 1
            && enhanced_graphics_available() && settings::get().enhanced_graphics) {
            const auto pose = scene::moby_pose_matrices(ram, m.address);
            // Ratchet's two palm roots, identified by their finger children in the
            // extracted class-0 skeleton; joint 56 also carries the wrench attachment.
            constexpr std::array<unsigned, 2> palms{56, 76};
            for (unsigned hand = 0; hand < palms.size(); ++hand) {
                if (palms[hand] >= pose.size()) {
                    continue;
                }
                const auto& translation = pose[palms[hand]][3];
                const auto point = renderer::transform_point(
                    m.matrix,
                    {translation[0] / 1024.f, translation[1] / 1024.f, translation[2] / 1024.f}
                );
                if (std::all_of(point.begin(), point.end(), [](float value) {
                        return std::isfinite(value);
                    })) {
                    water_hands[hand] = point;
                }
            }
        }
        live.push_back(instance);
        live_addresses.push_back(m.address);
    }
    // The mods' props (interface 1.12): their meshes where the mods posed them, lit as a moby of the
    // level is, identified across frames by a key no game address takes.
    if (!only_own_camera && !under_menu) {
        const std::vector<mods::PropDraw> props = mods::props_drawn();
        for (std::size_t i = 0; i < props.size(); ++i) {
            const auto model =
                std::find_if(g->prop_models.begin(), g->prop_models.end(), [&](const auto& m) {
                    return m.first == props[i].mesh;
                });
            if (model == g->prop_models.end()) {
                continue;
            }
            scene::Instance instance{model->second, props[i].matrix, props[i].tint};
            instance.moby_light = {0.0f, 0.0f, 0.0f, 1.0f};
            instance.occlusion = scene::Pvs::always;
            instance.blend = props[i].blend;
            if (instance.blend != 0) {
                // A glow or a spark: drawn with the mobys' blended groups, no shadow.
                live.push_back(instance);
                live_addresses.push_back(0xFFFFFF00u + static_cast<std::uint32_t>(props[i].id));
                live_joints.push_back(0);
                continue;
            }
            // A prop casts the hero's kind of shadow (a board under his feet goes with his): in the
            // Enhanced shadow map as his equipment does, and, while the game draws his shadow, as
            // one capsule along the mesh's longer side, half its shorter side thick.
            instance.hero_shadow_caster = openrac_game_has_level(g->loaded);
            if (shadow_moby != 0 && model->second < g->level.models.size()) {
                const scene::Model& mesh = g->level.models[model->second];
                const renderer::Vec3 size{
                    mesh.max[0] - mesh.min[0], mesh.max[1] - mesh.min[1], mesh.max[2] - mesh.min[2]
                };
                // The mesh's up is its shortest side; the capsule runs along the longest.
                int along = 0, across = 0;
                for (int k = 1; k < 3; ++k) {
                    along = size[k] > size[along] ? k : along;
                }
                int up = along == 0 ? 1 : 0;
                for (int k = 0; k < 3; ++k) {
                    up = k != along && size[k] < size[up] ? k : up;
                }
                across = 3 - along - up;
                renderer::Vec3 a{}, b{};
                for (int k = 0; k < 3; ++k) {
                    a[k] = b[k] = 0.5f * (mesh.min[k] + mesh.max[k]);
                }
                const float radius = 0.5f * size[across];
                a[along] = mesh.min[along] + radius;
                b[along] = mesh.max[along] - radius;
                renderer::ShadowCapsule capsule;
                capsule.a = renderer::transform_point(props[i].matrix, a);
                capsule.b = renderer::transform_point(props[i].matrix, b);
                // The matrix's scale along the side the radius was measured on.
                const renderer::Vec3 o =
                    renderer::transform_point(props[i].matrix, renderer::Vec3{});
                renderer::Vec3 unit{};
                unit[across] = 1.0f;
                const renderer::Vec3 u = renderer::transform_point(props[i].matrix, unit);
                const float scale = std::sqrt(
                    (u[0] - o[0]) * (u[0] - o[0]) + (u[1] - o[1]) * (u[1] - o[1])
                    + (u[2] - o[2]) * (u[2] - o[2])
                );
                capsule.radius_a = capsule.radius_b = radius * scale;
                capsule.segments_a = capsule.segments_b = 8;
                shadows.push_back(capsule);
            }
            live.push_back(instance);
            live_addresses.push_back(0xFFFFFF00u + static_cast<std::uint32_t>(props[i].id));
            live_joints.push_back(0);
        }
    }
    // The poses: each a pure function of the game's memory, evaluated in parallel (the costliest
    // part of reading the scene, about 1.5 ms a tick for 180 mobys on a phone's fast core), then
    // given palette slots in the loop's order, as one thread gave them.
    std::vector<std::vector<assets::rac1::JointMatrix>> poses(pose_jobs.size());
    parallel::for_each(pose_jobs.size(), [&](std::size_t i) {
        poses[i] = scene::moby_palette(ram, pose_jobs[i].address);
    });
    for (std::size_t k = 0; k < pose_jobs.size(); ++k) {
        const PoseJob& job = pose_jobs[k];
        const auto& pose = poses[k];
        if (pose.empty()) {
            continue;
        }
        scene::Instance& instance = job.target ? target_mobys[job.index].instance : live[job.index];
        instance.palette = static_cast<int>(palette.size());
        if (!job.target) {
            live_joints[job.index] = job.joints;
        }
        for (int j = 0; j < job.joints; ++j) {
            // Columns: the images of the axes, then the translation, which the palette has in
            // packed units (the mesh is in packed units / 1024).
            scene::LevelScene::JointColumns c{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            if (static_cast<std::size_t>(j) < pose.size()) {
                const auto& f = pose[static_cast<std::size_t>(j)];
                for (int i = 0; i < 3; ++i) {
                    c[4 * i + 0] = f[i][0];
                    c[4 * i + 1] = f[i][1];
                    c[4 * i + 2] = f[i][2];
                    c[4 * i + 3] = 0.0f;
                }
                c[12] = f[3][0] / 1024.0f;
                c[13] = f[3][1] / 1024.0f;
                c[14] = f[3][2] / 1024.0f;
                c[15] = 1.0f;
            }
            palette.push_back(c);
        }
    }
    return {
        std::move(live),
        std::move(live_addresses),
        std::move(live_joints),
        std::move(palette),
        std::move(shadows),
        shadow_moby,
        shadow_state,
        std::move(shadow_sets),
        std::move(target_mobys),
        water_hands,
        std::move(point_lit),
        shrub_draws(ram, world_ran && !under_menu),
        scenery_matrices(ram, world_ran && !under_menu, state)
    };
}

// Gives the scene this tick's fog and the lights as the game holds them.
void light_scene(std::span<const std::uint8_t> ram, const host::GameState& state) {
    g->scene.set_fog(
        state.fog_colour, state.fog_near, state.fog_far, state.fog_near_f, state.fog_far_f
    );
    // The light bank as the game holds it now (16 sets of 0x40 bytes; the executable's, each
    // level's copy elsewhere): the level loader fills it from the gameplay file, and the game changes
    // sets at run time (the page menus write set 14 for their frame mobys, fog zones the hero's).
    if (light_bank_address() != 0) {
        const gaddr bank = openrac_relocate_data(light_bank_address());
        if (bank + 16 * 0x40 <= ram.size()) {
            std::vector<float> sets(16 * 16);
            std::memcpy(sets.data(), ram.data() + bank, sets.size() * sizeof(float));
            g->scene.set_light_sets(sets);
            if (bank + 16 * 0x40 + 8 * 0x20 <= ram.size()) {
                std::array<float, 64> points{};
                std::memcpy(points.data(), ram.data() + bank + 16 * 0x40, sizeof(points));
                g->scene.set_point_lights(points);
            }
        }
    }
    // The game's occlusion mask, which picks retail's fallback outside the baked grid.
    std::optional<scene::Pvs::Mask> occlusion;
    if (occlusion_address() != 0) {
        const gaddr mask = openrac_relocate_data(occlusion_address());
        if (mask + sizeof(scene::Pvs::Mask) <= ram.size()) {
            occlusion.emplace();
            std::memcpy(occlusion->data(), ram.data() + mask, occlusion->size());
        }
    }
    g->scene.set_game_pvs(occlusion);
}

// Keeps this tick's scene for the frames drawn until the next one (Kept), the last tick's as
// where they move from, and counts the tick.
void keep_tick(
    std::span<const std::uint8_t> ram,
    const host::GameState& state,
    std::uint32_t draws,
    TickMobys&& mobys
) {
    const profiling::Zone trace("Capture world");

    // What this tick drew, kept for the frames drawn until the next one (between): the scene
    // then moves from the last tick's camera and mobys to this one's.
    Kept& k = *g->kept;
    if (k.level != g->loaded) {
        k.has_state = false;
        k.live.clear();
        k.addresses.clear();
        k.joints.clear();
        k.shadows.clear();
        k.shadow_moby = 0;
        k.shadow_sets.clear();
        k.shrubs.clear();
        k.scenery = {};
        k.prev_scenery = {};
        k.level = g->loaded;
        // A new level's frames cost what they cost: the system forgets the last level's
        // (performance_hint.h).
        platform::workload_reset();
        g->water_impulses.clear();
    }
    const bool adjacent = g->index == k.frame + 1;
    k.prev_state = k.state;
    k.has_prev_state = k.has_state && adjacent;
    k.prev_matrices.clear();
    k.prev_spheres.clear();
    k.prev_poses.clear();
    for (std::size_t i = 0; adjacent && i < k.live.size() && i < k.addresses.size(); ++i) {
        k.prev_matrices[k.addresses[i]] = k.live[i].matrix;
        k.prev_spheres[k.addresses[i]] = k.live[i].world_sphere;
        if (i < k.joints.size() && k.joints[i] > 0 && k.live[i].palette >= 0) {
            k.prev_poses[k.addresses[i]] = {k.live[i].palette, k.joints[i]};
        }
    }
    k.water_impulses.swap(g->water_impulses);
    g->water_impulses.clear();
    if (adjacent && k.has_state) {
        water_projectile_impulses(*g->profile, state.mobys, k.state.mobys, k.water_impulses);
    }
    k.prev_palette = std::move(k.palette);
    k.prev_shadows = std::move(k.shadows);
    k.prev_shadow_moby = k.shadow_moby;
    k.prev_shadow_sets = std::move(k.shadow_sets);
    // A captured-pose gap has no valid one-tick interpolation endpoint.
    if (!adjacent) {
        k.prev_palette.clear();
        k.prev_shadows.clear();
        k.prev_shadow_moby = 0;
        k.prev_shadow_sets.clear();
    }
    k.shadows = std::move(mobys.shadows);
    k.shadow_moby = mobys.shadow_moby;
    k.prev_shadow_state = k.shadow_state;
    k.shadow_state = mobys.shadow_state;
    k.shadow_sets = std::move(mobys.shadow_sets);
    k.water_hands = mobys.water_hands;
    if (mobys.shrubs) {
        k.shrubs = std::move(*mobys.shrubs);
    }
    if (mobys.scenery) {
        k.prev_scenery = adjacent ? std::move(k.scenery.matrices)
                                  : std::array<std::vector<std::array<float, 12>>, 2>{};
        k.scenery = std::move(*mobys.scenery);
    }
    k.hero_airborne =
        g->profile->hero_state() != 0
            ? scene::hero_airborne(
                  ram, OPENRAC_DATA(g->profile->hero_state()), g->profile->hero_air_frames()
              )
            : std::nullopt;
    std::uint32_t mode = 0;
    // The level's mode while a real-time cutscene plays (the profile's). Movies bypass world
    // reconstruction.
    const auto cutscene_mode = g->profile->cutscene_mode();
    const bool has_mode = openrac_game_mode != nullptr && openrac_game_mode(&mode) != 0;
    k.cutscene = has_mode && cutscene_mode && mode == *cutscene_mode;
    // Both games report mode zero for normal play. Unknown modes retain the native lens.
    k.gameplay_camera = has_mode && mode == 0;
    k.state = state;
    k.has_state = true;
    k.live = std::move(mobys.live);
    k.target_mobys = std::move(mobys.target_mobys);
    k.texture_variant = g->profile->texture_variant(ram);
    k.addresses = std::move(mobys.addresses);
    k.joints = std::move(mobys.joints);
    k.palette = std::move(mobys.palette);
    k.draws = draws;
    k.haze.clear();
    for (const mods::HazeDraw& h : mods::haze_drawn()) {
        k.haze.push_back({h.position[0], h.position[1], h.position[2], h.radius, h.strength});
    }
    k.has_camera = state.forward[0] != 0.0f || state.forward[1] != 0.0f || state.forward[2] != 0.0f;
    g->last_tick_menu =
        k.before_world >= 0x40000;  // the last tick's, read before this one's is kept
    k.frame = g->index++;
}

// The current tick's packet data is cheap to capture even when its world pose will be dropped.
struct TickPackets {
    std::vector<std::uint8_t> packets;
    std::vector<renderer::EffectVertex> strip_vertices;
    std::vector<renderer::EffectStrip> strips;
    std::size_t before_world = 0;
    const std::vector<renderer::UiQuad>* named_quads = nullptr;
};

// Keeps this tick's 2D path (the display list's direct GIF data, from `chain` where the game
// sends it so, else the list the game's profile says is shown in the program `level`) and where its
// world goes in it. Returns the native quads its markers name, if they are still kept.
TickPackets capture_2d_path(
    std::span<const std::uint8_t> ram,
    std::uint32_t chain,
    int level,
    std::uint32_t draws,
    bool debug
) {
    TickPackets k;
    // OPENRAC_DUMP_DRAWS=N or N-M: the frames whose 2D draws are logged.
    static const std::optional<FrameRange> dump_at = env_frames(env::Var::DumpDraws);
    if (dump_at) {
        renderer::g_dump_draws = dump_at->holds(g->index);
    }
    if (renderer::g_dump_draws) {
        log::info("frame {}: the 2D path's draws", g->index);
    }
    // The 2D path: the frame's direct GIF data, drawn by the direct renderer.
    // RAC1 keeps two alternating list buffers and the sent list is one tick behind; Deadlocked
    // submits the title's list through VIF1 DMA without that RAC1 buffer-index convention, and
    // so does Going Commando: their profiles say so (sends_display_list).
    const bool sent_chain = g->profile->sends_display_list();
    const std::uint32_t shown = sent_chain ? chain : g->profile->shown_chain(ram, level);
    std::vector<renderer::EffectVertex> strip_vertices;
    std::vector<renderer::EffectStrip> strips;
    std::size_t before_world = SIZE_MAX;
    std::size_t first_offscreen = SIZE_MAX;
    std::size_t world_marker = SIZE_MAX;
    std::size_t moby_marker = SIZE_MAX;
    std::vector<QuadMark> quad_marks;
    g->texture_mobys.clear();
    renderer::g_quads_sample_targets = g->profile->own_mobys_into_narrow_buffers();
    draw_coverage::begin_tick(level, g->index);
    std::vector<std::uint8_t> packets = direct_packets(
        ram,
        shown,
        &strip_vertices,
        &strips,
        &g->effect_uploads,
        &before_world,
        &first_offscreen,
        &world_marker,
        &moby_marker,
        &quad_marks,
        &g->texture_mobys
    );
    // The effect quads the list marks (openrac_game_effect_quad_marker) take the TEST and ZBUF in
    // effect there: Going Commando's flight sparks and flames test alpha GEQUAL 0x60, and the
    // pixels that pass write Z. A list of this tick marks this tick's quads. Going Commando's list
    // is the last tick's (the game builds the next while one is sent), so it marks the last tick's
    // quads: each of this tick's takes the registers of the last tick's quads drawn with the same
    // TEX0, TEX1, CLAMP and ALPHA, when they all had the same (a quad new in this tick, or one whose
    // state the last tick drew with two TESTs, is drawn as before: no alpha test, no Z).
    std::size_t quads_marked = 0, marks_stale = 0;
    const auto apply = [&](renderer::EffectQuad& q, std::uint64_t test, std::uint64_t zbuf) {
        q.test = test;
        q.zbuf = zbuf;
        q.registers = test != 0;
        ++quads_marked;
    };
    const std::uint32_t generation = g->effect_generation & 0xFFF;
    const std::uint32_t last_generation = (g->effect_generation - 1) & 0xFFF;

    struct Registers {
        std::uint64_t test, zbuf;
        bool ambiguous;
    };

    std::map<std::array<std::uint64_t, 4>, Registers> by_state;
    for (const QuadMark& m : quad_marks) {
        const std::uint32_t index = m.marker & 0xFFFFF;
        const std::uint64_t zbuf = m.zbuf_seen ? m.zbuf : 0;
        if ((m.marker >> 20) == generation && index < g->effects.size()) {
            apply(g->effects[index], m.test, zbuf);
        } else if ((m.marker >> 20) == last_generation && index < g->effect_states_before.size()) {
            const auto [at, added] = by_state.try_emplace(
                g->effect_states_before[index], Registers{m.test, zbuf, false}
            );
            if (!added && (at->second.test != m.test || at->second.zbuf != zbuf)) {
                at->second.ambiguous = true;
            }
        } else {
            ++marks_stale;
        }
    }
    if (!by_state.empty()) {
        for (renderer::EffectQuad& q : g->effects) {
            const auto found = by_state.find({q.tex0, q.tex1, q.clamp, q.alpha});
            if (found != by_state.end() && !found->second.ambiguous) {
                apply(q, found->second.test, found->second.zbuf);
            }
        }
    }
    if (debug && marks_stale > 0) {
        log::info(
            "frame {}: {} effect quad marks name another tick's quads ({} applied)",
            g->index,
            marks_stale,
            quads_marked
        );
    }
    stats::gauge("effect quads marked", double(quads_marked));
    // The markers in a list index the quads of the tick that built it, and the list shown is the
    // one the last tick built (the game builds the next while this one is sent), one tick behind
    // the quads just recorded; FadeToBlack sends an old list again. Drawn with this tick's quads, a
    // marker named another quad whenever they changed, and the HUD (the bolts, the health, a
    // HelpDesk frame) and the menus flashed for a tick: the white flicker, and the palettes landing
    // on pixels that 8587dcfa held the last layer for. Each marker names its tick's generation
    // (ui_quad); the quads of the last four ticks that drew any are kept, and the list is drawn
    // with the ones it names.
    if (!g->ui_quads.empty()) {
        const std::uint32_t generation = g->ui_generation & 0x3F;
        g->ui_kept[generation & 3] = {generation, std::move(g->ui_quads)};
        ++g->ui_generation;
    }
    const std::vector<renderer::UiQuad>* named_quads = nullptr;
    if (const auto named = marker_generation(packets)) {
        for (const auto& [generation, quads] : g->ui_kept) {
            if (generation == *named && !quads.empty()) {
                named_quads = &quads;
            }
        }
        if (debug && !named_quads) {
            log::info(
                "frame {}: the 2D list names the quads of generation {}, no longer kept; the last "
                "ones stand in",
                g->index,
                *named
            );
        }
    }
    k.strip_vertices = std::move(strip_vertices);
    k.strips = std::move(strips);
    k.packets = std::move(packets);
    // Only a level's frame is ordered so: the boot program (title, main menu) starts VU1 programs
    // of its own before its 2D path, and draws its world natively, all of its 2D path after it.
    // A frame with mobys and no VU1 program in its chain (the vendor's menu: its frozen world, then
    // the vendor, which the port draws natively, then its screens rendered to textures and put on
    // the monitors): what comes before the first render to a texture is the picture under the mobys.
    // A level's frame whose terrain was drawn has the world's marker where it was (put by the
    // terrain pass, rac1/host/render_native.c): the split is there, or at a VU1 program started
    // before it. The first VU1 program alone was too fragile: the port's world renderers send
    // none, and the first is the world text's sprite pass (func_001F7B70), which runs only while
    // there is any. On Kerwan in play there was none: all of the 2D path, the HUD with it, was
    // taken as coming before the world and drawn under it. On Rilgar the split moved from frame
    // to frame, and the HUD and the sea's layers flickered.
    if (world_marker != SIZE_MAX) {
        before_world = std::min(before_world, world_marker);
    }
    if (before_world == SIZE_MAX && (draws & 0x10u) != 0) {
        before_world = first_offscreen;
    }
    // A game whose menus draw their mobys on the screen late in their 2D path (Going Commando's
    // Ship Shack: its frame saved to a texture first, then its panels, then the ship): what comes
    // before them is under them, wherever the first render to a texture was.
    if (moby_marker != SIZE_MAX && g->profile->own_mobys_split_the_2d()) {
        before_world = moby_marker;
    }
    // The flight between planets is drawn as the boot program draws it, also when it starts from a
    // planet whose level program is still loaded (world_to_draw: the flight's world): split there,
    // the planet's name was taken as coming before the world and the stars hid it.
    k.before_world = openrac_game_has_level(level) && world_to_draw(ram, level) >= 0
                         ? std::min(before_world, k.packets.size())
                         : 0;
    k.named_quads = named_quads;
    return k;
}

bool sends_image(std::span<const std::uint8_t> packets);

// Decides how this tick's frames start (frontend/tick_picture.h): the last frame standing, the last
// frame under the tick's 2D, or a cleared frame. `level`: the level program loaded.
void choose_repeat_frame(int level, bool debug) {
    Kept& k = *g->kept;
    const TickPicture picture = g->tick_pictures.choose(
        {k.draws,
         k.has_camera,
         openrac_game_has_level(level) != 0,
         k.packets.size(),
         k.before_world,
         g->frame.id() != 0,
         level}
    );
    g->repeat_frame = picture == TickPicture::Repeat;
    g->overdraw = picture == TickPicture::Overdraw;
    // The picture stands, but the chip still took the images the tick's list sent: kept, a block
    // the game sends once is there when a later tick draws from it.
    if (g->repeat_frame && g->hud && g->profile->repeated_ticks_send_images()
        && sends_image(k.packets)) {
        g->hud->retain_uploads(k.packets);
    }
    if (picture != TickPicture::Draw && debug) {
        log::info(
            "frame {}: {}",
            g->index,
            g->repeat_frame ? "the last frame stands" : "the 2D goes over the last frame"
        );
    }
    if (debug && k.frame % 100 == 0 && !k.strips.empty()) {
        int bare = 0;
        for (const auto& st : k.strips) {
            bare += st.upload_count == 0 ? 1 : 0;
        }
        log::info(
            "  {} strips ({} vertices, {} without their texture's upload), the first TEX0 {:#x} "
            "ALPHA {:#x}",
            k.strips.size(),
            k.strip_vertices.size(),
            bare,
            k.strips.front().tex0,
            k.strips.front().alpha
        );
        // The ones sent before the world's mobys (EffectStrip::before_mobys), drawn with the
        // liquids, by their GS state.
        std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, int> early;
        for (const auto& st : k.strips) {
            if (st.before_mobys) {
                ++early[{st.alpha, st.test, st.z_masked}];
            }
        }
        for (const auto& [state, count] : early) {
            log::info(
                "    {} strips before the mobys: ALPHA {:#x} TEST {:#x}{}",
                count,
                std::get<0>(state),
                std::get<1>(state),
                std::get<2>(state) ? ", depth masked" : ""
            );
        }
    }
}

// Keeps the world effects, the sky and the particles the game drew this tick, and the native
// quads its 2D list names (`named_quads`, capture_2d_path), and clears what was gathered.
void keep_effects(
    std::span<const std::uint8_t> ram,
    const host::GameState& state,
    const std::vector<renderer::UiQuad>* named_quads
) {
    const profiling::Zone trace("Capture effects");

    Kept& k = *g->kept;
    // The game's quads as the next tick's list marks them, by index (capture_2d_path).
    g->effect_states_before.clear();
    for (const renderer::EffectQuad& e : g->effects) {
        g->effect_states_before.push_back({e.tex0, e.tex1, e.clamp, e.alpha});
    }
    effect_textures();  // before the sky's sprites go in front of the effect quads
    sky_sprite_quads(state);
    particle_quads(ram, state);
    // The last tick's corners of the quads that carry an id, when it was the tick before this
    // one: a frame drawn between ticks moves each from there, as the mobys move (scene_hud.cpp's
    // set_packets). Kept effects of an older tick, or of a tick that kept no world, move nothing.
    k.prev_effect_corners.clear();
    if (g->profile->interpolates_effect_quads() && k.effects_frame + 1 == k.frame) {
        for (const renderer::EffectQuad& e : k.effects) {
            if (e.id != 0) {
                auto& corners = k.prev_effect_corners[e.id];
                std::memcpy(corners.data(), e.corner, sizeof(e.corner));
            }
        }
    }
    k.effects_frame = k.frame;
    ++g->effect_generation;
    k.effects = std::move(g->effects);
    k.effect_uploads = std::move(g->effect_uploads);
    k.sky_shells = g->sky_shells;
    k.sky_shell_mask = std::exchange(g->sky_shell_mask, 0u);
    // The list's own tick's quads (above). The game's FadeToBlack (after a story card, between
    // scenes) sends the last display list again for each of its frames, with its black over it,
    // and draws nothing itself: its markers still name the tick that built it, which is kept.
    // Without those quads the card's two lines were left out and only the band behind them was
    // drawn, a blue strip across the picture for the fade's two frames. A list whose tick is no
    // longer kept keeps the quads last drawn; a list without markers does not look at them.
    if (named_quads) {
        k.ui_quads = *named_quads;
    }
    g->ui_quads.clear();
    g->effects.clear();
    g->effect_uploads.clear();
    g->particles.clear();
    g->sky_sprites.clear();
}

// Reports what this tick drew: in the log (OPENRAC_FRAME_LOG) and to the debug overlay.
void report_tick(std::span<const std::uint8_t> ram, int level) {
    Kept& k = *g->kept;
    // OPENRAC_FRAME_LOG=FROM-TO: what each tick in that range drew, in the log.
    static const std::optional<FrameRange> frame_log = env_frames(env::Var::FrameLog);
    if (frame_log && frame_log->holds(k.frame)) {
        log::info(
            "tick {}: draws {:#x} camera {} 2D bytes {} before world {} quads {} effects {} "
            "mobys {}",
            k.frame,
            k.draws,
            k.has_camera,
            k.packets.size(),
            k.before_world,
            k.ui_quads.size(),
            k.effects.size(),
            k.live.size()
        );
    }
    // [debug tools] What the game holds this tick, for the overlay and the dump.
    debug_overlay::tick(
        {g->game.c_str(),
         ram,
         level,
         &k.state,
         k.draws,
         k.frame,
         k.packets.size(),
         k.ui_quads.size(),
         k.effects.size(),
         k.strips.size(),
         k.live.size()}
    );
}

// Whether a tick's 2D path sends the chip an image (an A+D write of TRXDIR 0, host to chip): the
// texture exists in the window only when that frame is drawn, as the 2D path's interpreter keeps
// the image it carries (GifInterpreter::image_data). The game sends many once (rac1: the Galactic
// Map's and the mission page's planet picture), so left out, it was never seen and the picture
// stayed empty or half drawn. A false match only keeps a frame that could have been left out.
bool sends_image(std::span<const std::uint8_t> packets) {
    // An A+D write is one GIF qword: the register's value in its low 8 bytes, the register's
    // address in the byte after them (documented: the GS manual's A+D packing).
    constexpr std::size_t kQwordBytes = 16;
    constexpr std::size_t kAddressByte = 8;
    // TRXDIR's XDIR field (bits 0-1) for a transfer from the host to the chip (documented).
    constexpr std::uint32_t kHostToLocal = 0;

    // Every qword is read as one: a GIF tag or image data read so is the false match above.
    for (std::size_t at = 0; at + kQwordBytes <= packets.size(); at += kQwordBytes) {
        std::uint64_t value = 0;
        std::memcpy(&value, packets.data() + at, sizeof(value));

        // A transfer to the chip: its image follows in the packets.
        if (packets[at + kAddressByte] == renderer::gs::kTrxdir
            && renderer::gs::bits(value, 0, 2) == kHostToLocal) {
            return true;
        }
    }

    return false;
}

// A skipped gameplay picture still applies its uploads; menu/background copies need a drawn frame.
bool retain_tick_uploads() {
    const Kept& tick = *g->kept;
    if (!sends_image(tick.packets)) {
        return true;
    }
    return g->hud && (tick.draws & OPENRAC_DRAW_TERRAIN) != 0
           && g->hud->retain_uploads(tick.packets);
}

// Whether the tick's own frame is left out because the tick came late. Counts and warns.
bool leave_late_frame() {
    // The tick's own frame (when frames are drawn between ticks, part way from the last tick's
    // scene, by the refresh it is shown at). A tick that comes more than a tick late (set_tick: the
    // machine draws frames slower than the game ticks) is not drawn, so that drawing never slows
    // the game's time: the next tick's frame shows where the game got to. At most three in a row,
    // so that a picture still comes; a machine slower than that runs the game slow, as it must.
    // Said when it goes on (a quarter of a hundred ticks' frames left out), not for the odd tick
    // that comes late after the system held the program up.
    static int ticks = 0;
    static int left_out = 0;
    static int said = 0;
    if (++ticks >= 100) {
        if (left_out >= 25 && said++ < 3) {
            log::warn(
                "a frame takes longer to draw ({:.1f} ms) than a game tick lasts ({:.1f} ms): "
                "{} of the last {} ticks' frames were left out to keep the game's speed. "
                "A lower render scale or fewer samples a pixel would help.",
                pacing.cost * 1000.0,
                pacing.period.count() * 1000.0,
                left_out,
                ticks
            );
        }
        ticks = 0;
        left_out = 0;
    }
    if (pacing.late > pacing.period && g->skipped < kMostSkipped && g->kept->has_camera
        && g->index > 1 && retain_tick_uploads()) {
        ++g->skipped;
        ++left_out;
        return true;
    }
    g->skipped = 0;
    return false;
}

// Applies owned uploads after the previous render job releases graphics ownership.
bool begin_capture(State& owner) {
    // A completed handoff releases every render job's scene owner before capture can mutate it.
    if (owner.kept.use_count() != 1) {
        log::error("cannot capture scene while a render job still owns it");
        return false;
    }
    // Deferred uploads belong to this frame, after the previous snapshot has finished.
    if (owner.renderer) {
        for (const auto& image : owner.pending_images) {
            owner.renderer->textures().upload(image);
        }
    }
    // What the game sent during the last tick goes again once that tick's list has been replayed.
    owner.pending_images = std::move(owner.images_after_list);
    owner.images_after_list.clear();
    return read_window_events();
}

// Uses prepared CPU values only while their level remains loaded.
host::GameState captured_state(
    std::span<const std::uint8_t> ram, int level, std::optional<PreparedTick>& prepared
) {
    // A level transition invalidates the interpretation of the previously prepared instances.
    if (prepared && prepared->level != g->loaded) {
        prepared.reset();
    }

    // Synchronous frames and transitions still capture after acquiring graphics ownership.
    return prepared ? std::move(prepared->state) : g->profile->read_state(ram, level);
}

// Draws the tick's own frame, or leaves it to between when the frame cap holds it back.
void draw_tick_frame() {
    // The frame cap may hold the tick's own frame back: the tick is never made to wait for it. The
    // frame is then drawn between ticks, when it is due (between), or not at all when the next
    // tick comes first (a cap under the game's tick rate).
    if (!frame_due(std::chrono::steady_clock::now()) && retain_tick_uploads()) {
        pacing.first_pending = true;
        return;
    }
    pacing.first_pending = false;
    draw_kept(true);
}

// Keeps effects and schedules the committed tick unless pacing leaves it out.
// `scene`: the measurement of reading the tick (common/stats.h), ended before the frame is drawn.
bool finish_tick(
    std::span<const std::uint8_t> ram,
    const host::GameState& state,
    const std::vector<renderer::UiQuad>* named_quads,
    int level,
    bool debug,
    std::optional<stats::Scope>& scene
) {
    choose_repeat_frame(level, debug);
    {
        const stats::Scope part("effects read");
        keep_effects(ram, state, named_quads);
    }
    report_tick(ram, level);
    scene.reset();

    // A late simulation tick may deliberately leave its displayed frame out.
    if (leave_late_frame()) {
        return true;
    }

    draw_tick_frame();
    return !g->render_failed;
}

}  // namespace

bool prepare_frame(std::span<const std::uint8_t> ram) try {
    const profiling::Zone trace("Prepare next scene");

    // A tick prepared on an earlier vsync whose frame() never ran (an undrawn tick keeps the last
    // picture, program/undrawn_ticks.h) is that tick's, not this one's: drawn, the galactic map's
    // first tick showed the level's mobys from the tick before.
    if (g) {
        g->prepared_tick.reset();
    }
    // Synchronous frames have no previous job to overlap, and never allocate a preparation slot.
    if (!g || !render_pending()) {
        return true;
    }

    const int level = openrac_game_loaded_overlay();

    // Loading and flight transitions need exclusive graphics ownership before changing level resources.
    if (level < 0 || world_to_draw(ram, level) != g->loaded) {
        return true;
    }

    // Late ticks may discard their world snapshot after retaining the packet uploads.
    if (pacing.late > pacing.period && g->skipped < kMostSkipped && g->world_camera) {
        return true;
    }
    const auto started = std::chrono::steady_clock::now();
    PreparedTick prepared;
    prepared.level = g->loaded;
    prepared.state = g->profile->read_state(ram, level);
    static bool noting = false;  // the profile's diagnostics run (the game's thread, at its vsync)
    if (const bool recording = effects_dump::armed(); recording || noting) {
        const host::DiagnosticsInput diagnostics{
            [](std::uint32_t address) { return OPENRAC_DATA(address); }, openrac_game_diagnostics
        };
        noting = recording;
        if (recording) {
            effects_dump::note(g->profile->diagnostics(ram, level, prepared.state, diagnostics));
        } else {
            g->profile->diagnostics_off(diagnostics);
        }
    }
    const bool world_ran = g->world_camera.has_value();
    const bool only_own_camera = !world_ran && !g->moby_cameras.empty();
    const bool under_menu = world_ran && g->world_under_menu;
    copy_world_camera(prepared.state);
    const bool debug = stats::probe("OPENRAC_DEBUG", "the game's state each 100 ticks, in the log");
    prepared.mobys = pose_mobys(ram, prepared.state, world_ran, only_own_camera, under_menu, debug);
    prepared.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    g->prepared_tick = std::move(prepared);
    return true;
} catch (const std::exception& failure) {
    g->render_failed = true;
    log::error("cannot prepare next frame: {}", failure.what());
    return false;
} catch (...) {
    g->render_failed = true;
    log::error("cannot prepare next frame: unknown exception");
    return false;
}

bool frame(std::span<const std::uint8_t> ram, std::uint32_t chain, std::uint32_t draws) {
    const profiling::Zone trace("Capture tick");

    // Resource changes require exclusive graphics ownership.
    if (!finish_render()) {
        return false;
    }
    if (!g) {
        return true;
    }
    // The window was closed.
    if (!begin_capture(*g)) {
        return false;
    }

    // [debug tools] Reading the tick from the game's memory, in parts: the "scene" row of the
    // overlay and of a performance capture (debug_overlay::begin times what is left of it).
    std::optional<stats::Scope> scene(std::in_place, "scene");

    // The level program loaded now (-1: the boot program, title and menus), as the port knows it.
    const int level = openrac_game_loaded_overlay();
    use_world(ram, level);
    auto prepared = std::exchange(g->prepared_tick, std::nullopt);
    std::optional<stats::Scope> part(std::in_place, "game state");
    host::GameState state = captured_state(ram, level, prepared);
    part.reset();
    // The world is drawn from the camera the game's world renderers used, not from whatever the
    // camera globals hold at the end of the frame (the page menu sets its own camera, at
    // (256, 256, 64) facing +x, to draw its frame objects, as ReRAC notes).
    // A tick on which the world renderers did not run but mobys were drawn with a camera of their
    // own (a page menu's frame objects): only those are drawn, as the game drew no others (the
    // pause page skips the Goodies frame while it has none).
    const bool world_ran = g->world_camera.has_value();
    const bool only_own_camera = !world_ran && !g->moby_cameras.empty();
    const bool under_menu = world_ran && g->world_under_menu;
    take_world_camera(state);
    log_camera(state, world_ran);
    follow_route(ram, level);
    trace_tick(state, draws);
    scripted_view(state.forward.data());
    // The run's frames are done.
    if (run_over()) {
        return false;
    }
    dump_ram(ram);

    // OPENRAC_DEBUG: the game's state each 100 frames, for bring-up.
    static const bool& debug =
        stats::probe("OPENRAC_DEBUG", "the game's state each 100 ticks, in the log");
    if (debug && g->index % 100 == 0) {
        log_tick_state(ram, state, level, draws);
    }
    log_mobys_near_hero(ram, state);
    auto packets = capture_2d_path(ram, chain, level, draws, debug);
    // Catch up before posing hundreds of objects for a picture that will never be displayed.
    if (pacing.late > pacing.period && g->skipped < kMostSkipped && g->index > 1 && world_ran
        && (draws & OPENRAC_DRAW_TERRAIN) != 0 && g->hud
        && g->hud->retain_uploads(packets.packets)) {
        ++g->skipped;
        ++g->index;
        g->ui_quads.clear();
        g->moby_cameras.clear();
        g->effects.clear();
        ++g->effect_generation;
        g->effect_states_before.clear();
        g->effect_uploads.clear();
        // effect_quad's notes of the cleared quads go with them: effect_textures() would read
        // them against the next tick's quads.
        g->effect_texture_ids.clear();
        g->effect_hashed.clear();
        g->effect_sources = {};
        g->particles.clear();
        g->sky_sprites.clear();
        g->sky_shell_mask = 0;
        return true;
    }
    part.emplace("moby poses");
    TickMobys mobys = prepared
                          ? std::move(prepared->mobys)
                          : pose_mobys(ram, state, world_ran, only_own_camera, under_menu, debug);
    g->moby_cameras.clear();
    stats::gauge("render preparation ms", prepared ? prepared->seconds * 1000.0 : 0.0);
    part.emplace("lights");
    light_scene(ram, state);
    g->scene.set_moby_point_lights(mobys.point_lit);
    g->scene.set_terrain_point_lights(g->profile->terrain_takes_point_lights());
    part.emplace("tick kept");
    keep_tick(ram, state, draws, std::move(mobys));
    g->kept->world_under_menu = under_menu;
    // The pause menu runs in a mode of its own, but the world under it (the picture of the frame
    // it took, or a level's world drawn live under it) is the level's, seen with the camera it was
    // paused with: a gameplay camera when the game was paused from play.
    std::uint32_t mode_now = 0;
    const auto pause_mode = g->profile->pause_mode();
    const bool paused = pause_mode && openrac_game_mode != nullptr
                        && openrac_game_mode(&mode_now) != 0 && mode_now == *pause_mode;
    if (under_menu || paused) {
        g->kept->gameplay_camera = g->paused_from_play;
    } else if (world_ran) {
        g->paused_from_play = g->kept->gameplay_camera;
    }
    g->kept->packets = std::move(packets.packets);
    g->kept->strip_vertices = std::move(packets.strip_vertices);
    g->kept->strips = std::move(packets.strips);
    g->kept->before_world = bytes_before_world(packets.before_world, under_menu);
    part.reset();
    return finish_tick(ram, state, packets.named_quads, level, debug, scene);
}

bool keep_picture() {
    if (!g) {
        return true;
    }
    if (!read_window_events()) {
        return false;
    }
    // The blank is a window frame as a drawn tick is: the scripted presses (OPENRAC_PRESS), the
    // shots and the run's end count them. Not counted, a script waiting on a frame number during
    // such a wait (the boot's) waited for ever.
    ++g->index;
    if (run_over()) {
        return false;
    }
    // The console shows its frame buffer as it is: the picture stands and nothing is drawn again.
    // Drawn again, the kept tick's 2D read the textures a load had written over since (the flight's
    // planet name came out as a solid block and its planet striped, before the landing).
    g->repeat_frame = true;
    draw_kept(false);
    return !g->render_failed;
}

}  // namespace openrac::frontend
