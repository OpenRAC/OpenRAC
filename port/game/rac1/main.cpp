// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// openrac-rac1: Ratchet & Clank (PAL), native.
//
//   openrac-rac1 --data <install>/active/rac1/data [--cards DIR] [--frames N] [--keep-going]
//
// The data folder is what the extractor made from the player's disc
// (tools/extractor.py; the launcher's "Set up from your disc"). The program
// does what the console's loader and the game's start-up code did, then
// runs the game's own main(), translated from the decompilation:
//
//   1. game memory, laid out as the console's (port/runtime);
//   2. the executable's data copied to its addresses, from the player's copy
//      of SCES_509.16 (its code bytes come along and are never run);
//   3. the game's functions registered by code address, with the level
//      program that is loaded deciding which ones a level address means;
//   4. main() (func_0012DB18): the boot stage, then each level's loop.
//
// While the port is being brought up, the program stops at the first function
// that has no C yet (still assembly in the decompilation, or a library not
// replaced yet) and says which; --keep-going logs it and carries on.

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "common/log.h"
#include "host/rac1_host.h"
#include "openrac/elf.h"
#include "openrac/guest.h"
#include "openrac/memory.h"

extern "C" {
void openrac_game_register_functions(void);
void func_0012DB18(void);  // main

const char* openrac_rac1_disc_image = nullptr;
const char* openrac_rac1_card_dir = nullptr;
int openrac_rac1_language = 1;
}

namespace {

namespace fs = std::filesystem;
using namespace openrac;

constexpr const char* kSerial = "SCES_509.16";
constexpr int kLevels = 19;

struct Options {
    fs::path data;
    fs::path cards;
    long frames = -1;
    bool keep_going = false;
};

Options g_options;
long g_frame = 0;
std::chrono::steady_clock::time_point g_next_frame;

// Where the memory card lives unless --cards says otherwise: the folder the
// launcher's save manager reads and backs up (launcher/core/src/saves.rs,
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
    return base / "memcard" / kSerial;
}

[[noreturn]] void usage(const char* why) {
    log::error("{}", why);
    log::error("usage: openrac-rac1 --data <install>/active/rac1/data [--cards DIR] [--frames N] "
               "[--keep-going]");
    std::exit(64);
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                usage(("missing value for " + a).c_str());
            }
            return argv[++i];
        };
        if (a == "--data") {
            o.data = value();
        } else if (a == "--cards") {
            o.cards = value();
        } else if (a == "--frames") {
            o.frames = std::stol(value());
        } else if (a == "--keep-going") {
            o.keep_going = true;
        } else if (a == "--help" || a == "-h") {
            usage("openrac-rac1: Ratchet & Clank (PAL), native");
        } else {
            usage(("unknown option " + a).c_str());
        }
    }
    if (o.data.empty()) {
        if (const char* env = std::getenv("OPENRAC_DATA")) {
            o.data = env;
        } else {
            usage("no data folder: pass --data, the folder the extractor wrote for rac1");
        }
    }
    if (o.cards.empty()) {
        o.cards = default_cards();
    }
    return o;
}

// The level program loaded, for calls through a level code address.
int current_overlay() {
    const int level = openrac_rac1_loaded_level();
    return level >= 0 && level < kLevels ? level : OPENRAC_OVERLAY_EXE;
}

void finish() {
    openrac_guest_report_missing();
    log::info("{} frames", g_frame);
}

}  // namespace

// ---- What the library replacements call (host/rac1_host.h) ----

extern "C" {

int openrac_rac1_vsync(void) {
    // Until the window and renderer are connected (port/platform,
    // port/renderer), a frame is only paced, at PAL's 50 Hz.
    g_frame++;
    if (g_options.frames >= 0 && g_frame >= g_options.frames) {
        std::exit(0);  // finish() runs at exit
    }
    using namespace std::chrono;
    const auto now = steady_clock::now();
    if (g_next_frame.time_since_epoch().count() == 0 || now > g_next_frame + milliseconds(100)) {
        g_next_frame = now;
    }
    g_next_frame += microseconds(20000);
    std::this_thread::sleep_until(g_next_frame);
    return static_cast<int>(g_frame & 1);
}

void openrac_rac1_dma_send(gaddr channel, gaddr tag) {
    log::debug("frame {}: DMA chain at {:#010x} to channel {:#010x}", g_frame, tag, channel);
}

void openrac_rac1_set_display(const openrac_rac1_display* d) {
    log::debug(
        "display: {}x{} at page {}, format {:#x}", d->width, d->height, d->frame_base, d->psm
    );
}

void openrac_rac1_set_video_mode(int interlace, int mode, int field_mode) {
    log::info("video mode {:#x} (interlace {}, field mode {})", mode, interlace, field_mode);
}

void openrac_rac1_load_image(const openrac_rac1_image* image) {
    log::debug(
        "texture upload {}x{} format {:#x} to block {}",
        image->width,
        image->height,
        image->psm,
        image->base
    );
}

int openrac_rac1_pad(int port, uint16_t* buttons, uint8_t analog[4]) {
    (void)port;
    *buttons = 0xFFFF;  // nothing pressed (active low)
    std::memset(analog, 0x80, 4);
    return 0;
}

}  // extern "C"

int main(int argc, char** argv) {
    g_options = parse(argc, argv);
    const fs::path iso = g_options.data / "iso_data" / "rac1";
    const fs::path exe = iso / kSerial;
    const fs::path disc = iso / "disc.iso";
    if (!fs::exists(exe) || !fs::exists(disc)) {
        log::fatalf(
            "{} has no {} and disc.iso: set the game up from your disc first "
            "(python3 tools/extractor.py <image> --game rac1 --proj-path {})",
            iso.string(),
            kSerial,
            g_options.data.string()
        );
    }
    log::set_file((g_options.data / "openrac-rac1.log").string().c_str());

    runtime::Memory::create();
    runtime::install_crash_handler();

    std::vector<std::uint8_t> file;
    runtime::ElfImage image;
    std::string reason;
    if (!runtime::read_file(exe, &file) || !runtime::read_elf(file, &image, &reason)
        || !runtime::load_elf(file, image, &reason)) {
        log::fatalf("{}: {}", exe.string(), reason.empty() ? "cannot be read" : reason);
    }
    log::info("{}: {} segments loaded, entry {:#x}", kSerial, image.segments.size(), image.entry);

    static std::string disc_path = disc.string();
    std::error_code made;
    fs::create_directories(g_options.cards, made);
    static std::string cards_path = g_options.cards.string();
    openrac_rac1_disc_image = disc_path.c_str();
    openrac_rac1_card_dir = cards_path.c_str();

    openrac_game_register_functions();
    openrac_guest_set_overlay_source(current_overlay);
    openrac_guest_stop_on_missing(g_options.keep_going ? 0 : 1);
    std::atexit(finish);

    // What _start did before main: the stack and heap are the runtime's, .bss
    // was zeroed by the loader.
    func_0012DB18();
    return 0;
}
