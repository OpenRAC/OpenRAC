// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Command-line validation, exit status and the persistent build summary.
#include <cstdio>
#include <stdexcept>

#include "renderer/vulkan/pipelines/shader_backend.h"
#include "renderer/vulkan/pipelines/shader_package.h"
#include "tools/shader_compiler/compiler.h"
#include "tools/shader_compiler/file_io.h"

namespace {
// Saves and prints one invocation summary; publication errors reach the command-line handler.
void save_report(
    const openrac::shader_compiler::Compiler& compiler,
    const std::filesystem::path& output,
    bool success,
    std::string_view error = {}
) {
    using openrac::shader_compiler::write_file;
    const auto summary = compiler.summary(success, error);

    if (!output.empty()) {
        write_file(output / "shader-compiler.log", summary);
    }

    std::fprintf(success ? stdout : stderr, "%s", summary.c_str());
}

// Runs the invocation while the executable boundary retains exception reporting.
int run_compiler(int argc, char** argv) {
    using namespace openrac::shader_compiler;
    using namespace openrac::vkgl;

    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::printf(
            "openrac-shader-compiler package format 1\n%s\n%s\n",
            compiler_detail::compiler_version().c_str(),
            shader_package_fingerprint().c_str()
        );
        return 0;
    }

    if ((argc != 3 && argc != 5) || std::string_view(argv[1]) != "--output"
        || (argc == 5 && std::string_view(argv[3]) != "--expect-fingerprint")) {
        std::fprintf(
            stderr,
            "Usage: openrac-shader-compiler --output <directory> [--expect-fingerprint <hash>]\n"
        );
        return 2;
    }

    const std::filesystem::path output(argv[2]);
    Compiler compiler(output);

    try {
        if (argc == 5 && std::string_view(argv[4]) != shader_package_fingerprint()) {
            throw std::runtime_error(
                "host shader compiler does not match this source tree and build configuration; "
                "rebuild it"
            );
        }

        compiler.run();
        save_report(compiler, output, true);
        return 0;
    } catch (const std::exception& error) {
        try {
            save_report(compiler, output, false, error.what());
        } catch (const std::exception& log_error) {
            std::fprintf(
                stderr,
                "Shader compilation failed: %s\nCannot save build log: %s\n",
                error.what(),
                log_error.what()
            );
        }

        return 1;
    }
}
}  // namespace

int main(int argc, char** argv) {
    try {
        return run_compiler(argc, argv);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "cannot build shader package: %s\n", error.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "cannot build shader package: unknown exception\n");
        return 1;
    }
}
