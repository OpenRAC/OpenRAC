// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The levels as the program reads them: each level's program, for the runtime's relocation of the
// functions levels share (load_level_program), and --check-levels, which reads every extracted
// level as the window would, optionally with --mods overrides (check_levels).

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "common/breadcrumbs.h"
#include "common/log.h"
#include "openrac/elf.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/memory.h"
#include "program/program.h"

#ifdef OPENRAC_HAS_DISC
#include "assets/disc/disc.h"
#include "assets/disc/wad.h"
#endif
#ifdef OPENRAC_FRONTEND
#include "scene/level.h"
#endif

namespace openrac::program {
namespace {

#ifdef OPENRAC_HAS_DISC
// Going Commando's programs (games/rac2/decomp/docs/build/levels.md): G/LEVELn.WAD, which holds level n but
// for the museum, level 30, in G/LEVEL21.WAD. Its word pair at +0x10 gives the level data's sector,
// which opens with the (offset, size) of the program. The image is opened once.
bool rac2_level_program(int level, std::vector<std::uint8_t>* bytes) {
    constexpr int kMuseum = 30;
    constexpr int kMuseumFile = 21;
    constexpr std::uint64_t kSector = 2048;
    static std::optional<assets::disc::IsoImage> iso;
    static bool tried = false;
    const fs::path image = g_options.data / "iso_data" / openrac_game.game / "disc.iso";

    try {
        if (!tried) {
            tried = true;
            iso.emplace(assets::disc::IsoImage::open(image));
        }

        if (!iso) {
            return false;
        }

        const auto word = [](const std::vector<std::uint8_t>& from, std::size_t at) {
            std::uint32_t value;
            std::memcpy(&value, from.data() + at, sizeof(value));
            return value;
        };
        const std::string name =
            std::format("G/LEVEL{}.WAD", level == kMuseum ? kMuseumFile : level);
        const assets::disc::IsoEntry* file = iso->find(name);

        if (file == nullptr) {
            return false;
        }

        const std::uint64_t start = file->lba * kSector;
        const std::vector<std::uint8_t> head = iso->read_bytes(start, 0x18);

        if (static_cast<int>(word(head, 0x08)) != level) {
            return false;
        }

        const std::uint64_t data = start + word(head, 0x10) * kSector;
        const std::vector<std::uint8_t> slot = iso->read_bytes(data, 8);
        *bytes = iso->read_bytes(data + word(slot, 0), word(slot, 4));
        return !bytes->empty();
    } catch (const std::exception& e) {
        log::warn("level {}'s program cannot be read from {}: {}", level, image.string(), e.what());
        return false;
    }
}
#endif

#ifdef OPENRAC_HAS_DISC
// Up Your Arsenal's programs (editor/wads.py, WadDisc and level_ranges, the editor's reader of the
// disc): its table of contents from sector 1001, global headers (each sized by its first word,
// the last padded with zeros) and then the level table, three (header sector, sector count) pairs
// a level. A level's header is 0x60 bytes: +4 the file's first sector, +8 the level's number,
// +0x10 the level data's sector in the file, which opens with the (offset, size) of the program,
// WAD-compressed. The functions every level carries are written with Veldin's (level 1's)
// addresses, so any level other than Veldin needs Veldin's program to pair them.
bool rac3_level_program(int level, std::vector<std::uint8_t>* bytes) {
    constexpr std::uint32_t kTocSector = 1001;
    constexpr std::uint64_t kSector = 2048;
    constexpr std::size_t kTocLimit = 0x200000;
    constexpr std::size_t kPair = 24;
    constexpr std::uint32_t kLevelHeader = 0x60;
    static std::optional<assets::disc::IsoImage> iso;
    static std::vector<std::uint8_t> toc;
    static bool tried = false;
    const fs::path image = g_options.data / "iso_data" / openrac_game.game / "disc.iso";

    try {
        if (!tried) {
            tried = true;
            iso.emplace(assets::disc::IsoImage::open(image));
            const std::uint64_t sectors = iso->sector_count();
            const std::uint64_t size = sectors > kTocSector ? (sectors - kTocSector) * kSector : 0;
            toc = iso->read_bytes(kTocSector * kSector, std::min<std::uint64_t>(kTocLimit, size));
        }
        if (!iso || toc.empty()) {
            return false;
        }

        const auto word = [](const std::vector<std::uint8_t>& from, std::size_t at) {
            std::int32_t value = 0;
            if (at + 4 <= from.size()) {
                std::memcpy(&value, from.data() + at, sizeof(value));
            }
            return value;
        };
        // The size of the header at sector `lba` of the table, 0 when it is not a header the
        // games use (wads.HEADER_KINDS).
        const auto header_size = [&](std::int32_t lba) -> std::uint32_t {
            if (lba <= static_cast<std::int32_t>(kTocSector)) {
                return 0;
            }
            const std::size_t at = (static_cast<std::size_t>(lba) - kTocSector) * kSector;
            const auto size = static_cast<std::uint32_t>(word(toc, at));
            switch (size) {
                case 0x60: case 0xC68: case 0x1018: case 0x1818: case 0x2A0: case 0x137C:
                case 0x26F0:
                    return at + 8 <= toc.size() ? size : 0;
                default:
                    return 0;
            }
        };

        // The level table: the first entry whose first two pairs point at headers and whose third
        // does too or is empty.
        std::size_t table = 0;
        for (;;) {
            if (table + kPair > toc.size()) {
                return false;
            }
            const std::int32_t first = word(toc, table), second = word(toc, table + 8),
                               third = word(toc, table + 16);
            if (header_size(first) != 0 && header_size(second) != 0
                && (third == 0 || header_size(third) != 0)) {
                break;
            }
            const std::int32_t size = word(toc, table);
            if (size == 0) {
                table += 4;
                continue;
            }
            if (size < 8 || size > 0x10000 || size % 4 != 0) {
                return false;
            }
            table += static_cast<std::size_t>(size);
        }

        for (std::size_t at = table; at + kPair <= toc.size() && at < table + 100 * kPair;
             at += kPair) {  // wads.MAX_LEVELS
            for (std::size_t pair = 0; pair < 3; ++pair) {
                const std::int32_t lba = word(toc, at + pair * 8);
                if (header_size(lba) != kLevelHeader) {
                    continue;
                }
                const std::size_t header = (static_cast<std::size_t>(lba) - kTocSector) * kSector;
                if (word(toc, header + 8) != level) {
                    continue;
                }
                const std::uint64_t data =
                    (static_cast<std::uint64_t>(word(toc, header + 4))
                     + static_cast<std::uint64_t>(word(toc, header + 0x10)))
                    * kSector;
                const std::vector<std::uint8_t> slot = iso->read_bytes(data, 8);
                const std::int32_t offset = word(slot, 0), size = word(slot, 4);
                if (size <= 0) {
                    return false;
                }
                std::vector<std::uint8_t> raw = iso->read_bytes(data + offset, size);
                *bytes = assets::disc::is_wad(raw) ? assets::disc::wad_decompress(raw)
                                                   : std::move(raw);
                return !bytes->empty();
            }
        }
    } catch (const std::exception& e) {
        log::warn("level {}'s program cannot be read from {}: {}", level, image.string(), e.what());
    }
    return false;
}
#endif

// A level's program straight from the player's image (<data>/iso_data/<game>/disc.iso, kept by the
// set-up), for the games whose disc layout is known. The image is opened once.
bool level_program_from_disc(int level, std::vector<std::uint8_t>* bytes) {
#ifdef OPENRAC_HAS_DISC
    if (std::string(openrac_game.game) == "rac2") {
        return rac2_level_program(level, bytes);
    }
    if (std::string(openrac_game.game) == "rac3") {
        return rac3_level_program(level, bytes);
    }

    static std::optional<assets::disc::Disc> disc;
    static bool tried = false;

    if (!tried) {
        tried = true;
        const fs::path image = g_options.data / "iso_data" / openrac_game.game / "disc.iso";

        if (std::string(openrac_game.game) == "rac1") {
            try {
                disc.emplace(assets::disc::Disc::open(image, assets::disc::rac1_layout()));
            } catch (const std::exception& e) {
                log::warn(
                    "the level programs cannot be read from {}: {}", image.string(), e.what()
                );
            }
        }
    }

    if (!disc) {
        return false;
    }

    try {
        assets::disc::LevelFiles files = disc->level(static_cast<std::uint32_t>(level));

        if (files.overlay) {
            *bytes = std::move(*files.overlay);
            return true;
        }
    } catch (const std::exception& e) {
        log::warn("level {}'s program cannot be read from the disc image: {}", level, e.what());
    }
#else
    (void)level;
    (void)bytes;
#endif
    return false;
}

}  // namespace

void load_level_program(int level) {
    crash::level_breadcrumb("level program load", level);
    char name[8];
    std::snprintf(name, sizeof(name), "%02d", level);
    const fs::path path =
        g_options.data / "decompiler_out" / openrac_game.game / "levels" / name / "overlay.bin";
    std::vector<std::uint8_t> bytes;

    if (!runtime::read_file(path, &bytes) && !level_program_from_disc(level, &bytes)) {
        crash::level_breadcrumb("level program missing", level);
        log::error(
            "level {}'s program is in neither {} nor the disc image: the functions levels share "
            "are not "
            "relocated, and the game will misbehave (the hero's heading ignores the camera)",
            level,
            path.string()
        );
        return;
    }

    log::verbose("level {}'s program: {} bytes", level, bytes.size());
    crash::level_breadcrumb("level program relocation", level);
    std::uint32_t entry = 0;

    for (std::size_t at = 0; at + 16 <= bytes.size();) {
        std::uint32_t head[4];
        std::memcpy(head, bytes.data() + at, sizeof(head));

        if (entry == 0) {
            entry = head[3];
        }

        if (head[3] != entry || at + 16 + head[1] > bytes.size()) {
            break;
        }

        openrac_guest_add_level_program(level, head[0], bytes.data() + at + 16, head[1]);
        at += 16 + head[1];
    }
    crash::level_breadcrumb("level program ready", level);
}

extern "C" void openrac_game_clear_level_programs(void) {
    openrac_guest_clear_level_programs();
}

#ifdef OPENRAC_FRONTEND
int check_levels(const fs::path& levels, const fs::path& mods) {
    const fs::path stamp = levels / std::format("verified-{}.json", scene::kManifestFormat);
    std::error_code ignored;
    if (mods.empty()) {
        fs::remove(stamp, ignored);
        fs::remove(levels / "verified.json", ignored);  // the stamp before formats were named
    }
    std::vector<int> ids;

    for (int i = 0; i < 19; ++i) {
        ids.push_back(i);
    }

    ids.push_back(98);
    ids.push_back(99);
    int bad = 0;
    int newer = 0;  // levels of a newer format than this game reads: the game is out of date

    for (const int id : ids) {
        const fs::path dir = levels / std::format("level_{:02d}", id);
        scene::LevelData level;
        std::string error;

        const fs::path edited =
            mods.empty() ? fs::path{} : mods / std::format("level_{:02d}", id) / "placements.json";
        if (!scene::load_level(dir, level, error, edited)) {
            log::error("level {:02d}: {}", id, error);
            ++bad;
            newer += error.find("newer than this game") != std::string::npos;
            continue;
        }

        // The flight world has only its ships' textures; every other one has ground to stand on.
        if (id != 98 && (level.vertices.empty() || level.models.empty())) {
            log::error(
                "level {:02d}: it has nothing to draw ({} models, {} vertices)",
                id,
                level.models.size(),
                level.vertices.size()
            );
            ++bad;
            continue;
        }

        if (level.missing_images > 0) {
            log::error("level {:02d}: {} of its textures are missing", id, level.missing_images);
            ++bad;
            continue;
        }

        log::info(
            "level {:02d}: {} models, {} vertices, {} textures",
            id,
            level.models.size(),
            level.vertices.size(),
            level.images.size()
        );
        if (level.overridden_models > 0 || level.overridden_images > 0) {
            log::info(
                "level {:02d}: loaded {} modded model(s) and {} modded texture(s)",
                id,
                level.overridden_models,
                level.overridden_images
            );
        }
    }

    if (newer > 0) {
        log::error(
            "{} of {} levels in {} are of a newer format than this game reads: build the game "
            "again (the "
            "launcher's \"Build the game\"; tools/build_port.py)",
            newer,
            ids.size(),
            levels.string()
        );
        return 1;
    }

    if (bad > 0) {
        if (mods.empty()) {
            log::error(
                "{} of {} levels in {} cannot be drawn: read them from the disc again (the "
                "launcher's "
                "\"Get the levels ready\"; editor/extract.py port --replace)",
                bad,
                ids.size(),
                levels.string()
            );
        } else {
            log::error(
                "{} of {} modded levels cannot be drawn from {}: repair or remove the named mod "
                "files",
                bad,
                ids.size(),
                mods.string()
            );
        }
        return 1;
    }

    if (std::FILE* f = std::fopen(stamp.string().c_str(), "wb")) {
        const std::string text = std::format(
            "{{\"format\": {}, \"levels\": {}, \"program\": \"{}\"}}\n",
            scene::kManifestFormat,
            ids.size(),
            program_name()
        );
        std::fwrite(text.data(), 1, text.size(), f);
        std::fclose(f);
    } else {
        log::error("cannot write {}", stamp.string());
        return 1;
    }

    if (mods.empty()) {
        log::info("all {} levels can be drawn: {}", ids.size(), stamp.string());
    } else {
        log::info(
            "all {} levels can be drawn with mod overrides from {}", ids.size(), mods.string()
        );
    }
    return 0;
}
#endif

}  // namespace openrac::program
