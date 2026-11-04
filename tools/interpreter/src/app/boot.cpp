// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

/**
 * openrac-boot: runs the program on your own disc image and reports how far
 * it gets. A development tool: it shows what the machine model still lacks.
 *
 * It starts a `Machine` on a disc image, runs it field by field, and either shows the pictures
 * and plays the sound in a window or runs headless. Options script the pad, write the last
 * picture or all the sound to a file, record a frame's display list for `openrac-vubench`,
 * list what one frame is drawn with, and set the memory card directory. At the end it prints
 * what the model did not understand. It leaves out any menu or settings of its own.
 *
 * Sources: the command line is its own; the machine's sources are listed in `sys/machine.h`.
 */

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <iterator>
#include <sstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "ps2/vu_dis.h"
#include "sys/machine.h"
#include "sys/snapshot.h"

#include <zlib.h>

#ifndef OPENRAC_NO_WINDOW
#include "host/window.h"
#endif

using namespace ps2;

namespace {

/**
 * Prints how much each of a game's vector unit programs ran on one unit.
 *
 * @param unit The unit's name for the heading.
 * @param chunks The chunks of the game's programs, from its executable.
 * @param uses What the unit ran.
 */
void report_vu_programs(
    const char* unit, const std::vector<VuChunk>& chunks, const std::vector<Vu::Use>& uses
) {
    std::vector<VuProgramUse> programs = vu_program_use(chunks, uses);
    u64 all = 0;

    for (const VuProgramUse& program : programs) {
        all += program.pairs;
    }

    std::fprintf(stderr, "%s programs (program, runs started, instructions, share):\n", unit);

    for (const VuProgramUse& program : programs) {
        double share =
            all ? 100.0 * static_cast<double>(program.pairs) / static_cast<double>(all) : 0.0;

        std::fprintf(
            stderr,
            "  %8u  %10llu  %14llu  %5.1f%%%s\n",
            program.program,
            static_cast<unsigned long long>(program.starts),
            static_cast<unsigned long long>(program.pairs),
            share,
            program.program == 0 ? "  (not in the program's table)" : ""
        );
    }
}

/**
 * Writes a picture as a binary PPM file.
 *
 * @param path Host path of the file to make.
 * @param image The picture; each pixel has red in the low byte, then green and blue.
 * @return False if the file cannot be made.
 */
bool write_ppm(const std::string& path, const Image& image) {
    std::FILE* f = std::fopen(path.c_str(), "wb");

    // The host cannot make the file.
    if (!f) {
        return false;
    }

    // PPM header: format P6 (binary RGB), width, height, and 255 as the largest value of a channel.
    std::fprintf(f, "P6\n%d %d\n255\n", image.width, image.height);

    // One pixel at a time: three bytes, dropping the unused top byte.
    for (u32 p : image.pixels) {
        u8 rgb[3] = {static_cast<u8>(p), static_cast<u8>(p >> 8), static_cast<u8>(p >> 16)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    return true;
}

}  // namespace

/**
 * Runs the program on a disc image.
 *
 * @param argc Number of command line arguments.
 * @param argv The program name, the disc image, and the options listed in the usage message.
 * @return 0 when the run ended, 1 when the disc, the program or the window cannot be opened, 2
 *     for a bad command line.
 */
/**
 * Main memory as one number (FNV-1a over words), leaving out the swapped code (below 0x100000) and
 * the stack (from 0x1F00000).
 */
u64 memory_hash(sys::Machine& machine) {
    u64 hash = 0xCBF29CE484222325ull;
    const u32* words = reinterpret_cast<const u32*>(machine.memory.ram(0x100000));

    for (std::size_t n = 0; n < (0x1F00000 - 0x100000) / 4; n++) {
        hash = (hash ^ words[n]) * 0x100000001B3ull;
    }

    return hash;
}

int main(int argc, char** argv) {
    // Settings from the command line. A frame number of -1 means the option was not given.
    std::string iso, hooks, ppm, vif_file, card, wav, native, native_calls;
    bool card_wanted = true;
    int vif_frame = -1;
    int frames = -1, report = 60, states_frame = -1, ppm_from = -1;

    /*
     * Threads that draw: by default most of the fast cores, leaving one for the program itself.
     * At least 2 cores are assumed, and no more than 12 threads are used.
     */
    int gs_threads =
        static_cast<int>(std::min(12u, std::max(2u, std::thread::hardware_concurrency()) - 1));
    bool window_wanted = false, drawing_thread = true, vu_programs = false;

    /** Scripted input: hold these buttons from one frame for some frames. */
    struct Press {
        /** The first field of the press and how many fields it lasts. */
        int frame, length;

        /** The buttons held, as the pad's bit mask. */
        unsigned buttons;
    };

    std::vector<Press> presses;

    /** Scripted stick: hold it at X, Y (0x80 centred, 0 left/up, 0xFF right/down) for some fields. */
    struct Stick {
        /** The first field and how many fields it lasts. */
        int frame, length;

        /** The axes. */
        unsigned x, y;

        /** The right stick (--rstick) rather than the left. */
        bool right = false;
    };

    std::vector<Stick> sticks;

    /**
     * A scripted change of the program's memory: a word kept at a value for some fields. For
     * reaching a state that no script of presses reaches, such as starting in a later level.
     */
    struct Write {
        /** The first field of the change and how many fields it lasts. */
        int frame, length;

        /** The guest address of the 32-bit word and the value written at each field's start. */
        unsigned address, value;
    };

    std::vector<Write> writes;

    /**
     * For checking functions by what they do (`openrac-fcheck`): the machine's state is written at
     * the first calls of each listed function. The list has a line per function: its address,
     * how many of its first bytes to compare and their CRC-32 (so that a level's function is
     * taken only where that level's code is loaded), its name, and how many calls to take
     * (3 when left out; 0 only counts the calls, for `--calls`).
     */
    struct Capture {
        unsigned address, bytes, crc, wanted, taken;
        std::string name;

        /** Calls met (after the CRC check), and their first and last fields. */
        unsigned long long calls;
        long long first, last;

        /** The phase (`--capture-phases`) the last call was in, its calls so far and the states taken in it. */
        int phase;
        unsigned long long seen;
        unsigned phase_taken;

        /** With `--capture-key`: the calls seen and the states taken for each value of the key. */
        std::map<unsigned long long, std::pair<unsigned long long, unsigned>> keyed;
    };

    std::vector<Capture> captures;
    std::string capture_dir, calls_file;

    /*
     * Which calls become states. The run is cut into phases at the listed fields (by default one
     * phase from field 0): up to `wanted` calls are taken in each phase, so that the states of a
     * function called all the time come from each part of the run (the title, a menu page, the
     * load) and not only its first frames. With `--capture-spread` the calls taken in a phase
     * are those numbered 1, 2, 4, 8, ... in it rather than the first ones. Calls before the first
     * phase are counted but not taken.
     */
    std::vector<long long> phases{0};
    bool capture_spread = false;

    /*
     * The words of `--capture-key`: with any, the calls taken are per value of those words, not
     * per phase, so a function called every frame gets states from each hero state, weapon or
     * game state the run goes through and not just the first ones met. `capture_limit` is the
     * most states of one function in the run (0: no limit).
     */
    std::vector<unsigned> capture_keys;
    unsigned capture_limit = 0;

    /*
     * `--trace FILE ADDRESS[:WORDS],...`: a line per field with those words of main memory in
     * hexadecimal, to compare a move against the port's run field by field.
     */
    struct Traced {
        unsigned address, words;
    };

    /** `--swap`: a function whose calls go to code loaded elsewhere. */
    struct Swap {
        u32 entry = 0, first_word = 0, target = 0;
        std::vector<std::pair<u32, std::vector<char>>> code;
        u64 calls = 0;
    };

    std::vector<Swap> swaps;
    std::set<long long> dump_fields;
    std::FILE* digest_file = nullptr;
    std::FILE* returns_file = nullptr;
    u32 returns_entry = 0, returns_first_word = 0;
    std::vector<std::pair<u32, u32>> returns_pending;  // Return address and stack, innermost last.
    std::vector<u8> cover_map;
    u32 cover_base = 0;
    std::string cover_path;
    std::vector<Traced> traced;
    std::FILE* trace_file = nullptr;
    sys::Machine machine;

    /*
     * `--watch-stores ADDRESS:BYTES` (hexadecimal): a line on stderr for each store of the
     * program into those bytes of main memory, with the field, the store's instruction, the
     * return address register and the value, to find what writes a word.
     */
    unsigned watch_first = 0, watch_bytes = 0;

    // One option per pass; a value-taking option reads the next argument as well.
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--capture" && i + 2 < argc) {
            // A list of functions and the directory their call states go to.
            std::ifstream list(argv[++i]);
            capture_dir = argv[++i];
            std::string line;

            while (std::getline(list, line)) {
                Capture c{};
                char name[128] = "";

                // Lines that do not parse (comments, blank lines) are skipped.
                int got = std::sscanf(line.c_str(), "%x %u %x %127s %u", &c.address, &c.bytes, &c.crc, name, &c.wanted);

                if (got >= 4) {
                    c.name = name;
                    c.wanted = got == 5 ? c.wanted : 3;
                    c.first = c.last = -1;
                    c.phase = -1;
                    captures.push_back(c);
                }
            }
        } else if (arg == "--capture-phases" && i + 1 < argc) {
            // FIELD[,FIELD...]: where each phase of the capture starts, in increasing order.
            phases.clear();
            for (const char* s = argv[++i]; *s;) {
                char* end = nullptr;
                phases.push_back(std::strtoll(s, &end, 10));
                s = *end == ',' ? end + 1 : end + std::strlen(end);
            }
            std::sort(phases.begin(), phases.end());
        } else if (arg == "--capture-spread") {
            // Take calls 1, 2, 4, 8, ... rather than the first ones.
            capture_spread = true;
        } else if (arg == "--capture-key" && i + 1 < argc) {
            // ADDRESS[,ADDRESS...] in hexadecimal: take states per value of these words.
            for (const char* s = argv[++i]; *s;) {
                char* end = nullptr;
                capture_keys.push_back(static_cast<unsigned>(std::strtoul(s, &end, 16)));
                s = *end == ',' ? end + 1 : end + std::strlen(end);
            }
        } else if (arg == "--capture-limit" && i + 1 < argc) {
            // The most states of one function in this run.
            capture_limit = static_cast<unsigned>(std::atoi(argv[++i]));
        } else if (arg == "--dump-ram" && i + 1 < argc) {
            // FIELD[,FIELD...]: main memory into ram_FIELD.bin in the working directory.
            std::stringstream parts(argv[++i]);
            std::string part;

            while (std::getline(parts, part, ',')) {
                dump_fields.insert(std::stoll(part));
            }
        } else if (arg == "--digest" && i + 1 < argc) {
            // FILE: a line per field with a hash of main memory, leaving out the swapped code
            // (0xA0000 to 0x100000) and the stack (from 0x1F00000). Two runs part at the first line
            // that differs.
            digest_file = std::fopen(argv[++i], "w");
        } else if (arg == "--cover" && i + 2 < argc) {
            // ADDRESS:BYTES (hexadecimal) and FILE: which instructions of that code the run ran, a
            // digit each, written at the end.
            std::string range = argv[++i];
            std::size_t colon = range.find(':');

            cover_base = std::stoul(range.substr(0, colon), nullptr, 16);
            cover_map.assign(std::stoul(range.substr(colon + 1), nullptr, 16) / 4, 0);
            cover_path = argv[++i];
        } else if (arg == "--returns" && i + 2 < argc) {
            // ENTRY:FIRSTWORD (hexadecimal) and FILE: a line at each return of that function: the
            // field, the return address, the result registers, the registers a function keeps and
            // the memory hash `--digest` uses. Two runs of one script compare call by call.
            std::string spec = argv[++i];
            std::size_t colon = spec.find(':');

            returns_entry = std::stoul(spec.substr(0, colon), nullptr, 16);
            returns_first_word = std::stoul(spec.substr(colon + 1), nullptr, 16);
            returns_file = std::fopen(argv[++i], "w");
        } else if (arg == "--swap" && i + 1 < argc) {
            // ENTRY:FIRSTWORD:TARGET:FILE[:ADDRESS:FILE...] (hexadecimal addresses): a call of
            // ENTRY while FIRSTWORD is there goes to TARGET, with each FILE loaded at its address
            // (the first at TARGET). For a whole-run comparison of C with the function it replaces.
            std::stringstream parts(argv[++i]);
            std::string part;
            std::vector<std::string> fields;

            while (std::getline(parts, part, ':')) {
                fields.push_back(part);
            }

            if (fields.size() < 4 || fields.size() % 2 != 0) {
                std::fprintf(stderr, "--swap wants ENTRY:FIRSTWORD:TARGET:FILE[:ADDRESS:FILE...]\n");
                return 2;
            }

            Swap swap;
            swap.entry = std::stoul(fields[0], nullptr, 16);
            swap.first_word = std::stoul(fields[1], nullptr, 16);
            swap.target = std::stoul(fields[2], nullptr, 16);

            for (std::size_t n = 2; n + 1 < fields.size(); n += 2) {
                std::ifstream file(fields[n + 1], std::ios::binary);

                if (!file) {
                    std::fprintf(stderr, "cannot read %s\n", fields[n + 1].c_str());
                    return 2;
                }

                swap.code.push_back({static_cast<u32>(std::stoul(fields[n], nullptr, 16)),
                                     std::vector<char>(std::istreambuf_iterator<char>(file), {})});
            }

            swaps.push_back(std::move(swap));
        } else if (arg == "--trace" && i + 2 < argc) {
            // FILE and ADDRESS[:WORDS],... in hexadecimal, words decimal.
            trace_file = std::fopen(argv[++i], "w");
            for (const char* s = argv[++i]; *s;) {
                char* end = nullptr;
                Traced t{static_cast<unsigned>(std::strtoul(s, &end, 16)), 1};
                s = end;
                if (*s == ':') {
                    t.words = static_cast<unsigned>(std::strtoul(s + 1, &end, 10));
                    s = end;
                }
                traced.push_back(t);
                s += *s == ',' ? 1 : 0;
            }
        } else if (arg == "--watch-stores" && i + 1 < argc) {
            // ADDRESS:BYTES in hexadecimal.
            char* end = nullptr;
            watch_first = static_cast<unsigned>(std::strtoul(argv[++i], &end, 16));
            watch_bytes = *end == ':' ? static_cast<unsigned>(std::strtoul(end + 1, nullptr, 16)) : 4;
        } else if (arg == "--calls" && i + 1 < argc) {
            // At the end, write each listed function's call count and first and last field.
            calls_file = argv[++i];
        } else if (arg == "--hooks" && i + 1 < argc) {
            // The table that says which addresses of the program are which library functions.
            hooks = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            // Stop after this many fields.
            frames = std::atoi(argv[++i]);
        } else if (arg == "--report" && i + 1 < argc) {
            // Print counts and the speed every this many fields; 0 turns it off.
            report = std::atoi(argv[++i]);
        } else if (arg == "--verbose" && i + 1 < argc) {
            // The machine's log level (see `Machine::log`).
            machine.verbose = std::atoi(argv[++i]);
        } else if (arg == "--ppm" && i + 1 < argc) {
            // Where to write the last picture.
            ppm = argv[++i];
        } else if (arg == "--press" && i + 1 < argc) {
            // FRAME:BUTTONS[:FRAMES], buttons as a hexadecimal mask (cross is 4000, start 8)
            // The length defaults to 4 fields.
            Press p{0, 4, 0};
            std::sscanf(argv[++i], "%d:%x:%d", &p.frame, &p.buttons, &p.length);
            presses.push_back(p);
        } else if (arg == "--stick" && i + 1 < argc) {
            // FRAME:X:Y[:FRAMES], the left stick's axes in hexadecimal (80 centred); 4 fields by default.
            Stick s{0, 4, 0x80, 0x80};
            std::sscanf(argv[++i], "%d:%x:%x:%d", &s.frame, &s.x, &s.y, &s.length);
            sticks.push_back(s);
        } else if (arg == "--rstick" && i + 1 < argc) {
            // FRAME:X:Y[:FRAMES], the right stick (the camera's) as --stick has the left.
            Stick s{0, 4, 0x80, 0x80, true};
            std::sscanf(argv[++i], "%d:%x:%x:%d", &s.frame, &s.x, &s.y, &s.length);
            sticks.push_back(s);
        } else if (arg == "--ppm-from" && i + 1 < argc) {
            // FIELD: with --ppm FILE, also every field's picture from FIELD on, as FILE_<field>.ppm.
            ppm_from = std::atoi(argv[++i]);
        } else if (arg == "--write" && i + 1 < argc) {
            // FRAME:ADDRESS:VALUE[:FRAMES], address and value in hexadecimal; one field by default.
            Write w{0, 1, 0, 0};
            std::sscanf(argv[++i], "%d:%x:%x:%d", &w.frame, &w.address, &w.value, &w.length);
            writes.push_back(w);
        } else if (arg == "--gs-states" && i + 1 < argc) {
            // The field whose drawing states are listed.
            states_frame = std::atoi(argv[++i]);
        } else if (arg == "--gs-threads" && i + 1 < argc) {
            // How many threads the GS draws with (see `Gs::set_threads`).
            gs_threads = std::atoi(argv[++i]);
        } else if (arg == "--dump-vif" && i + 2 < argc) {
            /*
             * For openrac-vubench: one frame's display list as VIF1 gets it, with
             * VU1's memories as they were before it. The file is game data: it is
             * for your own machine.
             */
            vif_frame = std::atoi(argv[++i]);
            vif_file = argv[++i];
        } else if (arg == "--wav" && i + 1 < argc) {
            // Everything heard, as a sound file.
            wav = argv[++i];
        } else if (arg == "--native" && i + 1 < argc) {
            // A library of host code built from the game's decompiled C (runtime/port).
            native = argv[++i];
        } else if (arg == "--native-range" && i + 1 < argc) {
            // Use only the library's functions FIRST up to LAST of its table: FIRST:LAST.
            const char* range = argv[++i];
            const char* colon = std::strchr(range, ':');

            machine.native.first = static_cast<std::size_t>(std::strtoull(range, nullptr, 10));

            // Without a colon the range is open at the top.
            if (colon) {
                machine.native.last =
                    static_cast<std::size_t>(std::strtoull(colon + 1, nullptr, 10));
            }
        } else if (arg == "--native-name" && i + 1 < argc) {
            // Print the name of the function at a place of the library's table.
            machine.native.name_at = std::atol(argv[++i]);
        } else if (arg == "--native-calls" && i + 1 < argc) {
            // Count calls by level and address and write them to this file at the end.
            native_calls = argv[++i];
            machine.native.count_calls = true;
        } else if (arg == "--vu-programs") {
            // At the end, list how much each of the game's vector unit programs ran.
            vu_programs = true;
        } else if (arg == "--native-skip" && i + 1 < argc) {
            // A file of function names, one a line, that the interpreter keeps running.
            std::ifstream names(argv[++i]);
            std::string name;

            while (std::getline(names, name)) {
                machine.native.skip.insert(name);
            }
        } else if (arg == "--native-check" && i + 1 < argc) {
            // Compare this many of each host function's first calls with the retail code.
            machine.native.check = static_cast<unsigned>(std::atoi(argv[++i]));
        } else if (arg == "--card" && i + 1 < argc) {
            // The directory that is the memory card.
            card = argv[++i];
        } else if (arg == "--no-card") {
            // The slot stays empty.
            card_wanted = false;
        } else if (arg == "--one-thread") {
            // The drawing path on the program's own thread.
            drawing_thread = false;
        } else if (arg == "--ntsc") {
            // 60 Hz display: 59.94 fields a second.
            machine.hz = 59.94;
        } else if (arg == "--window") {
            // Show the pictures in a window, play the sound and read the keyboard.
            window_wanted = true;
        } else if (arg[0] != '-' && iso.empty()) {
            // The first argument that is not an option is the disc image.
            iso = arg;
        } else {
            // Anything else is a mistake: say what the options are.
            std::fprintf(
                stderr,
                "usage: openrac-boot DISC.iso [--hooks FILE] [--frames N] [--report N] [--verbose "
                "N] "
                "[--ntsc] [--window] [--ppm FILE] [--ppm-from FIELD] [--press "
                "FRAME:BUTTONS[:FRAMES]] [--rstick FRAME:X:Y[:FRAMES]] [--stick "
                "FRAME:X:Y[:FRAMES]] [--gs-states "
                "FRAME] "
                "[--gs-threads N] [--one-thread] [--dump-vif FRAME FILE] [--card DIRECTORY | "
                "--no-card] [--wav FILE] [--native LIBRARY] [--native-check CALLS] [--native-range "
                "FIRST:LAST] [--native-skip FILE] [--native-calls FILE] [--write "
                "FRAME:ADDRESS:VALUE[:FRAMES]] [--vu-programs] [--capture LIST DIRECTORY] "
                "[--capture-phases FIELD,...] [--capture-spread] [--capture-key ADDRESS,...] "
                "[--capture-limit N] [--trace FILE ADDRESS[:WORDS],...] [--calls FILE] "
                "[--watch-stores ADDRESS:BYTES]\n"
            );
            return 2;
        }
    }

    // No frame count given: a window runs until it is closed, a headless run for 600 fields.
    if (frames < 0) {
        frames = window_wanted ? INT_MAX : 600;
    }

    // No disc image was named, or it cannot be opened.
    if (iso.empty() || !machine.disc.open(iso)) {
        std::fprintf(stderr, "cannot open the disc image %s\n", iso.c_str());
        return 1;
    }

    // A negative thread count counts as none: the caller draws.
    machine.graphics.gs.set_threads(static_cast<unsigned>(std::max(gs_threads, 0)));

    // Without `--one-thread` the drawing path gets its own thread before the program starts.
    if (drawing_thread) {
        machine.drawing.start();
    }

    std::string error;

    // The disc has no bootable program, or the hooks table is bad.
    if (!machine.boot(&error) || (!hooks.empty() && !machine.load_hooks(hooks, &error))) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    // Host code in place of guest functions was asked for and cannot be loaded.
    if (!native.empty() && !machine.native.load(native, &error)) {
        std::fprintf(stderr, "%s: %s\n", native.c_str(), error.c_str());
        return 1;
    }

    // Stores into the watched bytes are logged with the field they happen in.
    if (watch_bytes) {
        machine.ee.watch_first = watch_first & 0x1FFFFFFFu;
        machine.ee.watch_end = machine.ee.watch_first + watch_bytes;
        // OPENRAC_BOOT_WATCH_STACK=WORDS: also the words of the stack above sp that look like
        // return addresses (in 0x100000..0x600000, word aligned), nearest first: the callers that
        // `ra` alone does not show.
        const char* stack_env = std::getenv("OPENRAC_BOOT_WATCH_STACK");
        const unsigned stack_words = stack_env ? static_cast<unsigned>(std::atoi(stack_env)) : 0;
        machine.ee.on_watched_store = [&machine, stack_words](u32 address, u64 value, unsigned bytes) {
            std::fprintf(
                stderr, "store field %llu pc %08x ra %08x address %08x bytes %u value %llx",
                static_cast<unsigned long long>(machine.frames), machine.ee.instruction,
                static_cast<u32>(machine.ee.gpr[31].lo), address, bytes,
                static_cast<unsigned long long>(value)
            );
            const u32 sp = machine.ee.sp32() & 0x1FFFFFFFu;
            if (stack_words) {
                std::fprintf(stderr, " sp %08x", sp);
            }
            for (unsigned k = 0; k < stack_words; k++) {
                const u32 at = sp + 4 * k;
                if (at + 4 > 0x2000000u) {
                    break;
                }
                u32 word;
                std::memcpy(&word, machine.memory.ram(at), 4);
                if (word >= 0x100000u && word < 0x600000u && (word & 3) == 0) {
                    std::fprintf(stderr, " %x:%08x", 4 * k, word);
                }
            }
            std::fputc('\n', stderr);
        };
    }

    std::vector<u8> capture_marks;

    // Call states were asked for: mark the functions' first instructions for the interpreter.
    if (!captures.empty()) {
        // Host code uses the same marks; the two do not go together.
        if (!native.empty()) {
            std::fprintf(stderr, "--capture and --native do not go together\n");
            return 2;
        }

        std::filesystem::create_directories(capture_dir);
        capture_marks.assign(GuestMemory::kRamBytes / 4, 0);

        for (const Capture& c : captures) {
            capture_marks[(c.address & (GuestMemory::kRamBytes - 1)) >> 2] = 1;
        }

        machine.ee.native_marks = capture_marks.data();

        // Reached a marked word: take the state if it is one of the listed functions' first calls.
        machine.ee.on_native = [&](u32 address) {
            for (Capture& c : captures) {
                bool here = (c.address & (GuestMemory::kRamBytes - 1)) == (address & (GuestMemory::kRamBytes - 1));

                if (!here) {
                    continue;
                }

                // Another program's code is at this address now (a level's).
                if (crc32(0, machine.memory.ram(c.address), c.bytes) != c.crc) {
                    continue;
                }

                long long field = static_cast<long long>(machine.frames);
                c.calls++;
                c.first = c.first < 0 ? field : c.first;
                c.last = field;

                // The phase of this call: a new one starts its counts afresh.
                int phase = static_cast<int>(std::upper_bound(phases.begin(), phases.end(), field) - phases.begin()) - 1;
                if (phase != c.phase) {
                    c.phase = phase;
                    c.seen = 0;
                    c.phase_taken = 0;
                }

                // Before the first phase, or the run's limit reached.
                if (phase < 0 || (capture_limit && c.taken >= capture_limit)) {
                    continue;
                }

                // Per value of the key words, or per phase: enough taken there.
                unsigned long long key = 0;
                for (unsigned word : capture_keys) {
                    u32 value;
                    std::memcpy(&value, machine.memory.ram(word), 4);
                    key = key * 0x100000001B3ULL + value;
                }
                auto& keyed = c.keyed[key];
                unsigned long long n;
                if (!capture_keys.empty()) {
                    if (keyed.second >= c.wanted) {
                        continue;
                    }
                    n = ++keyed.first;
                } else {
                    if (c.phase_taken >= c.wanted) {
                        continue;
                    }
                    n = ++c.seen;
                }

                // Spread: only calls whose number is a power of two.
                if (capture_spread && (n & (n - 1)) != 0) {
                    continue;
                }

                sys::CallState state;
                state.take(machine.ee, machine.vu0, machine.memory, machine.vif0.micro.data(), machine.vif0.data.data());
                state.address = address;
                std::string path = capture_dir + "/" + c.name + "." + std::to_string(c.taken) + ".snap";

                // The file is counted only when it was written.
                if (state.save(path)) {
                    c.taken++;
                    c.phase_taken++;
                    keyed.second++;

                    // Which field and key each state is of, to tell later what the game was doing.
                    if (std::FILE* index = std::fopen((capture_dir + "/states.tsv").c_str(), "a")) {
                        std::fprintf(
                            index, "%s\t%u\t%lld\t%llx", c.name.c_str(), c.taken - 1, field, key
                        );
                        for (unsigned word : capture_keys) {
                            u32 value;
                            std::memcpy(&value, machine.memory.ram(word), 4);
                            std::fprintf(index, "\t%x", value);
                        }
                        std::fputc('\n', index);
                        std::fclose(index);
                    }
                }
            }

            return false;
        };
    }

    // The memory card: a directory, by default one per disc among the user's
    // own files.
    if (card_wanted) {
        // No directory was named: use the one under the user's data folder.
        if (card.empty()) {
            std::filesystem::path home = std::getenv("HOME") ? std::getenv("HOME") : ".";
#if defined(__APPLE__)
            std::filesystem::path data = home / "Library" / "Application Support" / "OpenRAC";
#else
            std::filesystem::path data =
                std::getenv("XDG_DATA_HOME")
                    ? std::filesystem::path(std::getenv("XDG_DATA_HOME")) / "openrac"
                    : home / ".local" / "share" / "openrac";
#endif
            card = (data / "memcard" / machine.program_name).string();
        }

        std::error_code problem;
        std::filesystem::create_directories(card, problem);

        // The directory cannot be made: the program runs without a card.
        if (problem) {
            std::fprintf(
                stderr,
                "no memory card: cannot make %s (%s)\n",
                card.c_str(),
                problem.message().c_str()
            );
        } else {
            machine.card.directory = card;
            std::fprintf(stderr, "memory card: %s\n", card.c_str());
        }
    }

#ifndef OPENRAC_NO_WINDOW
    host::Window window;

    // A 960 by 720 window (4:3); the program ends if SDL cannot make it.
    if (window_wanted && !window.open("OpenRAC", 960, 720)) {
        return 1;
    }
#else
    // A build without SDL has no window; the flag is read only to say it is not used.
    (void)window_wanted;
#endif

    std::vector<s16> heard;

    // Called by the machine at each vertical blank, on this thread, with that field's sound.
    machine.on_sound = [&](const s16* samples, std::size_t count) {
        // Keep the samples (left and right, so count * 2) only if a sound file was asked for.
        if (!wav.empty()) {
            heard.insert(heard.end(), samples, samples + count * 2);
        }
#ifndef OPENRAC_NO_WINDOW
        // In a window the sound is played as well.
        if (window_wanted) {
            window.play(samples, count, sys::Sound::kRate);
        }
#endif
    };

    Image image;
    auto reported_at = std::chrono::steady_clock::now();
    auto next_frame_at = reported_at;

    /**
     * What one field needs from here: `before_field` sets the pad and any recording up,
     * `after_field` shows the picture and writes what was asked for, `finish` is the closing
     * report. The machine calls them at each vertical blank through `on_frame`, wherever the
     * program is at that moment: also inside host code that stands in for a guest function,
     * which cannot return here first.
     */
    int frame = 0;
    bool stop_run = false;

    /**
     * For working on the GS: what one frame is drawn with, by state, with
     * how many primitives and the area they span.
     *
     * The area and level of detail start at values no primitive can have, so the first one
     * replaces them.
     */
    struct Use {
        /**
         * Primitives drawn with the state, the box they span as least and greatest
         * coordinates, and the rank of the first among all primitives.
         */
        int count = 0, x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30), first = 0;

        /** The least and greatest level of detail among them. */
        float lod0 = 1e9f, lod1 = -1e9f;
    };

    std::map<std::string, Use> states;
    int order = 0;
    std::vector<u8> vif_bytes, vif_micro, vif_data;

    // Before a field runs: the window, the pad, and what a tool asked to record.
    auto before_field = [&] {
#ifndef OPENRAC_NO_WINDOW
        // The user closed the window (or pressed Escape).
        if (window_wanted && !window.pump()) {
            stop_run = true;
            return;
        }
#endif

        states.clear();
        order = 0;

        // This is the field whose states were asked for: collect them while it is drawn.
        if (frame == states_frame) {
            // The drawing side must be idle before a callback is installed on the GS.
            machine.drawing.sync();

            // Called by the GS for every primitive drawn, on the drawing thread: sums up per state.
            machine.graphics.gs.on_primitive = [&](const std::string& state,
                                                   int x0,
                                                   int y0,
                                                   int x1,
                                                   int y1,
                                                   float lod0,
                                                   float lod1) {
                Use& u = states[state];

                // The first primitive with this state fixes where the state ranks in the listing.
                if (u.count++ == 0) {
                    u.first = order;
                }

                order++;
                u.x0 = std::min(u.x0, x0);
                u.y0 = std::min(u.y0, y0);
                u.x1 = std::max(u.x1, x1);
                u.y1 = std::max(u.y1, y1);
                u.lod0 = std::min(u.lod0, lod0);
                u.lod1 = std::max(u.lod1, lod1);
            };
        }

        // The field's memory as one number.
        if (digest_file) {
            std::fprintf(digest_file, "%lld %016llx\n", static_cast<long long>(machine.frames),
                         static_cast<unsigned long long>(memory_hash(machine)));
        }

        // The fields whose memory `--dump-ram` keeps.
        if (dump_fields.count(static_cast<long long>(machine.frames))) {
            const std::string name = "ram_" + std::to_string(static_cast<long long>(machine.frames)) + ".bin";

            if (std::FILE* file = std::fopen(name.c_str(), "wb")) {
                std::fwrite(machine.memory.ram(0), 1, GuestMemory::kRamBytes, file);
                std::fclose(file);
            }
        }

        // The traced words of this field.
        if (trace_file) {
            std::fprintf(trace_file, "%lld", static_cast<long long>(machine.frames));
            for (const Traced& t : traced) {
                for (unsigned k = 0; k < t.words; k++) {
                    u32 value;
                    std::memcpy(&value, machine.memory.ram(t.address + 4 * k), 4);
                    std::fprintf(trace_file, " %08x", value);
                }
            }
            std::fputc('\n', trace_file);
        }

        // The pad for this field: the scripted presses that cover it.
        machine.pad.buttons = 0;
        for (const Press& p : presses) {
            // The press covers fields from its start for its length.
            if (frame >= p.frame && frame < p.frame + p.length) {
                machine.pad.buttons |= static_cast<u16>(p.buttons);
            }
        }

        // The sticks for this field: centred, or the last scripted position that covers it.
        machine.pad.left_x = machine.pad.left_y = 0x80;
        machine.pad.right_x = machine.pad.right_y = 0x80;
        for (const Stick& s : sticks) {
            if (frame >= s.frame && frame < s.frame + s.length) {
                (s.right ? machine.pad.right_x : machine.pad.left_x) = static_cast<u8>(s.x);
                (s.right ? machine.pad.right_y : machine.pad.left_y) = static_cast<u8>(s.y);
            }
        }

        for (const Write& w : writes) {
            bool covered = frame >= w.frame && frame < w.frame + w.length;
            bool inside = w.address + 4 <= GuestMemory::kRamBytes;

            // The change covers this field and the word is in main memory.
            if (covered && inside) {
                std::memcpy(machine.memory.ram(w.address), &w.value, 4);
            }
        }

#ifndef OPENRAC_NO_WINDOW
        // In a window the keyboard and controller add to the scripted presses.
        if (window_wanted) {
            host::Controls c = window.controls();
            machine.pad.buttons |= static_cast<u16>(c.buttons);
            machine.pad.left_x = c.left_x;
            machine.pad.left_y = c.left_y;
            machine.pad.right_x = c.right_x;
            machine.pad.right_y = c.right_y;
        }
#endif
        vif_bytes.clear();
        vif_micro.clear();
        vif_data.clear();

        // This is the field to record: keep VU1's memories from before it, then its VIF1 bytes.
        if (frame == vif_frame) {
            // The memories are read directly, so the drawing side must be idle.
            machine.drawing.sync();
            vif_micro.assign(machine.graphics.vif.micro.begin(), machine.graphics.vif.micro.end());
            vif_data.assign(machine.graphics.vif.data.begin(), machine.graphics.vif.data.end());
            machine.drawing.vif_copy = &vif_bytes;
        }
    };

    // After a field has run: recordings, listings, the picture, the pace, the periodic report.
    auto after_field = [&] {
        // Write the recording: the magic, the two memories before the field, then the VIF1 bytes.
        if (frame == vif_frame) {
            machine.drawing.vif_copy = nullptr;

            // If the file cannot be made nothing is written and no message is given.
            if (std::FILE* f = std::fopen(vif_file.c_str(), "wb")) {
                // The magic is "ORVIF1" and two zero bytes: 8 bytes.
                std::fwrite("ORVIF1\0", 1, 8, f);
                std::fwrite(vif_micro.data(), 1, vif_micro.size(), f);
                std::fwrite(vif_data.data(), 1, vif_data.size(), f);
                std::fwrite(vif_bytes.data(), 1, vif_bytes.size(), f);
                std::fclose(f);
                std::fprintf(
                    stderr,
                    "frame %d: %zu bytes for VIF1 written to %s\n",
                    frame,
                    vif_bytes.size(),
                    vif_file.c_str()
                );
            }
        }

        // A VU0 microprogram did not stop: further fields would show nothing useful.
        if (machine.ee.vu0_runaways) {
            stop_run = true;
            return;
        }

        // The states of this field are complete: print them in the order they first appeared.
        if (frame == states_frame) {
            machine.drawing.sync();
            machine.graphics.gs.on_primitive = nullptr;
            std::vector<std::pair<int, std::string>> lines;

            // One line per state: first rank, count, the area, then the state's own text.
            for (const auto& [state, u] : states) {
                // Room for the numbers: fixed-width columns of at most 96 characters.
                char head[96];
                std::snprintf(
                    head,
                    sizeof(head),
                    "%6d x%-6d [%4d,%4d - %4d,%4d] ",
                    u.first,
                    u.count,
                    u.x0,
                    u.y0,
                    u.x1,
                    u.y1
                );
                char tail[48] = "";

                // Show the level of detail only if some primitive set it and it was not all zero.
                if (u.lod1 >= u.lod0 && (u.lod0 != 0 || u.lod1 != 0)) {
                    std::snprintf(
                        tail,
                        sizeof(tail),
                        " | lod %.2f..%.2f",
                        static_cast<double>(u.lod0),
                        static_cast<double>(u.lod1)
                    );
                }
                lines.push_back({u.first, head + state + tail});
            }

            std::sort(lines.begin(), lines.end());
            for (const auto& line : lines) {
                std::printf("%s\n", line.second.c_str());
            }
        }

        bool shown = machine.drawing.picture(image);

        // Every field's picture from --ppm-from on.
        if (shown && !ppm.empty() && ppm_from >= 0 && frame >= ppm_from) {
            write_ppm(ppm + "_" + std::to_string(frame) + ".ppm", image);
        }

#ifndef OPENRAC_NO_WINDOW
        // The picture is shown at the games' 4:3 shape, if the display was on.
        if (window_wanted && shown) {
            window.present(image, 4.0f / 3.0f);
        }

        if (window_wanted) {
            // No faster than the console: one frame per field.
            next_frame_at += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(1.0 / machine.hz)
            );
            auto now = std::chrono::steady_clock::now();

            // Ahead of the schedule: wait. Behind it: start the next schedule from now.
            if (next_frame_at > now) {
                std::this_thread::sleep_until(next_frame_at);
            } else {
                next_frame_at = now;
            }
        }
#else
        // A build without SDL does not show the picture.
        (void)shown;
#endif

        // Every `report` fields print counts and the speed.
        if (report > 0 && (frame + 1) % report == 0) {
            // The counts come from the drawing side, so it must have caught up.
            machine.drawing.sync();
            auto now = std::chrono::steady_clock::now();
            double seconds = std::chrono::duration<double>(now - reported_at).count();
            reported_at = now;

            // The speed is `report` fields over the time taken; the floor avoids dividing by zero.
            std::fprintf(
                stderr,
                "frame %d: pc %08x ra %08x, %llu primitives, %llu pixels, %llu VU1 instructions, "
                "%.1f frames a second\n",
                frame + 1,
                machine.ee.pc,
                static_cast<u32>(machine.ee.gpr[31].lo),
                static_cast<unsigned long long>(machine.graphics.gs.stats.primitives),
                static_cast<unsigned long long>(machine.graphics.gs.stats.pixels),
                static_cast<unsigned long long>(machine.graphics.vu1_instructions),
                report / std::max(seconds, 1e-9)
            );
        }
    };

    // When the run is over: the reports and the files asked for. Returns the exit code.
    auto finish = [&]() -> int {
        if (!cover_path.empty()) {
            if (std::FILE* file = std::fopen(cover_path.c_str(), "w")) {
                for (u8 ran : cover_map) {
                    std::fputc(ran ? '1' : '0', file);
                }

                std::fclose(file);
            }
        }

        if (digest_file) {
            std::fclose(digest_file);
        }

        for (const Swap& swap : swaps) {
            std::fprintf(stderr, "swap %x: %llu calls\n", swap.entry, static_cast<unsigned long long>(swap.calls));
        }

        // The closing report: the drawing side must finish first so its counts are complete.
        machine.drawing.sync();
        std::fprintf(
            stderr,
            "gs: %llu texture levels decoded (%llu texels), %llu transfers, %llu batches drawn\n",
            static_cast<unsigned long long>(machine.graphics.gs.stats.texture_decodes),
            static_cast<unsigned long long>(machine.graphics.gs.stats.texels_decoded),
            static_cast<unsigned long long>(machine.graphics.gs.stats.transfers),
            static_cast<unsigned long long>(machine.graphics.gs.stats.flushes)
        );
        std::fprintf(
            stderr,
            "stopped after %llu frames at pc %08x (ra %08x)\n",
            static_cast<unsigned long long>(machine.frames),
            machine.ee.pc,
            static_cast<u32>(machine.ee.gpr[31].lo)
        );

        // Everything the model met and does not do, once each with how often.
        for (const auto& [what, count] : machine.notes) {
            std::fprintf(
                stderr,
                "  not modelled: %s (%llu times)\n",
                what.c_str(),
                static_cast<unsigned long long>(count)
            );
        }

        // The EE met instructions it does not know: say how many, and the last one.
        if (machine.ee.unknown) {
            std::fprintf(
                stderr,
                "  %llu unknown EE instructions, the last %08x at %08x\n",
                static_cast<unsigned long long>(machine.ee.unknown),
                machine.ee.last_unknown,
                machine.ee.last_unknown_pc
            );
        }

        // Either vector unit or VIF1 met something it cannot decode.
        if (machine.vu0.unknown_ops || machine.graphics.vu1.unknown_ops
            || machine.graphics.vif.unknown_codes) {
            std::fprintf(
                stderr,
                "  unknown: %llu VU0, %llu VU1 instructions, %llu VIF1 codes\n",
                static_cast<unsigned long long>(machine.vu0.unknown_ops),
                static_cast<unsigned long long>(machine.graphics.vu1.unknown_ops),
                static_cast<unsigned long long>(machine.graphics.vif.unknown_codes)
            );
        }

        // A VU0 microprogram did not stop: say where it started and who started it.
        if (machine.ee.vu0_runaways) {
            std::fprintf(
                stderr,
                "  %llu VU0 microprograms did not stop (the first started at %u by the EE at "
                "%08x)\n",
                static_cast<unsigned long long>(machine.ee.vu0_runaways),
                machine.ee.vu0_runaway_start,
                machine.ee.vu0_runaway_from
            );
            u32 at = machine.ee.vu0_runaway_start;

            // The listing is long, so it is printed only when asked for in the environment.
            if (std::getenv("OPENRAC_VU0_LISTING")) {
                // The program as it sits in VU0, and where it was: 4 KB of memory is 512 pairs.
                for (u32 n = 0; n < 512; n++) {
                    u32 up = load<u32>(&machine.vif0.micro[n * 8 + 4]),
                        low = load<u32>(&machine.vif0.micro[n * 8]);

                    // Empty slots are not listed.
                    if (up || low) {
                        std::fprintf(stderr, "%s\n", vudis::pair(n, up, low).c_str());
                    }
                }

                // The program counter and the sixteen integer registers.
                std::fprintf(stderr, "pc %u; vi:", machine.vu0.pc);
                for (unsigned n = 0; n < 16; n++) {
                    std::fprintf(stderr, " %04x", machine.vu0.vi[n]);
                }
                std::fprintf(stderr, "\n");
            }

            // `at` is kept for working in a debugger; reading it here quiets the unused warning.
            (void)at;
        }

        // VU1 programs that were cut off at the instruction limit.
        if (machine.graphics.vu1_runaways) {
            std::fprintf(
                stderr,
                "  %llu of %llu VU1 program starts did not stop (the last from %u, at %u when cut "
                "off)\n",
                static_cast<unsigned long long>(machine.graphics.vu1_runaways),
                static_cast<unsigned long long>(machine.graphics.vu1_starts),
                machine.graphics.vu1_runaway_start,
                machine.graphics.vu1_runaway_pc
            );
        }

        // Asked for: how much each of the game's vector unit programs ran, by unit.
        if (vu_programs) {
            report_vu_programs("VU1", machine.vu_chunks, machine.graphics.vu1.uses());
            report_vu_programs("VU0", machine.vu_chunks, machine.vu0.uses());
        }

        // What host code ran, the 20 busiest functions first.
        if (!native.empty()) {
            machine.native.report(stderr, 20);
        }

        // The listed functions' calls were asked for: name, calls, first and last field (-1: none).
        if (!calls_file.empty()) {
            // If the file cannot be made nothing is written and no message is given.
            if (std::FILE* out = std::fopen(calls_file.c_str(), "w")) {
                for (const Capture& c : captures) {
                    std::fprintf(out, "%s\t%llu\t%lld\t%lld\t%u\n", c.name.c_str(), c.calls, c.first, c.last, c.taken);
                }
                std::fclose(out);
            }
        }

        // The counted calls were asked for.
        if (!native_calls.empty()) {
            std::FILE* out = std::fopen(native_calls.c_str(), "w");

            // If the file cannot be made nothing is written and no message is given.
            if (out) {
                machine.native.write_calls(out);
                std::fclose(out);
            }
        }

        // A sound file was asked for.
        if (!wav.empty()) {
            // If the file cannot be made nothing is written and no message is given.
            if (std::FILE* f = std::fopen(wav.c_str(), "wb")) {
                // A plain sound file: 16-bit, two channels.
                u32 bytes = static_cast<u32>(heard.size() * 2), rate = sys::Sound::kRate;
                u8 head[44] = {'R', 'I', 'F', 'F', 0,   0,   0,   0, 'W', 'A', 'V',
                               'E', 'f', 'm', 't', ' ', 16,  0,   0, 0,   1,   0,
                               2,   0,   0,   0,   0,   0,   0,   0, 0,   0,   4,
                               0,   16,  0,   'd', 'a', 't', 'a', 0, 0,   0,   0};

                /*
                 * RIFF size at byte 4 (the data plus the 36 bytes after it), sample rate at 24,
                 * bytes a second at 28 (rate times 4 bytes a frame), data size at 40 (documented).
                 */
                store<u32>(head + 4, bytes + 36);
                store<u32>(head + 24, rate);
                store<u32>(head + 28, rate * 4);
                store<u32>(head + 40, bytes);
                std::fwrite(head, 1, sizeof(head), f);
                std::fwrite(heard.data(), 2, heard.size(), f);
                std::fclose(f);
                std::fprintf(
                    stderr,
                    "%.1f seconds of sound written to %s\n",
                    static_cast<double>(heard.size()) / 2 / rate,
                    wav.c_str()
                );
            }
        }

        // The last picture was asked for and the display was on.
        if (!ppm.empty() && machine.drawing.picture(image)) {
            write_ppm(ppm, image);
        }

        return 0;
    };

    /*
     * Called by the machine when a field is complete. The run ends from here, not from the
     * loop below: at that moment the program may be inside host code many calls deep.
     */
    machine.on_frame = [&] {
        after_field();
        frame++;

        // The frame limit, a halt, a closed window or a runaway VU0 program.
        if (stop_run || frame >= frames || machine.halted) {
            std::exit(finish());
        }

        before_field();

        // The window was closed.
        if (stop_run) {
            std::exit(finish());
        }
    };

    before_field();

    if (!cover_map.empty()) {
        machine.ee.cover = cover_map.data();
        machine.ee.cover_base = cover_base;
        machine.ee.cover_bytes = static_cast<u32>(cover_map.size() * 4);
    }

    if (returns_file) {
        Ee& ee = machine.ee;

        ee.on_watch = [&, returns_file]() {
            if (returns_pending.empty() || static_cast<u32>(ee.gpr[29].lo) != returns_pending.back().second) {
                return;
            }

            std::fprintf(returns_file, "%lld %x v0 %llx v1 %llx f0 %x", static_cast<long long>(machine.frames),
                         returns_pending.back().first, static_cast<unsigned long long>(ee.gpr[2].lo),
                         static_cast<unsigned long long>(ee.gpr[3].lo), ee.fpr[0]);

            for (int r : {16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30, 31}) {
                std::fprintf(returns_file, " %llx", static_cast<unsigned long long>(ee.gpr[r].lo));
            }

            for (int f = 20; f < 32; f++) {
                std::fprintf(returns_file, " %x", ee.fpr[f]);
            }

            std::fprintf(returns_file, " mem %016llx\n", static_cast<unsigned long long>(memory_hash(machine)));
            std::fflush(returns_file);
            returns_pending.pop_back();
            ee.watch_pc = returns_pending.empty() ? ~0u : returns_pending.back().first;
        };
    }

    if (!swaps.empty() || returns_file) {
        machine.ee.on_call = [&, returns_file](u32 target) {
            if (returns_file && target == returns_entry) {
                u32 word = 0;

                std::memcpy(&word, machine.memory.ram(target), 4);

                if (word == returns_first_word) {
                    returns_pending.push_back({static_cast<u32>(machine.ee.gpr[31].lo),
                                               static_cast<u32>(machine.ee.gpr[29].lo)});
                    machine.ee.watch_pc = returns_pending.back().first;
                }
            }

            for (Swap& swap : swaps) {
                u32 word = 0;

                // A level's place holds other code until that level is loaded.
                if (target != swap.entry) {
                    continue;
                }

                std::memcpy(&word, machine.memory.ram(target), 4);

                if (word != swap.first_word) {
                    continue;
                }

                for (const auto& [address, bytes] : swap.code) {
                    std::memcpy(machine.memory.ram(address), bytes.data(), bytes.size());
                }

                swap.calls++;
                machine.ee.next_pc = swap.target;
            }
        };
    }

    // Runs until `on_frame` ends the process; it comes back here only when the program halts.
    while (!stop_run && !machine.halted && frames > 0) {
        machine.run_frame();
    }

    return finish();
}
