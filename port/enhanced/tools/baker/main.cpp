// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Enhanced baker command-line dispatch for interior inference, GI and reflection transport.
#include <algorithm>
#include <cfenv>
#include <charconv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "common/json.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_material.h"
#include "enhanced/lighting/probe_texture_encode.h"
#include "enhanced/tools/baker/batch.h"
#include "enhanced/tools/baker/batch_command.h"
#include "enhanced/tools/baker/output/report.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/coordinator.h"
#include "enhanced/tools/baker/pipeline/pipeline.h"
#include "enhanced/tools/baker/probe_device.h"
#include "enhanced/tools/baker/sources/activation.h"
#include "enhanced/tools/baker/sources/emission_migration.h"
#include "enhanced/tools/baker/sources/emission_prepare.h"
#include "enhanced/tools/baker/sources/emission_review.h"
#include "enhanced/tools/baker/sources/emission_textures.h"
#include "enhanced/tools/baker/sources/interior_prepare.h"
#include "enhanced/tools/baker/sources/interior_sources.h"
#include "enhanced/tools/baker/sources/interior_stage.h"
#include "enhanced/tools/baker/sources/level_scene.h"
#include "enhanced/tools/baker/sources/probe_fingerprint.h"
#include "enhanced/tools/baker/sources/probe_grid.h"
#include "enhanced/tools/water/water_bake.h"
#include "enhanced/tools/water/water_source.h"

namespace fs = std::filesystem;
using namespace openrac;
using namespace openrac::lighting;

namespace {
// ---- Standalone command parsing ----

// Rejects partial and signed integer controls before any asset is loaded.
unsigned integer(std::string_view text) {
    unsigned out = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), out);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::runtime_error("invalid integer option");
    }

    return out;
}

// Rejects trailing text and nonfinite scalar controls.
float real(const std::string& text) {
    std::size_t end = 0;
    const float value = std::stof(text, &end);

    if (end != text.size() || !std::isfinite(value)) {
        throw std::runtime_error("invalid float option");
    }

    return value;
}

// Prints the standalone preparation, inspection and transport entry points.
void usage() {
    std::cout
        << "Offline enhanced lighting bake (no game window)\n"
        << "openrac-enhanced-baker --inspect-emissive-task LEVELS_DIR REVIEW_JSON TASK_ID\n"
        << "openrac-enhanced-baker --calculate-mesh-emission LEVEL_DIR\n"
        << "openrac-enhanced-baker --migrate-mesh-emission LEVEL_DIR MATERIAL_OVERRIDES.json\n"
        << "openrac-enhanced-baker --bake-all --levels-dir DIR --output-dir DIR [--mods-dir DIR] "
           "[--update]\n"
        << "  --jobs N --memory-mib N\n"
        << "  --lz4-assets (experimental lossless file compression; requires OPENRAC_BAKE_LZ4)\n"
        << "  --compression auto|none|bc6h|astc (colour: default auto; ASTC colour requires HDR "
           "support)\n"
        << "openrac-enhanced-baker --level-dir DIR [--inspect | --output FILE | --verify FILE]\n"
        << "openrac-enhanced-baker --infer-interiors INPUT.json OUTPUT.json [SPACING]\n"
        << "openrac-enhanced-baker --prepare-level-interiors LEVEL_DIR GAMEPLAY.bin OUTPUT_DIR\n"
        << "openrac-enhanced-baker --prepare-level-emission LEVEL_DIR CATALOG.json DEFAULTS.json\n"
        << "openrac-enhanced-baker --print-emission-fingerprint LEVEL_DIR PROFILES_DIR\n"
        << "  --threads N (optional emission hashing worker budget)\n"
        << "openrac-enhanced-baker --decode-water LEVEL_DIR\n"
        << "openrac-enhanced-baker --bake-pokitaru-coast LEVEL_DIR X Y SIZE CELL\n"
        << "openrac-enhanced-baker --bake-coastal-wave PATCH.json PERIOD HEADING OFFSHORE_DEPTH\n"
        << "openrac-enhanced-baker --bake-ocean-spectrum PATCH.json OFFSHORE_DEPTH\n"
        << "openrac-enhanced-baker --bake-water INPUT.json OUTPUT.bin\n"
        << "  --print-interior-profile (interior stage cache identity, no scene load)\n"
        << "  --print-profile (resolved grid/quality defaults as JSON, no scene load)\n"
        << "  --batch-output DIR (prepare once, then read one JSON region plan on stdin)\n"
        << "  --prepare-interiors GAMEPLAY.bin (prepare interiors inside the batch process)\n"
        << "  --resume (with --batch-output: keep the grids a stopped bake verified)\n"
        << "  --spacing 128 (or X,Y,Z) --resolution 6 --rays 128 --bounces 2 --seed 1\n"
        << "  --tracy-wait SECONDS (wait up to 120 seconds for a Tracy capture)\n"
        << "  --threads 0 (all CPU threads; 1 for serial baking, same quality)\n"
        << "  --light-set 0 --environment 0.1 (uniform fallback radiance)\n"
        << "  --sky level|uniform --sky-intensity 1 (linear radiance multiplier)\n"
        << "  --bounds minX,minY,minZ,maxX,maxY,maxZ (optional local volume)\n"
        << "  --lattice-bounds minX,minY,minZ,maxX,maxY,maxZ (exact integer cell indices)\n"
        << "  --max-distance N (visibility range; default 1.5 times cell diagonal)\n"
        << "  --compare-coarse FILE (with --verify LOCAL_FILE)\n"
        << "Defaults: per-level probe-bake.json; explicit options override its settings.\n"
        << "Existing output files are never overwritten. Bakes stay local.\n";
}

// ---- Source preparation commands ----

// Runs interior inference through the same executable used for GI and reflection baking.
void interior_command(int argc, char** argv) {
    // Input and output are required; the optional spacing remains in game units.
    if (argc != 4 && argc != 5) {
        throw std::runtime_error("--infer-interiors requires INPUT.json OUTPUT.json [SPACING]");
    }

    // The default floor-sample spacing is two game units.
    const float spacing = argc == 5 ? real(argv[4]) : 2.0f;
    const auto result = infer_interior_file(argv[2], argv[3], spacing);

    // Native diagnostics flow through main's existing failure handling.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Runs native preparation alone for diagnostics and cold-cache regression checks.
void prepare_interiors_command(int argc, char** argv) {
    // The standalone preparation stage needs a level, retained gameplay and an output directory.
    if (argc != 5) {
        throw std::runtime_error(
            "--prepare-level-interiors requires LEVEL_DIR GAMEPLAY.bin OUTPUT_DIR"
        );
    }

    const auto result = prepare_level_interiors(argv[2], argv[3], argv[4]);

    // Preserve the stage diagnostic through the existing command-line error boundary.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

// Refreshes profiles before cache checks or scene loading without decoding meshes.
void prepare_emission_command(int argc, char** argv) {
    // Paths are explicit so installed launchers use their matching tracked game profiles.
    if (argc != 5 && argc != 7) {
        throw std::runtime_error(
            "--prepare-level-emission requires LEVEL_DIR CATALOG.json DEFAULTS.json [--threads N]"
        );
    }

    BakeExecution execution{1};

    // Standalone preparation may use the same bounded hashing budget as automatic bakes.
    if (argc == 7) {
        if (std::string_view(argv[5]) != "--threads" || integer(argv[6]) > 256) {
            throw std::runtime_error("emission threads must be between zero and 256");
        }

        execution.threads = integer(argv[6]);
    }

    const auto result = prepare_level_emission(argv[2], argv[3], argv[4], execution);

    // Source or publication errors keep the previous authored profile intact.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    std::cout << "emission_profile_changed " << static_cast<int>(*result) << '\n';
}

// Runs a staged water bake, optionally waiting for a capture before the short numerical pipeline.
void water_command(int argc, char** argv) {
    // Only the two required paths and an optional profiler wait are accepted.
    if (argc != 4 && argc != 6) {
        throw std::runtime_error(
            "--bake-water requires INPUT.json OUTPUT.bin [--tracy-wait SECONDS]"
        );
    }

    // A short bake can otherwise finish before an on-demand client connects.
    if (argc == 6) {
        const float seconds = real(argv[5]);

        // Profile waits retain the existing two-minute upper bound.
        if (std::string_view(argv[4]) != "--tracy-wait" || seconds < 0 || seconds > 120) {
            throw std::runtime_error("invalid water profiler wait");
        }

        // Waits stay on the controller, before any geometry is loaded.
        if (!profiling::wait_for_connection(
                std::chrono::milliseconds(static_cast<int>(seconds * 1000))
            )) {
            throw std::runtime_error("Tracy capture not connected (requires OPENRAC_TRACY)");
        }
    }

    const auto result = openrac::lighting::water::bake(argv[2], argv[3]);

    // Failed staged pairs must never be installed by the caller.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
}

}  // namespace

// ---- Entry point ----

// Dispatches native enhanced-baker stages and reports failures at the command-line boundary.
int main(int argc, char** argv) {
    try {
        if (argc == 4 && std::string_view(argv[1]) == "--print-emission-fingerprint") {
            std::cout << emission_texture_fingerprint(argv[2], argv[3]) << '\n';
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--print-mod-key") {
            std::cout << probe_variant_key(argv[2]) << '\n';
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--print-modded-source") {
            std::cout << probe_input_hash(argv[2], argv[3]) << '\n';
            return 0;
        }
        profiling::thread_name("Enhanced bake");

        if (argc == 3 && std::string_view(argv[1]) == "--calculate-mesh-emission") {
            std::cout << calculate_mesh_emission(argv[2]);
            return 0;
        }

        if (argc == 4 && std::string_view(argv[1]) == "--migrate-mesh-emission") {
            migrate_mesh_emission(argv[2], argv[3]);
            return 0;
        }

        if (argc > 1 && std::string_view(argv[1]) == "--inspect-emissive-task") {
            if (argc != 5) {
                throw std::runtime_error(
                    "--inspect-emissive-task requires LEVELS_DIR REVIEW_JSON TASK_ID"
                );
            }
            inspect_emissive_task(argv[2], argv[3], argv[4], std::cout);
            return 0;
        }

        // One native process admits multiple levels while Python supplies preparation and plans.
        if (argc == 2 && std::string_view(argv[1]) == "--coordinate-levels") {
            return coordinate_levels(std::cin, std::cout);
        }

        // Cache checks use a semantic water identity rather than the executable's build hash.
        if (argc == 2 && std::string_view(argv[1]) == "--print-water-profile") {
            std::cout << "{\"version\":1,\"algorithm\":\"native-water-v1\",\"format\":3}\n";
            return 0;
        }
        // Inspection exposes the declarative decoder without running numerical water baking.
        if (argc == 3 && std::string_view(argv[1]) == "--decode-water") {
            const auto decoded = water::decode_sources(argv[2]);

            if (!decoded) {
                throw std::runtime_error(decoded.error().message);
            }

            std::cout << bake_json::json_text(*decoded) << '\n';
            return 0;
        }

        // The experimental patch is emitted separately from launcher water assets.
        if (argc > 1 && std::string_view(argv[1]) == "--bake-pokitaru-coast") {
            if (argc != 7) {
                throw std::runtime_error("--bake-pokitaru-coast requires LEVEL_DIR X Y SIZE CELL");
            }
            const auto patch = water::pokitaru_coast(
                argv[2], {real(argv[3]), real(argv[4])}, integer(argv[5]), real(argv[6])
            );
            if (!patch) {
                throw std::runtime_error(patch.error().message);
            }
            std::cout << *patch << '\n';
            return 0;
        }

        // The spectrum keeps its dominant coastal responses in one runtime sidecar.
        if (argc > 1 && std::string_view(argv[1]) == "--bake-ocean-spectrum") {
            if (argc != 4) {
                throw std::runtime_error("--bake-ocean-spectrum requires PATCH OFFSHORE_DEPTH");
            }
            const auto spectrum = water::ocean_spectrum(argv[2], real(argv[3]));
            if (!spectrum) {
                throw std::runtime_error(spectrum.error().message);
            }
            std::cout << *spectrum << '\n';
            return 0;
        }

        // Bake one steady incoming mode before integrating any wave response with rendering.
        if (argc > 1 && std::string_view(argv[1]) == "--bake-coastal-wave") {
            if (argc != 6) {
                throw std::runtime_error("--bake-coastal-wave requires PATCH PERIOD HEADING DEPTH");
            }
            const auto wave =
                water::coastal_wave(argv[2], real(argv[3]), real(argv[4]), real(argv[5]));
            if (!wave) {
                throw std::runtime_error(wave.error().message);
            }
            std::cout << *wave << '\n';
            return 0;
        }

        // Version-specific decoding supplies bounded generic inputs to the numerical water pipeline.
        if (argc > 1 && std::string_view(argv[1]) == "--bake-water") {
            water_command(argc, argv);
            return 0;
        }

        const std::vector<std::string> arguments(argv + 1, argv + argc);

        // Launcher bakes own scheduling and publication directly, without a Python coordinator.
        if (std::find(arguments.begin(), arguments.end(), "--bake-all") != arguments.end()) {
            return run_bake_pipeline(arguments, std::cout);
        }

        // Standalone and coordinated batches share exactly the same level preparation code.
        if (std::find(arguments.begin(), arguments.end(), "--batch-output") != arguments.end()) {
            return run_batch_command(arguments, std::cin, std::cout);
        }

        // Interior cache identity stays independent of unrelated GI and reflection changes.
        if (argc == 2 && std::string_view(argv[1]) == "--print-interior-profile") {
            std::cout << interior_stage_profile() << '\n';
            return 0;
        }

        // Interior preparation runs before transport loads the generated emission sources.
        if (argc > 1 && std::string_view(argv[1]) == "--infer-interiors") {
            interior_command(argc, argv);
            return 0;
        }

        // Stage-only preparation shares exactly the same implementation as persistent batches.
        if (argc > 1 && std::string_view(argv[1]) == "--prepare-level-interiors") {
            prepare_interiors_command(argc, argv);
            return 0;
        }

        // Automatic cache checks need refreshed emission before deciding whether transport is reusable.
        if (argc > 1 && std::string_view(argv[1]) == "--prepare-level-emission") {
            prepare_emission_command(argc, argv);
            return 0;
        }

        // ---- Single-grid inspection, verification and baking ----

        fs::path directory, overrides, output, verify, compare_coarse;
        bool inspect = false, game_rounding = false, print_profile = false;

        // Resolve the directory first: explicit CLI overrides win in any order.
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::string_view(argv[i]) == "--level-dir") {
                directory = argv[++i];
            } else if (std::string_view(argv[i]) == "--level-overrides") {
                overrides = argv[++i];
            } else if (std::string_view(argv[i]) == "--tracy-wait") {
                const float seconds = real(argv[++i]);

                if (seconds < 0 || seconds > 120) {
                    throw std::runtime_error("Tracy wait must be between 0 and 120 seconds");
                }

                if (!profiling::wait_for_connection(
                        std::chrono::milliseconds(static_cast<int>(seconds * 1000))
                    )) {
                    throw std::runtime_error(
                        "Tracy capture not connected (requires OPENRAC_TRACY)"
                    );
                }
            }
        }

        const profiling::Zone trace("Probe baker");

        auto profile =
            directory.empty() ? LevelBakeProfile{} : load_level_bake_profile(directory, overrides);
        Float3 spacing = profile.sparse_spacing;
        Float3 environment = profile.activation.environment;
        unsigned resolution = profile.map_resolution, light_set = profile.activation.light_set;
        BakeSettings settings = profile.activation.bake;
        settings.texture_format = ProbeTextureFormat::Automatic;
        unsigned threads = 0;
        std::optional<std::array<float, 6>> bounds;
        bool lattice_bounds =
            false;  // Local coordinates name integer cells rather than world units.
        std::optional<float> max_distance;

        for (int i = 1; i < argc; ++i) {
            const std::string_view argument = argv[i];

            if (argument == "--help") {
                usage();
                return 0;
            }

            if (argument == "--print-profile") {
                print_profile = true;
                continue;
            }

            if (argument == "--inspect") {
                inspect = true;
                continue;
            }

            if (argument == "--game-rounding") {
                game_rounding = true;
                continue;
            }

            if (i + 1 >= argc) {
                throw std::runtime_error("option needs a value");
            }

            const std::string value = argv[++i];

            if (argument == "--compression") {
                settings.texture_format = parse_probe_texture_format(value);
            } else if (argument == "--level-dir" || argument == "--level-overrides"
                       || argument == "--tracy-wait") {
                // The first pass resolves level paths and waits before any asset loading.
            } else if (argument == "--output") {
                output = value;
            } else if (argument == "--verify") {
                verify = value;
            } else if (argument == "--compare-coarse") {
                compare_coarse = value;
            } else if (argument == "--max-distance") {
                max_distance = real(value);
            } else if (argument == "--spacing") {
                if (value.find(',') == std::string::npos) {
                    spacing.fill(real(value));
                } else {
                    std::istringstream stream(value);
                    std::string component;

                    for (float& n : spacing) {
                        if (!std::getline(stream, component, ',')) {
                            throw std::runtime_error("spacing needs X,Y,Z");
                        }

                        n = real(component);
                    }

                    if (std::getline(stream, component, ',') || value.back() == ',') {
                        throw std::runtime_error("spacing needs exactly three coordinates");
                    }
                }
            } else if (argument == "--bounds" || argument == "--lattice-bounds") {
                lattice_bounds = argument == "--lattice-bounds";
                std::array<float, 6> parsed{};
                std::istringstream stream(value);
                std::string component;

                for (float& n : parsed) {
                    if (!std::getline(stream, component, ',')) {
                        throw std::runtime_error("bounds needs six comma-separated coordinates");
                    }

                    n = real(component);
                }

                if (std::getline(stream, component, ',')) {
                    throw std::runtime_error("bounds needs exactly six coordinates");
                }

                bounds = parsed;
            } else if (argument == "--environment") {
                environment.fill(real(value));
            } else if (argument == "--sky") {
                if (value != "level" && value != "uniform") {
                    throw std::runtime_error("sky must be level or uniform");
                }

                settings.use_level_sky = value == "level";
            } else if (argument == "--sky-intensity") {
                settings.sky_intensity = real(value);

                if (settings.sky_intensity < 0) {
                    throw std::runtime_error("negative sky intensity");
                }
            } else if (argument == "--resolution") {
                resolution = integer(value);
            } else if (argument == "--light-set") {
                light_set = integer(value);
            } else if (argument == "--rays") {
                settings.rays_per_probe = integer(value);
                settings.adaptive_rays = false;
                profile.explicit_ray_policy = true;
            } else if (argument == "--bounces") {
                settings.bounces = integer(value);
            } else if (argument == "--threads") {
                threads = integer(value);
            } else if (argument == "--seed") {
                settings.seed = integer(value);
            } else {
                throw std::runtime_error("unknown bake option");
            }
        }

        if (directory.empty() || (!inspect && !print_profile && output.empty() && verify.empty())) {
            usage();
            return 1;
        }

        if (settings.texture_format == ProbeTextureFormat::Automatic) {
            settings.texture_format = detect_probe_texture_format();
        }
        if (print_profile) {
            // Machine-readable defaults for region planners: one C++ resolver
            // owns schema validation and default values for all consumers.
            auto resolved = profile;
            resolved.activation = {settings, light_set, environment};
            resolved.sparse_spacing = spacing;
            resolved.map_resolution = resolution;
            std::cout << describe_level_bake_profile(resolved);
            return 0;
        }

        if (!compare_coarse.empty() && verify.empty()) {
            throw std::runtime_error("--compare-coarse requires --verify LOCAL_FILE");
        }

        if (!output.empty()
            && (fs::exists(output) || fs::exists(output.string() + ".txt")
                || fs::exists(output.string() + ".tmp"))) {
            throw std::runtime_error("output exists; choose a new bake path");
        }

        json::Value manifest;
        std::string error;

        if (!json::parse_file((directory / "manifest.json").string(), manifest, error)) {
            throw std::runtime_error(error);
        }

        const std::string game = manifest["game"].string(), version = manifest["version"].string();

        if (!bake_identity_supported(game, version)) {
            throw std::runtime_error("expected a RAC1 PAL or NTSC export or a RAC2 NTSC one");
        }

        scene::LevelData level;

        if (game_rounding) {
            std::fesetround(FE_TOWARDZERO);
        }

        std::cout << "Loading extracted level " << directory.string() << std::endl;

        const auto edited = overrides.empty() ? fs::path{} : overrides / "placements.json";
        if (!scene::load_level(directory, level, error, edited)) {
            throw std::runtime_error(error);
        }

        if (level.level < 0) {
            throw std::runtime_error("level identity missing");
        }

        if (!verify.empty()) {
            ProbeActivation expected;
            expected.bake = settings;
            expected.light_set = light_set;
            expected.environment = environment;
            ProbeAssetLoader loader(level, expected);
            const auto volume = loader.load_with_previous_defaults(verify, profile, true);
            std::cout << "Probe asset verified: " << volume.grid.game_version << " level "
                      << volume.grid.level << ", " << volume.probes.size()
                      << " probes; scene and settings match\n";

            if (compare_coarse.empty()) {
                report_terrain_coverage(level, volume, nullptr, std::cout);
            } else {
                const auto coarse = loader.load(compare_coarse, true);
                validate_probe_pair(coarse, volume);
                report_terrain_coverage(level, volume, &coarse, std::cout);
            }

            return 0;
        }

        const auto start = std::chrono::steady_clock::now();
        auto exported = export_probe_scene(level, light_set, environment);
        bake_detail::EmissionAverageCache emission_cache(exported.scene);
        // The source sidecar is already part of the canonical scene fingerprint used at runtime.
        auto interior_report = apply_interior_sources(level, exported.scene, &emission_cache);

        if (!interior_report) {
            throw std::runtime_error(interior_report.error().message);
        }

        if (!interior_report->empty()) {
            std::cout << "interior_area_emitters " << exported.scene.area_emitters.size() << '\n';
        }

        ProbeGrid grid;

        // Exact lattice endpoints retain the planner's narrow blend bands at fractional spacing.
        if (bounds && lattice_bounds) {
            grid = lattice_probe_grid(exported, *bounds, spacing, resolution);
        } else if (bounds) {
            grid = bounded_probe_grid(
                exported,
                {(*bounds)[0], (*bounds)[1], (*bounds)[2]},
                {(*bounds)[3], (*bounds)[4], (*bounds)[5]},
                spacing,
                resolution
            );
        } else {
            grid = sparse_probe_grid(exported, spacing, resolution);
        }

        if (max_distance) {
            grid.max_distance = *max_distance;
            grid.validate();
        }

        grid.game_version = game + "-" + version;
        grid.level = static_cast<std::uint32_t>(level.level);
        grid.settings_hash = probe_bake_settings_hash(grid, settings);
        std::ostringstream report;
        report << describe_probe_bake(level, exported, grid, settings, light_set);
        std::cout << report.str() << std::flush;

        if (inspect) {
            return 0;
        }

        // Free the loader's copies of meshes/textures before building the BVH.
        level = scene::LevelData{};
        auto last = start;
        const auto volume =
            ProbeBaker(exported.scene, settings, 0, nullptr, &emission_cache)
                .bake(
                    grid,
                    [&](std::size_t done, std::size_t total) {
                        const auto now = std::chrono::steady_clock::now();

                        if (done == 0 || done == total || now - last >= std::chrono::seconds(5)) {
                            std::cout << "Baked " << done << '/' << total << " probes\n"
                                      << std::flush;
                            last = now;
                        }
                    },
                    threads
                );
        const auto encoded = encode_baked_probes(volume, settings.texture_format, {threads});
        // Check the asset that will be used by the renderer before publishing.
        const auto decoded = decode_gpu_probes(encoded);

        if (!probe_identity_matches(
                decoded, grid.game_version, grid.level, grid.scene_hash, grid.settings_hash
            )) {
            throw std::runtime_error("probe asset round-trip identity mismatch");
        }

        const auto valid =
            std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& p) {
                return p.valid;
            });
        const auto relocated =
            std::count_if(volume.probes.begin(), volume.probes.end(), [](const Probe& p) {
                return p.relocated;
            });
        const auto seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        report << "relocated_probes " << relocated << "\nvalid_probes " << valid
               << "\ninvalid_probes " << volume.probes.size() - static_cast<std::size_t>(valid)
               << "\nbake_seconds " << seconds << "\nasset_bytes " << encoded.size() << '\n';
        write_bake_file(output, encoded);
        const auto text = report.str();
        write_bake_file(
            output.string() + ".txt",
            std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size())
        );
        std::cout << "Saved " << output.string() << " (" << valid << '/' << volume.probes.size()
                  << " valid probes, " << seconds << " s)\n";
        return 0;
    } catch (const NothingToBake& nothing) {
        std::cerr << "probe bake: " << nothing.what() << '\n';
        return kNothingToBake;
    } catch (const std::bad_alloc&) {
        std::cerr << "probe bake: out of memory; close other large apps and retry; use --jobs 1 "
                     "for command-line baking\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "probe bake: " << error.what() << '\n';
        return 1;
    }
}
