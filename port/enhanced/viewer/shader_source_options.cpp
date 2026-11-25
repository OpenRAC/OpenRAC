// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Maps runtime renderer choices into the context-free shader assembly options.

#include "common/stats.h"
#include "enhanced/viewer/shader_sources.h"
#include "renderer/gl/gl.h"
#include "renderer/settings/render_opt.h"

namespace openrac::lighting::shader_sources {
Options runtime_options() {
    Options out;
    out.enhanced = gl::enhanced_rendering_available();
    out.vulkan = gl::is_vulkan();
    out.water_local = renderer::render_opt::enabled(renderer::render_opt::Opt::WaterLocalShader);
    out.gather = !stats::probe(
        "OPENRAC_SHADOW_GATHER_OFF", "Reference: individual shadow fetches (reload level)"
    );
    out.filter_reference =
        stats::probe("OPENRAC_SHADOW_FILTER_REFERENCE", "Reference: nine bilinear shadow taps");
    out.zero_weight_reference = stats::probe(
        "OPENRAC_PROBE_ZERO_WEIGHT_REFERENCE", "Reference: fetch zero-weight probe corners"
    );
    return out;
}
}  // namespace openrac::lighting::shader_sources
