// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-extract/src/lib.rs and
// json.rs (the JSON-lines stream of docs/plan/launcher_contract.md): ISC License,
// Copyright (c) 2026 ReRAC contributors.
//
// What the extractor tells whoever runs it. Failures carry OpenGOAL's numbers
// for the same failures (its extractor's ExtractorErrorCode), as
// tools/extractor.py does, so a player's report reads the same in both
// projects; 4060 and up are OpenRAC's own. Output is either lines for a
// person or, with --json, one JSON object per line for the launcher:
//
//   {"type":"progress","stage":"identify|copy|decompile","done":N,"total":N,"file":"..."}
//   {"type":"info","message":"..."}
//   {"type":"disc","serial":"...","version":"rac1/pal","region":"PAL","game":"rac1",
//    "title":"...","supported":true,"boot_sha1":"..."}
//   {"type":"error","code":4002,"message":"..."}
//   {"type":"done","elapsed_ms":N}
//
// Every run ends with exactly one "done" or one "error" line, and nothing
// after it. Progress lines are limited to about ten a second; each stage ends
// with a line where done == total.

#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>

namespace openrac::extract {

enum class ErrorCode : int {
    NoBootExecutable = 4000,  // no SYSTEM.CNF, or the file it names is missing
    UnknownVersion = 4001,    // not a version of this game OpenRAC knows
    UnknownBuild = 4002,      // the serial is known, its boot executable is not
    WrongFileCount = 4011,
    WrongContents = 4012,
    NotIso = 4020,           // not an ISO 9660 image
    DecompileFailed = 4030,  // the disc's data is not what its layout says
    NotImageFile = 4040,     // not a disc image file (.iso)
    ImageTooSmall = 4041,
    NotYet = 4060,       // OpenRAC: this step does not exist yet for this game
    NoSpace = 4061,      // OpenRAC: not enough free space where the files go
    CannotWrite = 4062,  // OpenRAC: a file or folder could not be written
    CannotRead = 4063,   // OpenRAC: the image could not be read, or is truncated
};

std::string_view error_text(ErrorCode code);

class ExtractError : public std::runtime_error {
public:
    ExtractError(ErrorCode code, std::string detail = {});

    ErrorCode code() const { return m_code; }

    const std::string& detail() const { return m_detail; }

private:
    ErrorCode m_code;
    std::string m_detail;
};

enum class Stage {
    Identify,
    Copy,
    Decompile,
};

// What identify learned, as the "disc" line reports it.
struct DiscReport {
    std::string serial;
    std::string version;  // "rac1/pal", empty when the build is not known
    std::string region;
    std::string game;  // "rac1".."rac4", or "unknown"
    std::string title;
    bool supported = false;
    std::string boot_sha1;
};

class Reporter {
public:
    // Lines for a person go to `out` (failures to `err`); JSON lines all go
    // to `out`.
    explicit Reporter(bool json, std::FILE* out = stdout, std::FILE* err = stderr);

    bool json() const { return m_json; }

    void info(std::string_view message);
    void progress(Stage stage, std::uint64_t done, std::uint64_t total, std::string_view file);
    void disc(const DiscReport& disc);
    void error(const ExtractError& error);
    void done();

private:
    void line(std::FILE* to, const std::string& text);

    bool m_json;
    std::FILE* m_out;
    std::FILE* m_err;
    std::chrono::steady_clock::time_point m_start;
    std::chrono::steady_clock::time_point m_last_progress{};
};

// `text` as a JSON string, quotes included.
std::string json_string(std::string_view text);

}  // namespace openrac::extract
