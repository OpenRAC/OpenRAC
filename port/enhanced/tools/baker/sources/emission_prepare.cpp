// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// emission_prepare.h's generated-default cleanup, local override merging and atomic publication.
#include "enhanced/tools/baker/sources/emission_prepare.h"

#include <stdexcept>
#include <utility>

#include "common/profiler.h"
#include "enhanced/tools/baker/pipeline/bake_json.h"
#include "enhanced/tools/baker/pipeline/stage_file.h"
#include "enhanced/tools/baker/sources/emission_catalog.h"

namespace openrac::lighting {
namespace {
// Overlays one level of fields, preserving independent material properties on shared texture keys.
void overlay(bake_json::Object& target, const bake_json::Object& source) {
    // Level authoring overrides catalog fields individually, while scalar/list records replace whole values.
    for (const auto& [key, value] : source) {
        const auto found = target.find(key);

        // Object merging is shallow, matching the existing profile schema.
        if (found != target.end() && found->second.is_object() && value.is_object()) {
            auto fields = found->second.members();

            // Source fields win without dropping unrelated target settings.
            for (const auto& [name, field] : value.members()) {
                fields.insert_or_assign(name, field);
            }

            found->second = bake_json::object_value(fields);
        } else {
            target.insert_or_assign(key, value);
        }
    }
}

// Retains edited or independently authored properties while removing unchanged generated fields.
json::Value remove_generated_fields(const json::Value& current, const json::Value& generated) {
    auto fields = current.members();

    // The old generated value supplies evidence for removal, never a brightness heuristic.
    for (const auto& [name, value] : generated.members()) {
        const auto found = fields.find(name);

        // Local changes must survive updates and removals of shared catalog settings.
        if (found != fields.end() && bake_json::equivalent(found->second, value)) {
            fields.erase(found);
        }
    }

    return bake_json::object_value(fields);
}

// Removes only fields still equal to their last generated value, retaining user edits and added keys.
void remove_generated(bake_json::Object& existing, const bake_json::Object& previous) {
    // Removed catalog entries must not leave stale generated emitters behind.
    for (const auto& [key, generated] : previous) {
        const auto found = existing.find(key);

        // Missing keys already contain no generated data to remove.
        if (found == existing.end()) {
            continue;
        }

        // Material-level edits retain each independent property.
        if (found->second.is_object() && generated.is_object()) {
            const auto reduced = remove_generated_fields(found->second, generated);

            // Empty generated material objects need no residual override.
            if (reduced.members().empty()) {
                existing.erase(found);
            } else {
                found->second = reduced;
            }
        } else if (bake_json::equivalent(found->second, generated)) {
            // Unchanged scalar/list defaults can be removed as whole records.
            existing.erase(found);
        }
    }
}
}  // namespace

Result<bool> prepare_level_emission(
    const std::filesystem::path& level,
    const std::filesystem::path& catalog,
    const std::filesystem::path& defaults,
    const BakeExecution& execution
) {
    // Reject malformed configuration before replacing any generated profile.
    try {
        const profiling::Zone zone("Prepare emission profile");
        auto resolved = resolve_emission_catalog(level, catalog, execution);
        overlay(resolved, bake_json::optional_object(defaults));
        const auto path = level / "probe-emission.json";
        const auto original = bake_json::optional_object(path);
        auto existing = original;
        bake_json::Object previous;

        // Previous generated values distinguish catalog refreshes from independent local authoring.
        if (const auto found = existing.find("_catalog_defaults"); found != existing.end()) {
            // Invalid provenance cannot safely identify generated fields for removal.
            if (!found->second.is_object()) {
                throw std::runtime_error("generated emission defaults must be an object");
            }

            previous = found->second.members();
            existing.erase(found);
        }

        remove_generated(existing, previous);
        auto combined = resolved;
        overlay(combined, existing);

        // Empty catalogs do not add a provenance record to otherwise local-only authoring.
        if (!resolved.empty()) {
            combined.insert_or_assign("_catalog_defaults", bake_json::object_value(resolved));
        }

        // Absent profiles stay absent when there are no generated or authored settings.
        if (combined.empty() && previous.empty()) {
            return false;
        }

        const auto value = bake_json::object_value(combined);

        // Equivalent existing JSON keeps its exact bytes, fingerprint and modification time.
        if (std::filesystem::exists(path)
            && bake_json::equivalent(value, bake_json::object_value(original))) {
            return false;
        }

        const auto written = write_stage_file(path, bake_json::json_text(value) + '\n');

        // Publication failures leave the previous readable profile intact.
        if (!written) {
            return unexpected(written.error());
        }

        return true;
    } catch (const std::exception& error) {
        return unexpected(Error{error.what()});
    }
}
}  // namespace openrac::lighting
