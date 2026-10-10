// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-<game>: one of the games, native. Every game's program is this
// file, linked with that game's translated C and its description
// (openrac_game, generated from port/game/<id>/hostgen.json).
//
//   openrac-<id> --data <install>/active/<game>/data [--cards DIR] [--frames N] [--keep-going]
//
// The data folder is what the extractor made from the player's disc
// (tools/extractor.py; the launcher's "Set up from your disc"). The program
// does what the console's loader and the game's start-up code did, then runs
// the game's own main, translated from its decompilation:
//
//   1. game memory, laid out as the console's (port/runtime);
//   2. the executable's data copied to its addresses, from the player's copy
//      of it (its code bytes come along and are never run);
//   3. the game's functions registered by code address, with the level
//      program that is loaded deciding which ones a level address means;
//   4. the game's main.
//
// While the port is being brought up, the program stops at the first function
// that has no C yet (still assembly in the decompilation, or a library not
// replaced yet) and says which; --keep-going logs it and carries on.

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "common/log.h"
#include "openrac/elf.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/memory.h"

#ifdef OPENRAC_FRONTEND
#include "frontend.h"
#include "renderer/texture.h"
#endif
#ifdef OPENRAC_MOVIES
#include "media/movie.h"
#include "platform/audio.h"
#endif

extern "C" {
const char* openrac_game_disc_image = nullptr;
const char* openrac_game_card_dir = nullptr;
int openrac_game_language = 1;
int openrac_game_no_card = 0;
}

namespace {

namespace fs = std::filesystem;
using namespace openrac;

struct Options {
    fs::path data;
    fs::path cards;
    long frames = -1;
    bool keep_going = false;
    bool window = false;
    fs::path levels;  // the extracted levels the window draws (level_00, ...)
};

Options g_options;
long g_frame = 0;
gaddr g_chain = 0;  // the display list sent this frame
bool g_vsync_since_kick = true;  // a vertical blank was waited for since the last frame was sent
bool g_window = false;
std::chrono::steady_clock::time_point g_next_frame;

std::string program_name() {
    return std::string("openrac-") + openrac_game.id;
}

[[noreturn]] void usage(const std::string& why) {
    log::error("{}", why);
    log::error(
        "usage: {} --data <install>/active/{}/data [--cards DIR] [--frames N] [--keep-going]",
        program_name(),
        openrac_game.game
    );
    std::exit(64);
}

// Where the first memory card lives unless --cards says otherwise: the folder
// the launcher's save manager reads and backs up (launcher/core/src/saves.rs,
// get_memcard_dir): <data home>/openrac/memcard/<serial>.
fs::path default_cards() {
#if defined(__APPLE__)
    const char* home = std::getenv("HOME");
    fs::path base = fs::path(home != nullptr ? home : ".") / "Library/Application Support/OpenRAC";
#else
    fs::path base;
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && xdg[0] != '\0') {
        base = fs::path(xdg) / "openrac";
    } else {
        const char* home = std::getenv("HOME");
        base = fs::path(home != nullptr ? home : ".") / ".local/share/openrac";
    }
#endif
    return base / "memcard" / openrac_game.serial;
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                usage("missing value for " + a);
            }
            return argv[++i];
        };
        if (a == "--data") {
            o.data = value();
        } else if (a == "--cards") {
            o.cards = value();
        } else if (a == "--no-card") {
            openrac_game_no_card = 1;
        } else if (a == "--window") {
            o.window = true;
        } else if (a == "--levels") {
            o.levels = value();
        } else if (a == "--frames") {
            o.frames = std::stol(value());
        } else if (a == "--keep-going") {
            o.keep_going = true;
        } else if (a == "--help" || a == "-h") {
            usage(program_name() + ": " + openrac_game.title + ", native");
        } else {
            usage("unknown option " + a);
        }
    }
    if (o.data.empty()) {
        if (const char* env = std::getenv("OPENRAC_DATA")) {
            o.data = env;
        } else {
            usage(
                std::string("no data folder: pass --data, the folder the extractor wrote for ")
                + openrac_game.game
            );
        }
    }
    if (o.cards.empty()) {
        o.cards = default_cards();
    }
    return o;
}

void finish() {
    openrac_guest_report_missing();
    log::info("{} frames", g_frame);
}

}  // namespace

// ---- What the library replacements call (openrac/game_host.h) ----

extern "C" {

int openrac_game_vsync(void) {
    g_vsync_since_kick = true;
    // With a window, the renderer draws the frame from the game's memory; without one, a frame is
    // only paced, at the game's frame rate.
#ifdef OPENRAC_FRONTEND
    if (g_window) {
        const auto* ram = runtime::Memory::get().base();
        if (!frontend::frame(std::span<const std::uint8_t>(ram, 32u * 1024 * 1024), g_chain)) {
            std::exit(0);
        }
    }
#endif
    g_frame++;
    if (g_options.frames >= 0 && g_frame >= g_options.frames) {
        std::exit(0);  // finish() runs at exit
    }
    using namespace std::chrono;
    const auto now = steady_clock::now();
    if (g_next_frame.time_since_epoch().count() == 0 || now > g_next_frame + milliseconds(100)) {
        g_next_frame = now;
    }
    g_next_frame +=
        microseconds(1000000 / (openrac_game.frame_rate > 0 ? openrac_game.frame_rate : 60));
    std::this_thread::sleep_until(g_next_frame);
    return static_cast<int>(g_frame & 1);
}

void openrac_game_dma_send(gaddr channel, gaddr tag) {
    g_chain = tag;
    // A frame sent to VIF1 with no vertical blank waited for since the last one: on the console
    // the loop is paced by the DMA and the interrupts (the title loop never calls sceGsSyncV), so
    // the frame ends here, with the vertical blank's handlers, as if the hardware had done it.
    if (channel == 0x10009000u) {
        if (!g_vsync_since_kick) {
            openrac_game_vsync();
            openrac_game_run_vsync_handlers();
        }
        g_vsync_since_kick = false;
    }
    log::debug("frame {}: DMA chain at {:#010x} to channel {:#010x}", g_frame, tag, channel);
}

void openrac_game_set_display(const openrac_game_display* d) {
    log::debug(
        "display: {}x{} at page {}, format {:#x}", d->width, d->height, d->frame_base, d->psm
    );
}

void openrac_game_set_video_mode(int interlace, int mode, int field_mode) {
    log::info("video mode {:#x} (interlace {}, field mode {})", mode, interlace, field_mode);
}

void openrac_game_load_image(const openrac_game_image* image) {
    log::debug(
        "texture upload {}x{} format {:#x} to block {}",
        image->width,
        image->height,
        image->psm,
        image->base
    );
#ifdef OPENRAC_FRONTEND
    if (g_window && image->width > 0 && image->height > 0) {
        const std::size_t bytes = renderer::image_bytes(
            static_cast<std::uint8_t>(image->psm), image->width, image->height
        );
        frontend::upload_image(
            static_cast<std::uint32_t>(image->base),
            static_cast<std::uint32_t>(image->width_units),
            static_cast<std::uint8_t>(image->psm),
            static_cast<std::uint32_t>(image->x),
            static_cast<std::uint32_t>(image->y),
            static_cast<std::uint32_t>(image->width),
            static_cast<std::uint32_t>(image->height),
            std::span<const std::uint8_t>(static_cast<const std::uint8_t*>(G(image->pixels)), bytes)
        );
    }
#endif
}

/*
 * The movie player, after ReRAC's movie mode (crates/rc-game/src/movie_player.rs,
 * crates/rc-engine/src/movie_render.rs; ISC License, Copyright (c) 2026 ReRAC contributors):
 * the audio is the clock. Each vertical blank hands the device one blank's worth of the movie's
 * 48 kHz audio, and the picture shown is the one whose time holds the sample being heard (what has
 * been handed over less what the device still holds), so picture and sound cannot drift apart.
 * When the video ends (or is skipped) the last picture stays up for the console's FadeToBlack(4).
 */
int openrac_game_play_movie(uint32_t lsn, uint32_t bytes, int channel, int start_skips) {
#if defined(OPENRAC_FRONTEND) && defined(OPENRAC_MOVIES)
    if (!g_window || openrac_game_disc_image == nullptr || bytes == 0) {
        return 0;
    }
    std::vector<std::uint8_t> file(bytes);
    {
        std::FILE* disc = std::fopen(openrac_game_disc_image, "rb");
        if (disc == nullptr) {
            return 0;
        }
        const bool read = fseeko(disc, static_cast<off_t>(lsn) * 2048, SEEK_SET) == 0
                          && std::fread(file.data(), 1, file.size(), disc) == file.size();
        std::fclose(disc);
        if (!read) {
            log::warn("movie at sector {}: the disc image is too short", lsn);
            return 0;
        }
    }
    std::optional<media::MoviePlayer> player;
    try {
        player.emplace(media::Movie::open(file, static_cast<std::uint8_t>(channel)));
    } catch (const std::exception& e) {
        log::warn("movie at sector {}: {}", lsn, e.what());
        return 0;
    }
    const media::VideoSequence& sequence = player->movie().sequence();
    log::info(
        "movie at sector {}: {}x{}, {}/{} fps, audio channel {}", lsn, sequence.width, sequence.height,
        sequence.fps_num, sequence.fps_den, player->movie().audio_channel() ? int(*player->movie().audio_channel()) : -1
    );
    std::string error;
    auto output = platform::AudioOutput::open(error);
    if (!output) {
        log::warn("movie: no sound ({})", error);
    }
    const int rate = openrac_game.frame_rate > 0 ? openrac_game.frame_rate : 60;
    const std::size_t per_blank = static_cast<std::size_t>(platform::kAudioRate / rate);
    std::vector<std::int16_t> samples(per_blank * 2);
    std::vector<std::uint8_t> rgba;
    std::shared_ptr<const media::VideoFrame> shown;
    std::uint16_t previous = 0xFFFF;
    int skipped = 0;
    auto pace = [&] {
        using namespace std::chrono;
        const auto now = steady_clock::now();
        if (g_next_frame.time_since_epoch().count() == 0 || now > g_next_frame + milliseconds(100)) {
            g_next_frame = now;
        }
        g_next_frame += microseconds(1000000 / rate);
        std::this_thread::sleep_until(g_next_frame);
        g_frame++;
    };
    for (std::uint64_t blank = 0;; ++blank) {
        if (output) {
            const std::size_t got = player->read_audio(samples);
            if (got > 0) {
                output->queue(std::span<const std::int16_t>(samples.data(), got * 2));
            }
        }
        const std::uint64_t handed = (blank + 1) * per_blank;
        const std::uint64_t held = output ? static_cast<std::uint64_t>(output->queued_frames()) : 0;
        const double heard = static_cast<double>(handed > held ? handed - held : 0) / platform::kAudioRate;
        auto frame = player->frame_at(heard);
        if (frame) {
            shown = frame;
        }
        if (player->video_finished()) {
            break;
        }
        if (shown) {
            media::frame_to_rgba(*shown, rgba);
            if (!frontend::show_picture(rgba.data(), int(shown->width), int(shown->height), 0.0f)) {
                std::exit(0);
            }
        }
        std::uint16_t buttons = 0xFFFF;
        std::uint8_t analog[4];
        frontend::pad(0, &buttons, analog);
        const bool start_now = (buttons & 0x0008) == 0;
        const bool start_before = (previous & 0x0008) == 0;
        previous = buttons;
        if (start_skips && start_now && !start_before) {
            skipped = 1;
            break;
        }
        pace();
    }
    // FadeToBlack(4) over the last picture, then its closing black blank.
    for (int step = 0; step <= 4 && shown; ++step) {
        frontend::show_picture(rgba.data(), int(shown->width), int(shown->height), step >= 4 ? 1.0f : float(step + 1) / 4.0f);
        pace();
    }
    if (output) {
        output->pause(true);
    }
    log::info("movie at sector {}: {}", lsn, skipped ? "skipped" : "played");
    return skipped;
#else
    (void)lsn;
    (void)bytes;
    (void)channel;
    (void)start_skips;
    return 0;
#endif
}

int openrac_game_pad(int port, uint16_t* buttons, uint8_t analog[4]) {
#ifdef OPENRAC_FRONTEND
    if (g_window && frontend::pad(port, buttons, analog)) {
        return 1;
    }
#endif
    (void)port;
    *buttons = 0xFFFF;  // nothing pressed (active low)
    std::memset(analog, 0x80, 4);
    return 0;
}

}  // extern "C"

int main(int argc, char** argv) {
    g_options = parse(argc, argv);
    const fs::path iso = g_options.data / "iso_data" / openrac_game.game;
    const fs::path exe = iso / openrac_game.serial;
    const fs::path disc = iso / "disc.iso";
    if (!fs::exists(exe) || !fs::exists(disc)) {
        log::fatalf(
            "{} has no {} and disc.iso: set the game up from your disc first "
            "(python3 tools/extractor.py <image> --game {} --proj-path {})",
            iso.string(),
            openrac_game.serial,
            openrac_game.game,
            g_options.data.string()
        );
    }
    log::set_file((g_options.data / (program_name() + ".log")).string().c_str());
    log::info("{}: {}", program_name(), openrac_game.title);

    runtime::Memory::create();
    runtime::install_crash_handler();

    std::vector<std::uint8_t> file;
    runtime::ElfImage image;
    std::string reason;
    if (!runtime::read_file(exe, &file) || !runtime::read_elf(file, &image, &reason)
        || !runtime::load_elf(file, image, &reason)) {
        log::fatalf("{}: {}", exe.string(), reason.empty() ? "cannot be read" : reason);
    }
    log::info(
        "{}: {} segments loaded, entry {:#x}",
        openrac_game.serial,
        image.segments.size(),
        image.entry
    );

    static std::string disc_path = disc.string();
    std::error_code made;
    fs::create_directories(g_options.cards, made);
    static std::string cards_path = g_options.cards.string();
    openrac_game_disc_image = disc_path.c_str();
    openrac_game_card_dir = cards_path.c_str();

#ifdef OPENRAC_FRONTEND
    if (g_options.window) {
        std::string why;
        fs::path levels = g_options.levels.empty() ? g_options.data / "port" : g_options.levels;
        if (!frontend::open(openrac_game.id, levels, why)) {
            log::fatalf("no window: {}", why);
        }
        g_window = true;
    }
#endif

    openrac_game_register_functions();
    openrac_guest_set_overlay_source(openrac_game_loaded_overlay);
    openrac_guest_stop_on_missing(g_options.keep_going ? 0 : 1);
    std::atexit(finish);

    // What the console's start-up code did before main: the stack and heap are
    // the runtime's, .bss was zeroed by the loader.
    if (!openrac_game_run()) {
        log::error(
            "{}'s main is not known yet: port/game/{}/hostgen.json has no \"entry\"",
            openrac_game.title,
            openrac_game.id
        );
        return 2;
    }
    return 0;
}
