// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Emission catalog validation and bounded PNG matching for emission_prepare.h.
#include "enhanced/tools/baker/sources/emission_catalog.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include "common/profiler.h"
#include "enhanced/tools/baker/sources/interior_inputs.h"

namespace openrac::lighting {
namespace {
// Rejects malformed catalog controls before any generated profile can be replaced.
void validate_setting(const std::string& key, const json::Value& value) {
    // Vertex modulation is the catalog's only boolean material control.
    if (key == "vertex_colour") {
        // Numeric booleans would change the original profile's meaning.
        if (value.type() != json::Value::Type::Bool) {
            throw std::runtime_error("emission vertex_colour requires a boolean");
        }

        return;
    }

    // Catalog keys are deliberately narrower than independent level authoring.
    if (key != "contrast" && key != "runtime_intensity" && key != "bake_intensity"
        && key != "diffuse_scale") {
        throw std::runtime_error("unknown emission material setting: " + key);
    }

    // Invalid scalar values must not create negative or nonfinite transport.
    if (!value.is_number() || !std::isfinite(value.number()) || value.number() < 0) {
        throw std::runtime_error("invalid emission material intensity or diffuse scale");
    }

    // Contrast uses the runtime's supported one-to-four power range.
    if (key == "contrast" && (value.number() < 1 || value.number() > 4)) {
        throw std::runtime_error("emission contrast must be between one and four");
    }

    // Diffuse reflectance cannot amplify incoming irradiance.
    if (key == "diffuse_scale" && value.number() > 1) {
        throw std::runtime_error("invalid emission diffuse scale");
    }
}

// Retains the Python catalog's exact lowercase SHA-256 and duplicate-definition rules.
bake_json::Object definitions(const json::Value& catalog) {
    // A versioned list is required even when it contains no emitters.
    if (catalog["version"].number() != 1 || !catalog["materials"].is_array()) {
        throw std::runtime_error(
            "emission material catalog requires version 1 and a material list"
        );
    }

    bake_json::Object result;

    // Only explicit content identities qualify textures for emission.
    for (const auto& material : catalog["materials"].items()) {
        const auto& digest = material["texture_sha256"].string();

        // SHA-256 text has exactly 64 lowercase hexadecimal digits.
        if (!material.is_object() || !material["texture_sha256"].is_string() || digest.size() != 64
            || digest.find_first_not_of("0123456789abcdef") != digest.npos) {
            throw std::runtime_error("emission texture identity requires a SHA-256 digest");
        }

        // Names document reviewed materials; definitions are objects even with no overrides.
        if (!material["name"].is_string() || !material["emission"].is_object()) {
            throw std::runtime_error("emission material requires a name and emission object");
        }

        // Duplicate identities make catalog precedence ambiguous.
        if (!result.emplace(digest, material["emission"]).second) {
            throw std::runtime_error("duplicate emission texture identity");
        }

        // Validate every setting before hashing any source texture.
        for (const auto& [key, value] : material["emission"].members()) {
            validate_setting(key, value);
        }
    }

    return result;
}

// Enumerates only exported PNG files, in stable relative-path order.
std::vector<std::filesystem::path> textures(const std::filesystem::path& level) {
    const auto directory = level / "textures";

    // Levels without images have no content-matched emitters.
    if (!std::filesystem::exists(directory)) {
        return {};
    }

    std::vector<std::filesystem::path> result;

    // No recursion is needed for the export's flat shared-texture directory.
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        // Non-image sidecars and directories must not enter the material catalog.
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            result.push_back(entry.path());
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

// Hashes independent files with one fixed streaming buffer per active worker and stable result slots.
std::vector<std::string> texture_identities(
    const std::vector<std::filesystem::path>& paths, const BakeExecution& execution
) {
    const profiling::Zone timing("Hash emission textures");
    std::vector<std::string> identities(paths.size());

    // Empty texture directories need neither a pool nor any hashing tasks.
    if (paths.empty()) {
        return identities;
    }

    std::unique_ptr<BakeThreadPool> local;

    // Automatic bakes borrow their admitted process-wide worker budget.
    if (!execution.pool) {
        local = std::make_unique<BakeThreadPool>(execution.threads);
    }

    auto& pool = execution.pool ? *execution.pool : *local;
    // Explicit stage limits may reduce, but never exceed, the shared pool's budget.
    const unsigned budget =
        execution.threads ? std::min(execution.threads, pool.size()) : pool.size();
    const unsigned tasks = static_cast<unsigned>(std::min<std::size_t>(budget, paths.size()));
    std::atomic<std::size_t> next{0};

    // Each worker owns a distinct result slot; run() drains before ordered catalog assembly.
    const auto result = pool.run(tasks, [&] {
        for (auto index = next.fetch_add(1); index < paths.size(); index = next.fetch_add(1)) {
            const auto identity = bake_source_identity(paths[index]);

            // Missing or partially read PNGs must not acquire a valid emission identity.
            if (!identity) {
                throw std::runtime_error(identity.error().message);
            }

            identities[index] = *identity;
        }
    });

    // All borrowed paths and result slots remain alive until even failed tasks drain.
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    return identities;
}
}  // namespace

bake_json::Object resolve_emission_catalog(
    const std::filesystem::path& level,
    const std::filesystem::path& catalog,
    const BakeExecution& execution
) {
    const profiling::Zone zone("Resolve emission catalog");

    // An absent game catalog leaves explicit level definitions authoritative.
    if (!std::filesystem::exists(catalog)) {
        return {};
    }

    const auto entries = definitions(bake_json::document(catalog));
    bake_json::Object result;

    const auto paths = textures(level);
    const auto identities = texture_identities(paths, execution);

    // Sorted file order keeps matching and override publication independent of worker completion order.
    for (std::size_t index = 0; index < paths.size(); ++index) {
        const auto& path = paths[index];
        const auto found = entries.find(identities[index]);

        // Unmatched bright textures remain ordinary materials.
        if (found != entries.end()) {
            result.emplace(path.lexically_relative(level).generic_string(), found->second);
        }
    }

    return result;
}
}  // namespace openrac::lighting
