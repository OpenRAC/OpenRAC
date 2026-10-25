// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace openrac::test {
// Synthetic data only: never reads or copies game assets.
struct WaterTestBake {
    std::filesystem::path folder;
    std::array<char, 32> fingerprint{};

    WaterTestBake() {
        folder = std::filesystem::temp_directory_path()
                 / ("openrac-water-test-"
                    + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if (!std::filesystem::create_directory(folder)) {
            throw std::runtime_error("water test directory exists");
        }
        fingerprint.fill('x');
        write_mesh();
        write_bake();
    }

    ~WaterTestBake() {
        std::error_code ignored;
        std::filesystem::remove(folder / "water.mesh", ignored);
        std::filesystem::remove(folder / "water.bin", ignored);
        std::filesystem::remove(folder, ignored);
    }

    void write_mesh() {
        std::ofstream out(folder / "water.mesh", std::ios::binary);
        const auto put = [&](const auto& value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        out.write("ORWMESH2", 8);
        put(std::uint32_t(1));
        put(std::uint32_t(1));
        put(fingerprint);
        put(std::uint32_t(1));
        put(std::array<float, 9>{0, 0, 1, 1, 0, 1, 0, 1, 1});
        put(std::array<std::int32_t, 4>{256, 0, 0, 256});
        put(std::uint32_t(3));
        put(std::array<float, 6>{0, 0, 1, 0, 0, 1});
        if (!out) {
            throw std::runtime_error("cannot write test mesh");
        }
    }

    void write_bake() {
        std::ofstream out(folder / "water.bin", std::ios::binary);
        const auto put = [&](const auto& value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        out.write("ORWATER1", 8);
        put(std::uint32_t(3));
        put(std::uint32_t(1));
        put(std::uint32_t(256));
        put(fingerprint);
        put(std::array<float, 20>{0, 0, 0, 1, 1, 0, 0, 16, 0, 1, 0, 16, 0, 0, 1, 0, 0, 0, .5f, 0});
        for (int i = 0; i < 256; ++i) {
            put(std::array<float, 4>{513, 8, 0, 0});
        }
        for (int i = 0; i < 256; ++i) {
            put(std::array<float, 4>{.75f, 0, 0, 1});
        }
        if (!out) {
            throw std::runtime_error("cannot write test bake");
        }
    }

    WaterTestBake(const WaterTestBake&) = delete;
    WaterTestBake& operator=(const WaterTestBake&) = delete;
};
}  // namespace openrac::test
