// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// ASTC HDR search uses Arm's pinned symbolic codec, with paired-border constraints in linear light.
#include "enhanced/lighting/probe_astc.h"

#include <algorithm>
#include <astcenc_internal.h>
#include <cmath>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>

#include "enhanced/lighting/probe_astc_constraints.h"

namespace openrac::lighting::compression {
namespace {
// Maximum paired-border error in linear RGB, relative to source luminance.
constexpr double kSeamRelativeTolerance = .005;

// Evaluates the pinned HDR endpoint and interpolated-weight model for physical-block checks.
Tile symbolic_pixels(const block_size_descriptor& bsd, const symbolic_compressed_block& scb) {
    const auto& mode = bsd.get_block_mode(scb.block_mode);
    const auto& di = bsd.get_decimation_info(mode.decimation_mode);
    bool hdr = false, alpha = false;
    vint4 a, b;
    unpack_color_endpoints(
        ASTCENC_PRF_HDR_RGB_LDR_A, scb.color_formats[0], scb.color_values[0], hdr, alpha, a, b
    );
    std::array<Float3, 65> colours;

    for (int w = 0; w <= 64; w++) {
        auto value = asr<6>(a * vint4(64 - w) + b * vint4(w) + vint4(32));
        auto decoded = float16_to_float(hdr ? lns_to_sf16(value) : unorm16_to_sf16(value));
        colours[w] = {decoded.lane<0>(), decoded.lane<1>(), decoded.lane<2>()};
    }

    Tile out;

    for (unsigned p = 0; p < 64; p++) {
        int weight = 8;

        for (unsigned i = 0; i < di.texel_weight_count[p]; i++) {
            weight +=
                di.texel_weight_contribs_int_tr[i][p] * scb.weights[di.texel_weights_tr[i][p]];
        }

        out[p] = colours[weight >> 4];
    }

    return out;
}

// Exact paired equality supplies the first, zero-tolerance fitting pass.
bool seams_match(const Tile& output) {
    for (unsigned p = 0; p < 64; p++) {
        if (output[p] != output[gutter_source(p)]) {
            return false;
        }
    }

    return true;
}

// Measures paired error in decoded linear light against a tile-local luminance scale.
struct SeamBudget {
    std::array<double, 64> scale{};
    static constexpr double kBlackFloor = 0.0001;

    // Probe colours use the same fifth-power decode as runtime GI sampling.
    static double linear(float encoded) {
        const double v = std::max(0.0, double(encoded));
        const double square = v * v;
        return square * square * v;
    }

    // Each source interior texel supplies the shared scale for its border pair.
    void set(const Tile& input) {
        for (unsigned p = 0; p < 64; p++) {
            const auto& value = input[gutter_source(p)];
            const double luminance =
                .2126 * linear(value[0]) + .7152 * linear(value[1]) + .0722 * linear(value[2]);
            scale[p] = std::max(kBlackFloor, luminance);
        }
    }

    // One bright channel cannot hide a mismatch in another colour channel.
    double pair_ratio(const Tile& output, unsigned p) const {
        double value = 0;

        for (unsigned c = 0; c < 3; c++) {
            const double a = linear(output[p][c]), b = linear(output[gutter_source(p)][c]);

            if (!std::isfinite(a) || !std::isfinite(b)) {
                return INFINITY;
            }

            value = std::max(value, std::abs(a - b) / scale[p]);
        }

        return value;
    }

    // Exact and relaxed passes share the same paired-texel addressing.
    bool accepts(const Tile& output, double tolerance) const {
        if (tolerance == 0) {
            return seams_match(output);
        }

        for (unsigned p = 0; p < 64; p++) {
            if (gutter_source(p) != p && pair_ratio(output, p) > tolerance) {
                return false;
            }
        }

        return true;
    }
};

// Worker-owned fitting state and descriptor-derived interpolation moves.
struct AstcFitter {
    double seam_relative_tolerance =
        kSeamRelativeTolerance;       // Current exact or relaxed fitting pass.
    AstcInterpolation interpolation;  // Immutable descriptor-derived moves, reused across tiles.
    SeamBudget active_seam_budget;    // Derived from this worker's current tile, never shared.

    // Retains source texel order when comparing decoded candidate colour errors.
    float tile_error(const Tile& input, const Tile& output) {
        float result = 0;

        for (unsigned p = 0; p < 64; p++) {
            result += squared_colour_error(input[p], output[p]);
        }

        return result;
    }

    // Endpoint values must be repacked by astcenc's HDR quantizer, not treated as ordinary colour bytes.
    bool pack_hdr_endpoints(
        symbolic_compressed_block& scb, const block_mode& mode, vfloat4 a, vfloat4 b
    ) {
        const auto quant = quant_mode_table[3][111 - mode.weight_bits];

        if (quant < QUANT_6) {
            return false;
        }

        scb.quant_mode = static_cast<quant_method>(quant);
        scb.color_formats[0] = pack_color_endpoints(
            a, b, vfloat4::zero(), vfloat4::zero(), FMT_HDR_RGB, scb.color_values[0], scb.quant_mode
        );
        return true;
    }

    // Least-squares endpoint fitting in the HDR interpolation domain, evaluated using decoded colour error.
    symbolic_compressed_block fitted_endpoints(
        const Tile& input, const block_size_descriptor& bsd, symbolic_compressed_block scb
    ) {
        const auto& mode = bsd.get_block_mode(scb.block_mode);
        const auto& di = bsd.get_decimation_info(mode.decimation_mode);
        double aa = 0, ab = 0, bb = 0;
        std::array<double, 3> ay{}, by{};

        for (unsigned p = 0; p < 64; p++) {
            int w = 8;

            for (unsigned i = 0; i < di.texel_weight_count[p]; i++) {
                w += di.texel_weight_contribs_int_tr[i][p] * scb.weights[di.texel_weights_tr[i][p]];
            }

            const double t = double(w >> 4) / 64;
            auto value = float_to_lns(vfloat4(input[p][0], input[p][1], input[p][2], 1));
            const Float3 target{value.lane<0>(), value.lane<1>(), value.lane<2>()};
            aa += (1 - t) * (1 - t);
            ab += (1 - t) * t;
            bb += t * t;

            for (unsigned c = 0; c < 3; c++) {
                ay[c] += (1 - t) * target[c];
                by[c] += t * target[c];
            }
        }

        const double determinant = aa * bb - ab * ab;

        if (determinant < 1e-8) {
            return scb;
        }

        Float3 a{}, b{};

        for (unsigned c = 0; c < 3; c++) {
            a[c] = float(std::clamp((ay[c] * bb - by[c] * ab) / determinant, 0.0, 65535.0));
            b[c] = float(std::clamp((by[c] * aa - ay[c] * ab) / determinant, 0.0, 65535.0));
        }

        pack_hdr_endpoints(
            scb, mode, vfloat4(a[0], a[1], a[2], 65535), vfloat4(b[0], b[1], b[2], 65535)
        );
        return scb;
    }

    // Keeps one complete symbolic candidate beside its physical prediction.
    struct AstcFit {
        symbolic_compressed_block block{};  // Candidate endpoint and weight state.
        Tile output{};                      // Decoded prediction checked before publication.
        float error = INFINITY;  // Unset candidates cannot win a strict error comparison.
    };

    // Accept finite colours within the border budget; lower error never overrides the seam constraint.
    void consider(
        AstcFit& best,
        const Tile& input,
        const block_size_descriptor& bsd,
        const symbolic_compressed_block& candidate
    ) {
        const auto output = symbolic_pixels(bsd, candidate);

        if (!active_seam_budget.accepts(output, seam_relative_tolerance)) {
            return;
        }

        for (const auto& p : output) {
            for (float c : p) {
                if (!std::isfinite(c)) {
                    return;
                }
            }
        }

        const float value = tile_error(input, output);

        if (value < best.error) {
            best.block = candidate;
            best.output = output;
            best.error = value;
        }
    }

    using Groups = std::array<unsigned, 64>;

    // Border contributors share a conservative initial weight group before null-space refinement.
    unsigned group_root(const Groups& parent, unsigned index) {
        while (parent[index] != index) {
            index = parent[index];
        }

        return index;
    }

    // Union each gutter's interpolation contributors with those of its paired interior texel.
    Groups weight_groups(const decimation_info& di) {
        Groups parent{};
        std::iota(parent.begin(), parent.end(), 0);

        for (unsigned pixel = 0; pixel < 64; ++pixel) {
            // Interior texels have no additional border constraint.
            if (gutter_source(pixel) == pixel) {
                continue;
            }

            const auto first = di.texel_weights_tr[0][pixel];

            for (unsigned paired : {pixel, gutter_source(pixel)}) {
                for (unsigned i = 0; i < di.texel_weight_count[paired]; ++i) {
                    parent[group_root(parent, di.texel_weights_tr[i][paired])] =
                        group_root(parent, first);
                }
            }
        }

        return parent;
    }

    // Repack the stock endpoint colours into the candidate mode's HDR endpoint bit budget.
    std::optional<symbolic_compressed_block> initial_block(
        const symbolic_compressed_block& original, const block_mode& mode
    ) {
        auto block = original;
        block.block_mode = mode.mode_index;
        block.partition_count = 1;
        block.plane2_component = -1;
        bool hdr = false, alpha = false;
        vint4 low, high;
        unpack_color_endpoints(
            ASTCENC_PRF_HDR_RGB_LDR_A,
            original.color_formats[0],
            original.color_values[0],
            hdr,
            alpha,
            low,
            high
        );
        auto start = int_to_float(low), end = int_to_float(high);

        // LDR endpoints first need conversion to the HDR interpolation domain.
        if (!hdr) {
            start = float_to_lns(start / vfloat4(65535));
            end = float_to_lns(end / vfloat4(65535));
        }

        // Modes without an endpoint budget cannot describe this candidate.
        if (!pack_hdr_endpoints(block, mode, start, end)) {
            return std::nullopt;
        }

        const unsigned levels = get_quant_level(mode.get_weight_quant_mode());
        const auto& transfer = quant_and_xfer_tables[mode.quant_mode];
        std::fill(
            std::begin(block.weights),
            std::end(block.weights),
            transfer.quant_to_unquant[levels / 2]
        );
        return block;
    }

    // Immutable references keep each weight search tied to the same tile and ASTC mode.
    struct FitContext {
        const Tile& input;
        const block_size_descriptor& descriptor;
        const decimation_info& decimation;
        const quant_and_transfer_table& transfer;
        unsigned levels;
        Groups groups;
    };

    // Every stored weight in this conservative union moves together before null-space refinement.
    void set_group(
        symbolic_compressed_block& block,
        unsigned group,
        std::uint8_t value,
        const FitContext& context
    ) {
        for (unsigned i = 0; i < context.decimation.weight_count; ++i) {
            // Contributors from other groups retain their current weights.
            if (group_root(context.groups, i) == group) {
                block.weights[i] = value;
            }
        }
    }

    // Grouped moves provide a valid starting point even for tightly constrained 8x8 modes.
    void search_groups(AstcFit& best, const FitContext& context) {
        for (unsigned group = 0; group < context.decimation.weight_count; ++group) {
            // Each union is visited once at its representative.
            if (group_root(context.groups, group) != group) {
                continue;
            }

            for (unsigned value = 0; value < context.levels; ++value) {
                auto candidate = best.block;

                set_group(candidate, group, context.transfer.quant_to_unquant[value], context);

                consider(best, context.input, context.descriptor, candidate);
            }
        }
    }

    // Reject a null-space move if any resulting weight is outside the mode's quantization table.
    bool shift_weights(
        symbolic_compressed_block& candidate,
        const AstcWeightMove& action,
        int step,
        const FitContext& context
    ) {
        for (unsigned i = 0; i < context.decimation.weight_count; ++i) {
            // Zero coefficients leave their weights unchanged.
            if (!action[i]) {
                continue;
            }

            const int next = static_cast<int>(candidate.weights[i]) + step * action[i];
            bool found = false;

            for (unsigned q = 0; q < context.levels; ++q) {
                found = found || static_cast<int>(context.transfer.quant_to_unquant[q]) == next;
            }

            // Clamping would break the null-space constraint.
            if (!found) {
                return false;
            }

            candidate.weights[i] = static_cast<std::uint8_t>(next);
        }

        return true;
    }

    // Search integer null-space directions while retaining the exact interpolation constraints.
    void search_interpolation(AstcFit& best, const FitContext& context) {
        for (const auto& action : interpolation.moves(context.decimation)) {
            for (int step = -64; step <= 64; ++step) {
                auto candidate = best.block;

                // Only representable moves can be tested by the physical decoder model.
                if (shift_weights(candidate, action, step, context)) {
                    consider(best, context.input, context.descriptor, candidate);
                }
            }
        }
    }

    // Individual moves exploit equal decoded half floats within the permitted linear-light seam budget.
    void search_individual(AstcFit& best, const FitContext& context) {
        for (unsigned i = 0; i < context.decimation.weight_count; ++i) {
            for (unsigned q = 0; q < context.levels; ++q) {
                auto candidate = best.block;
                candidate.weights[i] = context.transfer.quant_to_unquant[q];
                consider(best, context.input, context.descriptor, candidate);
            }
        }
    }

    // Alternate legal weight moves and least-squares HDR endpoint refits in stable order.
    AstcFit fit_astc(
        const Tile& input,
        const block_size_descriptor& descriptor,
        const symbolic_compressed_block& original,
        const block_mode& mode,
        const AstcFit* seed = nullptr
    ) {
        AstcFit best;

        // Relaxed passes start from the valid result of the exact pass.
        if (seed && std::isfinite(seed->error)) {
            best = *seed;
        }

        const auto initial = initial_block(original, mode);

        // Modes without enough endpoint bits cannot improve the result.
        if (!initial) {
            return best;
        }

        consider(best, input, descriptor, *initial);
        const auto& decimation = descriptor.get_decimation_info(mode.decimation_mode);
        const FitContext context{
            input,
            descriptor,
            decimation,
            quant_and_xfer_tables[mode.quant_mode],
            get_quant_level(mode.get_weight_quant_mode()),
            weight_groups(decimation)
        };

        for (unsigned pass = 0; pass < 3; ++pass) {
            search_groups(best, context);
            search_interpolation(best, context);
            search_individual(best, context);
            consider(best, input, descriptor, fitted_endpoints(input, descriptor, best.block));
        }

        return best;
    }
};
}  // namespace

// Owns the upstream codec and its immutable block descriptor until the worker exits.
struct AstcEncoder::Impl : AstcFitter {
    std::unique_ptr<astcenc_context, decltype(&astcenc_context_free)> ctx{
        nullptr, astcenc_context_free
    };
    std::unique_ptr<block_size_descriptor> bsd = std::make_unique<block_size_descriptor>();
    unsigned full_mode = 0;

    // Fixes the portable HDR search settings and finds the exact binary fallback grid.
    Impl() {
        astcenc_config config;

        if (astcenc_config_init(
                ASTCENC_PRF_HDR_RGB_LDR_A, 8, 8, 1, ASTCENC_PRE_MEDIUM, 0, &config
            )) {
            throw std::runtime_error("cannot configure ASTC HDR encoder");
        }

        config.tune_partition_count_limit = 1;
        astcenc_context* allocated = nullptr;
        const auto allocated_status = astcenc_context_alloc(&config, 1, &allocated, nullptr);
        ctx.reset(allocated);

        if (allocated_status) {
            throw std::runtime_error("cannot allocate ASTC HDR encoder");
        }

        init_block_size_descriptor(8, 8, 1, false, 1, 1, *bsd);
        bool found = false;

        for (unsigned m = 0; m < bsd->block_mode_count_all; m++) {
            const auto& mode = bsd->block_modes[m];
            auto& di = bsd->get_decimation_info(mode.decimation_mode);

            if (!mode.is_dual_plane && di.weight_x == 8 && di.weight_y == 8) {
                full_mode = mode.mode_index;
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("no full ASTC weight grid");
        }
    }

    // Uses the upstream decoder to validate the physical bytes selected by custom fitting.
    Tile decode(Block block) {
        std::array<float, 256> pixels{};
        void* slices[]{pixels.data()};
        astcenc_image image{8, 8, 1, ASTCENC_TYPE_F32, slices};
        astcenc_swizzle sw{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1};

        if (astcenc_decompress_image(ctx.get(), block.data(), 16, &image, &sw, 0)) {
            throw std::runtime_error("cannot decode ASTC HDR block");
        }

        Tile output;

        for (unsigned p = 0; p < 64; p++) {
            for (unsigned c = 0; c < 3; c++) {
                output[p][c] = pixels[p * 4 + c];
            }
        }

        return output;
    }

    // Standard astcenc is retained whenever its physical block already satisfies the border budget.
    Block stock_block(const Tile& input) {
        std::array<float, 256> pixels{};

        for (unsigned pixel = 0; pixel < 64; ++pixel) {
            std::copy(
                input[pixel].begin(), input[pixel].end(), pixels.begin() + std::size_t(pixel) * 4
            );
            pixels[pixel * 4 + 3] = 1;
        }

        void* slices[]{pixels.data()};
        astcenc_image image{8, 8, 1, ASTCENC_TYPE_F32, slices};
        const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1};
        Block block{};
        astcenc_compress_reset(ctx.get());

        // Codec failures must not publish incomplete physical blocks.
        if (astcenc_compress_image(ctx.get(), &image, &swizzle, block.data(), 16, 0)) {
            throw std::runtime_error("ASTC HDR compression failed");
        }

        return block;
    }

    // A full per-texel binary grid always has a valid paired-index fallback.
    Block binary_seed(const Tile& input, symbolic_compressed_block block) {
        block.block_mode = static_cast<std::uint16_t>(full_mode);
        block.partition_count = 1;
        block.plane2_component = -1;
        const auto& mode = bsd->get_block_mode(full_mode);
        const unsigned count = ((block.color_formats[0] >> 2) + 1) * 2;
        block.quant_mode =
            static_cast<quant_method>(quant_mode_table[count / 2][111 - mode.weight_bits]);

        // Even the binary fallback needs legal endpoint quantization.
        if (block.quant_mode < QUANT_6) {
            throw std::runtime_error("ASTC HDR endpoint budget is too small");
        }

        for (unsigned channel = 0; channel < count; ++channel) {
            block.color_values[0][channel] =
                color_unquant_to_uquant_tables[block.quant_mode - QUANT_6]
                                              [std::size_t{2} * block.color_values[0][channel]];
        }

        Block physical{};
        std::fill(std::begin(block.weights), std::end(block.weights), 0);
        symbolic_to_physical(*bsd, block, physical.data());
        const auto low = decode(physical)[0];
        std::fill(std::begin(block.weights), std::end(block.weights), 64);
        symbolic_to_physical(*bsd, block, physical.data());
        const auto high = decode(physical)[0];

        for (unsigned pixel = 0; pixel < 64; ++pixel) {
            const auto& colour = input[gutter_source(pixel)];
            // Paired texels choose the same endpoint, regardless of their block position.
            block.weights[pixel] =
                squared_colour_error(colour, low) <= squared_colour_error(colour, high) ? 0 : 64;
        }

        symbolic_to_physical(*bsd, block, physical.data());
        return physical;
    }

    // Limit constrained search to practical single-plane grids and representable weight alphabets.
    bool supported_mode(const block_mode& mode) const {
        const auto& grid = bsd->get_decimation_info(mode.decimation_mode);
        const unsigned levels = get_quant_level(mode.get_weight_quant_mode());
        return !mode.is_dual_plane
               && (grid.weight_x == 4 || grid.weight_x == 6 || grid.weight_x == 8)
               && (grid.weight_y == 4 || grid.weight_y == 6 || grid.weight_y == 8)
               && (levels == 2 || levels == 4 || levels == 8 || levels == 16);
    }

    // Compare actual packed decoding with the search model before accepting an improvement.
    Block checked_block(const AstcFit& fit) {
        Block physical{};
        symbolic_to_physical(*bsd, fit.block, physical.data());
        const auto output = decode(physical);

        // Internal fitting must agree with the pinned upstream physical decoder.
        if (output != fit.output) {
            throw std::runtime_error("symbolic and upstream ASTC decoders disagree");
        }

        // Lower colour error never overrides the paired-border constraint.
        if (!active_seam_budget.accepts(output, seam_relative_tolerance)) {
            throw std::runtime_error("packed ASTC border exceeds the 0.5% linear RGB tolerance");
        }

        return physical;
    }

    // Refine exact interpolation constraints before allowing the agreed 0.5% physical colour tolerance.
    Block constrained_block(const Tile& input, const symbolic_compressed_block& original) {
        auto best_block = binary_seed(input, original);
        float best = tile_error(input, decode(best_block));

        for (unsigned index = 0; index < bsd->block_mode_count_all; ++index) {
            const auto& mode = bsd->block_modes[index];

            // Other modes either duplicate this search or cannot retain useful interior detail.
            if (!supported_mode(mode)) {
                continue;
            }

            seam_relative_tolerance = 0;
            auto improved = fit_astc(input, *bsd, original, mode);
            seam_relative_tolerance = kSeamRelativeTolerance;
            improved = fit_astc(input, *bsd, original, mode, &improved);

            // Stable mode order resolves equal-error results deterministically.
            if (improved.error < best) {
                best_block = checked_block(improved);
                best = improved.error;
            }
        }

        // The binary fallback must satisfy the same physical-block check as fitted candidates.
        if (!active_seam_budget.accepts(decode(best_block), seam_relative_tolerance)) {
            throw std::runtime_error("ASTC probe border exceeds the 0.5% linear RGB tolerance");
        }

        return best_block;
    }

    // Ordinary reflections use stock blocks; only GI requires octahedral border constraints.
    Block run(const Tile& input, bool probe_gutters) {
        // A preceding failed fit must not leave a reused worker in its temporary exact-only pass.
        seam_relative_tolerance = kSeamRelativeTolerance;
        active_seam_budget.set(input);
        const auto block = stock_block(input);

        // Standard encoding is sufficient when its physical gutter pairs already meet the budget.
        if (!probe_gutters || active_seam_budget.accepts(decode(block), seam_relative_tolerance)) {
            return block;
        }

        symbolic_compressed_block original{};
        physical_to_symbolic(*bsd, block.data(), original);

        // Constant physical blocks necessarily have equal pairs; any other type is malformed.
        if (original.block_type != SYM_BTYPE_NONCONST) {
            throw std::runtime_error("invalid stock ASTC block during constrained encoding");
        }

        return constrained_block(input, original);
    }
};

// Each worker retains its own mutable codec tables and fitting cache.
AstcEncoder::AstcEncoder() : m_impl(std::make_unique<Impl>()) {}

// Releases codec descriptors and cached moves with their owning worker.
AstcEncoder::~AstcEncoder() = default;

// Applies paired borders only to GI; reflection blocks retain the ordinary codec path.
Block AstcEncoder::encode(const Tile& tile, bool probe_gutters) {
    return m_impl->run(tile, probe_gutters);
}
}  // namespace openrac::lighting::compression
