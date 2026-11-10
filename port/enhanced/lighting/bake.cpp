// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Cached directions, probe integration and worker lifetime for offline level baking.
#include "enhanced/lighting/bake.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cfenv>
#include <condition_variable>
#include <exception>
#include <map>
#include <mutex>
#include <span>
#include <thread>

#include "common/env.h"
#include "common/log.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_alignment.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/lighting/bake_workers.h"
#if OPENRAC_GPU_BAKE
#include "enhanced/lighting/gpu_transport.h"
#endif

namespace openrac::lighting {
namespace {
using bake_detail::add;
using bake_detail::dot;
using bake_detail::kPi;
using bake_detail::mul;
using bake_detail::Random;
using bake_detail::Scene;

// Tracing and explicit emission feed the same portable integration records.
using bake_detail::EmissionSample;
using bake_detail::ProbeLightDistribution;
using bake_detail::RaySample;

constexpr unsigned kMinAdaptiveRays = 64;
constexpr unsigned kMaxAdaptiveRays = 512;

// Sobol dimensions 1 and 2. Every aligned power-of-two block is stratified;
// a per-probe digital shift avoids using the same directions in nearby probes.
constexpr auto kAdaptiveSobol = [] {
    std::array<std::array<std::uint32_t, 2>, kMaxAdaptiveRays> points{};
    std::array<std::uint32_t, std::bit_width(kMaxAdaptiveRays - 1)> second{};
    second[0] = 0x80000000u;
    for (std::size_t i = 1; i < second.size(); ++i) {
        second[i] = second[i - 1] ^ (second[i - 1] >> 1);
    }
    for (unsigned i = 1; i < points.size(); ++i) {
        const auto bit = std::countr_zero(i);
        points[i] = {points[i - 1][0] ^ (0x80000000u >> bit), points[i - 1][1] ^ second[bit]};
    }
    return points;
}();

// Apply the probe's digital shift before mapping the Sobol point to a direction.
Float3 adaptive_direction(unsigned index, const std::array<std::uint32_t, 2>& shift) {
    const auto& point = kAdaptiveSobol[index];
    constexpr float kScale = 1.0f / 16777216.0f;
    const float u = static_cast<float>((point[0] ^ shift[0]) >> 8) * kScale;
    const float angle = static_cast<float>((point[1] ^ shift[1]) >> 8) * kScale * 2 * kPi;
    const auto z = 1 - 2 * u;
    const auto radius = std::sqrt(std::max(0.0f, 1 - z * z));
    return {radius * std::cos(angle), radius * std::sin(angle), z};
}

// A floor keeps near-black noise from consuming the whole lighting budget.
bool accumulate_lighting_error(Float3 a, Float3 b, float& sum) {
    bool converged = true;
    for (std::size_t c = 0; c < a.size(); ++c) {
        const auto error = std::abs(a[c] - b[c]) / (0.01f + 0.12f * std::max(a[c], b[c]));
        converged &= error <= 3;
        sum += error * error;
    }
    return converged;
}

// ---- Direction tables ----

// Generate the regular 2x2 footprint for reuse by all probes and volumes of this resolution.
std::vector<bake_detail::DirectionBlock> integration_directions(std::uint32_t resolution) {
    constexpr auto kLanes = bake_detail::kIntegrationLanes;
    const auto texels = std::size_t{resolution} * resolution;
    std::vector<bake_detail::DirectionBlock> directions((texels + kLanes - 1) / kLanes);

    for (std::uint32_t y = 0; y < resolution; ++y) {
        for (std::uint32_t x = 0; x < resolution; ++x) {
            const auto texel = std::size_t{y} * resolution + x;
            auto& footprint = directions[texel / kLanes];
            const auto lane = texel % kLanes;

            for (unsigned sample = 0; sample < bake_detail::kIntegrationDirections; ++sample) {
                const unsigned column = sample % kProbeTexelSamplesPerAxis;
                const unsigned row = sample / kProbeTexelSamplesPerAxis;
                const float u = (static_cast<float>(column) + 0.5f)
                                / static_cast<float>(kProbeTexelSamplesPerAxis);
                const float v = (static_cast<float>(row) + 0.5f)
                                / static_cast<float>(kProbeTexelSamplesPerAxis);
                const auto direction = oct_decode(
                    {(static_cast<float>(x) + u) / static_cast<float>(resolution),
                     (static_cast<float>(y) + v) / static_cast<float>(resolution)}
                );
                footprint.x[sample][lane] = direction[0];
                footprint.y[sample][lane] = direction[1];
                footprint.z[sample][lane] = direction[2];
            }
        }
    }

    return directions;
}

// Probe-independent part of a primary ray; azimuth rotation stays specific to each probe.
struct PrimaryDirection {
    float z, radius, angle;
};

// Small immutable tables shared across volumes with the same resolution and rounding mode.
struct BakeDirections {
    static constexpr unsigned kPlacementPairs = 32;  // paired occupancy rays over a hemisphere
    std::vector<PrimaryDirection> primary;
    std::array<std::array<Float3, 2>, kPlacementPairs> placement;
    std::vector<bake_detail::DirectionBlock> texels;

    BakeDirections(std::uint32_t rays, std::uint32_t resolution)
        : texels(integration_directions(resolution)) {
        primary.reserve(rays);

        for (std::uint32_t i = 0; i < rays; ++i) {
            const float z = 1 - 2 * (static_cast<float>(i) + 0.5f) / static_cast<float>(rays);
            primary.push_back(
                {z, std::sqrt(std::max(0.0f, 1 - z * z)), static_cast<float>(i) * 2.39996323f}
            );
        }

        for (unsigned i = 0; i < kPlacementPairs; ++i) {
            const float z = (static_cast<float>(i) + 0.5f) / kPlacementPairs;
            const float r = std::sqrt(1 - z * z);
            const float angle = static_cast<float>(i) * 2.39996323f;
            const Float3 direction{r * std::cos(angle), r * std::sin(angle), z};
            placement[i] = {mul(direction, 1.0f), mul(direction, -1.0f)};
        }
    }
};

// ---- Per-probe work ----

// Disjoint per-probe writes sharing immutable scene, settings and direction tables.
struct ProbeWork {
    const Scene& scene;                // Prepared scene outlives all workers.
    const BakeSettings& settings;      // Immutable bake controls.
    const BakeDirections& directions;  // Immutable cached tables.
    ProbeVolume& out;                  // Workers write only their claimed probe's storage.

    // Seeks a nearby geometric exit without moving a probe out of its nominal cell.
    void relocate_from_current(std::size_t probe) const {
        const auto& grid = out.grid;
        // Seek a nearby exit from geometric occupancy before baking. Keep
        // the nominal cell fixed and cap displacement to avoid moving a
        // probe into another cell. Unreachable placements remain invalid.
        const float relocation_limit =
            settings.relocation_max_fraction
            * *std::min_element(grid.spacing.begin(), grid.spacing.end());
        auto placement = classify(scene, out.position(probe), settings, directions.placement);

        // Stop relocation after the configured attempts or a candidate outside this cell.
        for (std::uint32_t iteration = 0;
             placement.inside && iteration < settings.relocation_iterations;
             ++iteration) {
            const auto candidate =
                add(out.probes[probe].offset,
                    mul(placement.exit_direction, placement.nearest + 4 * settings.ray_bias));

            // Relocation may not cross the cell-local displacement limit.
            if (dot(candidate, candidate) > relocation_limit * relocation_limit) {
                break;
            }

            out.probes[probe].offset = candidate;
            placement = classify(scene, out.position(probe), settings, directions.placement);
        }

        out.probes[probe].valid = !placement.inside;
    }

    // Retries the original position when alignment consumes an exit's displacement budget.
    void relocate(std::size_t probe) const {
        const profiling::Zone trace("Relocate probe");

        const auto initial = out.probes[probe].offset;
        relocate_from_current(probe);

        // Alignment must not discard a placement the original relocation could recover.
        if (!out.probes[probe].valid && initial != Float3{}) {
            const auto aligned = out.probes[probe].offset;
            out.probes[probe].offset = {};
            relocate_from_current(probe);

            // Retain the aligned candidate when neither starting point can leave the geometry.
            if (!out.probes[probe].valid) {
                out.probes[probe].offset = aligned;
            }
        }

        out.probes[probe].relocated = out.probes[probe].offset != initial;
    }

    // Traces the original primary sequence and independent explicit-emitter sequence.
    void sample(
        std::size_t probe, std::vector<RaySample>& rays, std::vector<EmissionSample>& emission
    ) const {
        const profiling::Zone trace("Trace probe lighting");

        const auto origin = out.position(probe);
        const ProbeLightDistribution nearby(scene, origin);
        const auto* distribution = nearby.local() ? &nearby : nullptr;
        Random random{settings.seed ^ (static_cast<std::uint32_t>(probe) * 747796405u)};
        const float rotation = random.next() * 2 * kPi;
        // Separate sequence: adding emitters cannot perturb first-hit moment rays.
        Random emission_random{
            settings.seed ^ (static_cast<std::uint32_t>(probe) * 747796405u) ^ 0x9e3779b9u
        };

        sample_emitters(origin, emission, emission_random, distribution);

        // Preserve primary order so transport random consumption stays independent of worker scheduling.
        for (std::uint32_t i = 0; i < settings.rays_per_probe; ++i) {
            const auto& base = directions.primary[i];
            const float angle = base.angle + rotation;
            const Float3 direction{
                base.radius * std::cos(angle), base.radius * std::sin(angle), base.z
            };
            rays[i] = trace_primary(origin, direction, random, distribution);
        }
    }

    void sample_emitters(
        Float3 origin,
        std::span<EmissionSample> emission,
        Random& random,
        const ProbeLightDistribution* distribution
    ) const {
        for (auto& light : emission) {
            light =
                sample_emitter(scene, origin, settings.ray_bias, random, settings, distribution);
            weight_emitter(light, 1 / (4.0 * kPi));
        }
    }

    RaySample trace_primary(
        Float3 origin, Float3 direction, Random& random, const ProbeLightDistribution* distribution
    ) const {
        auto hit = scene.trace(
            origin, direction, settings.ray_bias, std::numeric_limits<float>::infinity()
        );
        // Distant geometry still blocks GI; only the stored visibility distance is capped.
        const float distance = std::min(hit.ray.tfar, out.grid.max_distance);
        return {direction, radiance(scene, settings, hit, random, distribution), distance};
    }

    // Compare independent halves of the sample budget, including the directional visibility map.
    struct Convergence {
        bool lighting = true;
        bool visibility = true;
    };

    Convergence convergence(
        std::span<const RaySample> rays,
        std::span<const EmissionSample> emission,
        bool check_lighting
    ) const {
        const auto half = rays.size() / 2;
        const auto emitter_half = emission.size() / 2;
        const auto range = out.grid.max_distance;
        const auto resolution = out.grid.map_resolution;
        const auto texel_count = std::size_t{resolution} * resolution;
        constexpr auto kLanes = bake_detail::kIntegrationLanes;
        float light_error = 0, depth_error = 0, spread_error = 0;
        Convergence result;
        const auto spread = [](Float2 moments) {
            return std::sqrt(std::max(0.0f, moments[1] - moments[0] * moments[0]));
        };

        for (std::size_t block = 0; block < directions.texels.size(); ++block) {
            const auto& texels = directions.texels[block];
            const auto a = bake_detail::integrate_probe_block(
                texels,
                rays.first(half),
                emission.first(emitter_half),
                settings.distance_exponent,
                range
            );
            const auto b = bake_detail::integrate_probe_block(
                texels,
                rays.last(half),
                emission.last(emitter_half),
                settings.distance_exponent,
                range
            );
            for (std::size_t lane = 0; lane < std::min(kLanes, texel_count - block * kLanes);
                 ++lane) {
                if (check_lighting && result.lighting) {
                    result.lighting = accumulate_lighting_error(
                        a[lane].irradiance, b[lane].irradiance, light_error
                    );
                }
                const auto& x = a[lane].moments;
                const auto& y = b[lane].moments;
                const auto depth_delta = std::abs(x[0] - y[0]) / (0.06f * range);
                const auto spread_delta = std::abs(spread(x) - spread(y)) / (0.08f * range);
                result.visibility &= depth_delta <= 3 && spread_delta <= 3;
                depth_error += depth_delta * depth_delta;
                spread_error += spread_delta * spread_delta;
                if (!result.visibility && (!result.lighting || !check_lighting)) {
                    return {false, false};
                }
            }
        }
        // Test the whole map while bounding individual directions above.
        const auto texels = static_cast<float>(texel_count);
        result.lighting &= light_error <= 3 * texels;
        result.visibility &= depth_error <= texels && spread_error <= texels;
        return result;
    }

    std::pair<unsigned, unsigned> bake_adaptive(
        std::size_t probe, std::span<RaySample> rays, std::span<EmissionSample> emission
    ) const {
        const auto origin = out.position(probe);
        const ProbeLightDistribution nearby(scene, origin);
        const auto* distribution = nearby.local() ? &nearby : nullptr;
        const auto seed = settings.seed ^ (static_cast<std::uint32_t>(probe) * 747796405u);
        Random transport{seed};
        Random emitter{seed ^ 0x9e3779b9u};
        Random orientation{seed ^ 0x85ebca6bu};
        std::array<std::uint32_t, 2> shift{};
        for (auto& axis : shift) {
            orientation.next();
            axis = orientation.state;
        }
        unsigned lighting_count = 0;
        unsigned count = 0;
        for (unsigned target = kMinAdaptiveRays;; target *= 2) {
            const auto first = count;
            for (; count < target; ++count) {
                const auto direction = adaptive_direction(count, shift);
                // Once lighting converges, only the first hit contributes.
                if (lighting_count) {
                    const auto hit =
                        scene.trace(origin, direction, settings.ray_bias, out.grid.max_distance);
                    rays[count] = {direction, {}, hit.ray.tfar};
                } else {
                    rays[count] = trace_primary(origin, direction, transport, distribution);
                }
            }
            // The emitter sequence is independent of the transport sequence.
            if (!emission.empty() && !lighting_count) {
                sample_emitters(
                    origin, emission.subspan(first, count - first), emitter, distribution
                );
            }

            const auto primary = rays.first(count);
            const auto radiance_count = lighting_count ? lighting_count : count;
            const auto lights = emission.first(emission.empty() ? 0 : radiance_count);
            const auto result = count == kMaxAdaptiveRays
                                    ? Convergence{}
                                    : convergence(
                                          primary,
                                          lighting_count ? std::span<EmissionSample>{} : lights,
                                          !lighting_count
                                      );
            if (result.visibility && (lighting_count || result.lighting)) {
                integrate(probe, primary, lights, lighting_count);
                return {count, radiance_count};
            }
            if (result.lighting && !lighting_count) {
                lighting_count = count;
            }
        }
    }

    // Writes integrated texels while leaving odd-resolution tail lanes outside the gutters.
    void integrate(
        std::size_t probe,
        std::span<const RaySample> rays,
        std::span<const EmissionSample> emission,
        std::size_t lighting_ray_count = 0
    ) const {
        const profiling::Zone trace("Integrate probe maps");

        const auto& grid = out.grid;
        const auto resolution = grid.map_resolution, side = resolution + 2;
        const auto texel_count = std::size_t{resolution} * resolution;

        // Each SIMD block accumulates the same ordered samples into independent texel lanes.
        for (std::size_t block = 0; block < directions.texels.size(); ++block) {
            const auto samples = bake_detail::integrate_probe_block(
                directions.texels[block],
                rays,
                emission,
                settings.distance_exponent,
                grid.max_distance,
                lighting_ray_count
            );

            // Store active texels into the bordered probe map.
            for (std::size_t lane = 0; lane < bake_detail::kIntegrationLanes; ++lane) {
                const auto texel = block * bake_detail::kIntegrationLanes + lane;

                // Odd resolutions leave inactive lanes in the final block; never write them into gutters.
                if (texel >= texel_count) {
                    break;
                }

                const auto at = probe * grid.texels_per_probe() + (texel / resolution + 1) * side
                                + texel % resolution + 1;
                out.irradiance[at] = samples[lane].irradiance;
                out.distance_moments[at] = samples[lane].moments;
            }
        }
    }

    // Completes one probe using buffers private to its worker.
    std::pair<unsigned, unsigned> bake(
        std::size_t probe, std::vector<RaySample>& rays, std::vector<EmissionSample>& emission
    ) const {
        relocate(probe);
        if (settings.adaptive_rays) {
            return bake_adaptive(probe, rays, emission);
        }
        sample(probe, rays, emission);
        integrate(probe, rays, emission);
        return {settings.rays_per_probe, settings.rays_per_probe};
    }
};

// ---- Worker coordination ----

// Worker coordination; callbacks remain on the caller, and failures stop further claims.
class BakeWorkers {
public:
    // Borrows the per-volume work and callback until run() has joined every worker.
    BakeWorkers(
        const ProbeWork& work,
        const std::function<void(std::size_t, std::size_t)>& progress,
        std::uint32_t thread_count,
        BakeThreadPool* pool
    )
        : m_work(work),
          m_progress(progress),
          m_thread_count(thread_count),
          m_pool(pool) {
        // Workers inherit the caller's arithmetic environment, including directed rounding.
        if (std::fegetenv(&m_environment) != 0) {
            throw std::runtime_error("could not read bake floating-point environment");
        }
    }

    // Runs serially on the caller or joins a set of workers before propagating their first error.
    void run() {
        // The serial path invokes progress at each probe without a condition-variable handoff.
        if (inline_progress()) {
            // Concurrent level controllers must share even the single-thread execution budget.
            if (m_pool) {
                const auto result = m_pool->run(1, [this] { worker(); });

                if (!result) {
                    throw std::runtime_error(result.error().message);
                }
            } else {
                worker();
            }
        } else {
            run_parallel();
        }

        // Joined workers can no longer modify the stored exception.
        if (m_failure) {
            std::rethrow_exception(m_failure);
        }
        if (m_work.settings.adaptive_rays) {
            log::info(
                "adaptive probe rays: 64={} 128={} 256={} 512={}",
                m_ray_counts[0].load(),
                m_ray_counts[1].load(),
                m_ray_counts[2].load(),
                m_ray_counts[3].load()
            );
            log::info("adaptive visibility-only rays: {}", m_visibility_only.load());
        }
    }

private:
    // Only a direct serial worker may invoke progress from inside the tracing loop.
    bool inline_progress() const { return m_thread_count == 1 && (!m_pool || m_pool->size() == 1); }

    const ProbeWork& m_work;  // Borrowed immutable work descriptor with disjoint output writes.
    const std::function<void(std::size_t, std::size_t)>& m_progress;  // Caller-thread callback.
    std::uint32_t m_thread_count;        // Fixed before any worker starts.
    BakeThreadPool* m_pool;              // Borrowed optional pool; run() drains its tasks.
    std::fenv_t m_environment;           // Caller arithmetic environment copied to worker threads.
    std::atomic<std::size_t> m_next{0};  // Atomic probe claim shared by workers.
    std::atomic<bool> m_stopped{false};  // Atomic cancellation flag shared by caller and workers.
    std::array<std::atomic<std::size_t>, 4> m_ray_counts{};
    std::atomic<std::size_t> m_visibility_only{0};
    std::mutex m_mutex;                 // Guards completion count and the first worker failure.
    std::condition_variable m_changed;  // Wakes the caller on completion or failure.
    std::size_t m_completed = 0;        // Guarded by m_mutex.
    std::exception_ptr m_failure;       // Guarded by m_mutex until all workers join.

    // Reports directly on the serial path or wakes the caller after publishing completion.
    void complete(std::size_t probe) {
        // Serial callbacks already run on the caller's thread.
        if (inline_progress()) {
            // An absent callback requires no progress delivery.
            if (m_progress) {
                m_progress(probe + 1, m_work.out.probes.size());
            }

            return;
        }

        {
            const std::lock_guard lock(m_mutex);
            ++m_completed;
        }
        m_changed.notify_one();
    }

    // Retains the first worker error and wakes a caller waiting for progress.
    void fail() {
        {
            const std::lock_guard lock(m_mutex);

            // Later errors must not replace the original cause.
            if (!m_failure) {
                m_failure = std::current_exception();
            }
        }
        m_stopped.store(true, std::memory_order_relaxed);
        m_changed.notify_one();
    }

    // Record the stopping checkpoint and the samples spent on visibility alone.
    void record_adaptive_counts(unsigned count, unsigned lighting) {
        const auto bucket = std::bit_width(count / kMinAdaptiveRays) - 1;
        m_ray_counts[bucket].fetch_add(1, std::memory_order_relaxed);
        if (count != lighting) {
            m_visibility_only.fetch_add(count - lighting, std::memory_order_relaxed);
        }
    }

    // Claims probes until exhausted or cancelled, with private reusable ray buffers.
    void worker() {
        const profiling::Zone trace("Bake worker");

        // Every exit reports device errors or records an exception for the caller.
        try {
            // The serial path already uses the caller's environment.
            if (!inline_progress() && std::fesetenv(&m_environment) != 0) {
                throw std::runtime_error("could not set bake floating-point environment");
            }

            const auto budget =
                m_work.settings.adaptive_rays ? kMaxAdaptiveRays : m_work.settings.rays_per_probe;
            std::vector<RaySample> rays(budget);
            std::vector<EmissionSample> emission(m_work.scene.emitters.empty() ? 0 : budget);
            // Cancellation stops new claims; already claimed probes may finish their disjoint writes.
            while (!m_stopped.load(std::memory_order_relaxed)) {
                const auto probe = m_next.fetch_add(1, std::memory_order_relaxed);

                // The final claims may exceed the probe count when workers finish together.
                if (probe >= m_work.out.probes.size()) {
                    break;
                }

                const auto [count, lighting] = m_work.bake(probe, rays, emission);
                if (m_work.settings.adaptive_rays) {
                    record_adaptive_counts(count, lighting);
                }
                complete(probe);
            }

            // Embree records device errors per calling thread.
            m_work.scene.check_error();
        } catch (...) {
            fail();
        }
    }

    // Delivers completion notifications on the caller until all probes finish or a worker fails.
    void report_progress() {
        const profiling::Zone zone("Bake progress wait", profiling::ZoneColour::Idle);
        std::size_t reported = 0;

        // Completion counts may coalesce while the caller handles a callback.
        while (reported < m_work.out.probes.size()) {
            std::unique_lock lock(m_mutex);
            // Workers publish completion and failure under the same mutex before notifying.
            m_changed.wait(lock, [&] { return m_failure || m_completed > reported; });

            // Worker exceptions propagate only after joining in run_parallel().
            if (m_failure) {
                break;
            }

            reported = m_completed;
            lock.unlock();

            // Callbacks may throw and must run without holding the workers' mutex.
            if (m_progress) {
                m_progress(reported, m_work.out.probes.size());
            }
        }
    }

    // Drains borrowed tasks on every exit, including submission and callback failures.
    void run_parallel() {
        std::unique_ptr<BakeThreadPool> local;

        // Standalone callers retain their existing worker budget without owning a pool.
        if (!m_pool) {
            local = std::make_unique<BakeThreadPool>(m_thread_count);
        }

        auto& pool = m_pool ? *m_pool : *local;
        // Pool callbacks borrow this state until run() drains every task.
        const auto result = pool.run(
            m_thread_count,
            [this] { worker(); },
            [this] {
                try {
                    report_progress();
                } catch (...) {
                    m_stopped.store(true, std::memory_order_relaxed);
                    throw;
                }
            }
        );

        if (!result) {
            throw std::runtime_error(result.error().message);
        }
    }
};

#if OPENRAC_GPU_BAKE
// relocate() for every probe, with each classification round as one ray-query batch.
void relocate_on_gpu(const ProbeWork& work, bake_detail::GpuTransport& gpu) {
    auto& out = work.out;
    const auto& settings = work.settings;
    const auto& pairs = work.directions.placement;
    const auto count = out.probes.size();
    const float limit = settings.relocation_max_fraction
                        * *std::min_element(out.grid.spacing.begin(), out.grid.spacing.end());
    auto inside = [&](const bake_detail::GpuPlacement& placement) {
        return static_cast<float>(placement.inside_pairs) / static_cast<float>(pairs.size())
               >= settings.min_inside_fraction;
    };
    auto classify = [&](const std::vector<std::size_t>& probes) {
        std::vector<Float3> positions;
        positions.reserve(probes.size());

        for (auto probe : probes) {
            positions.push_back(out.position(probe));
        }

        return gpu.place(positions, pairs, 0);
    };
    // relocate_from_current over a set of probes, one batch per iteration.
    auto from_current = [&](std::vector<std::size_t> pending) {
        auto placement = classify(pending);

        for (std::uint32_t iteration = 0;
             iteration < settings.relocation_iterations && !pending.empty();
             ++iteration) {
            std::vector<std::size_t> next;

            for (std::size_t k = 0; k < pending.size(); ++k) {
                auto& probe = out.probes[pending[k]];

                if (!inside(placement[k])) {
                    probe.valid = true;
                    continue;
                }

                const auto candidate =
                    add(probe.offset,
                        mul(placement[k].exit_direction,
                            placement[k].nearest + 4 * settings.ray_bias));

                if (dot(candidate, candidate) > limit * limit) {
                    probe.valid = false;
                    continue;
                }

                probe.offset = candidate;
                next.push_back(pending[k]);
            }

            pending = std::move(next);
            placement =
                pending.empty() ? std::vector<bake_detail::GpuPlacement>{} : classify(pending);
        }

        for (std::size_t k = 0; k < pending.size(); ++k) {
            out.probes[pending[k]].valid = !inside(placement[k]);
        }
    };

    std::vector<Float3> initial(count);
    std::vector<std::size_t> all(count);

    for (std::size_t probe = 0; probe < count; ++probe) {
        initial[probe] = out.probes[probe].offset;
        all[probe] = probe;
    }

    from_current(all);
    std::vector<std::size_t> retry;
    std::vector<Float3> aligned(count);

    for (std::size_t probe = 0; probe < count; ++probe) {
        if (!out.probes[probe].valid && initial[probe] != Float3{}) {
            aligned[probe] = out.probes[probe].offset;
            out.probes[probe].offset = {};
            retry.push_back(probe);
        }
    }

    if (!retry.empty()) {
        from_current(retry);
    }

    for (auto probe : retry) {
        if (!out.probes[probe].valid) {
            out.probes[probe].offset = aligned[probe];
        }
    }

    for (std::size_t probe = 0; probe < count; ++probe) {
        out.probes[probe].relocated = out.probes[probe].offset != initial[probe];
    }
}

// Traces every 16th probe of a GPU batch on the CPU too and logs how far the two differ.
void check_gpu(
    const ProbeWork& work,
    std::size_t start,
    std::size_t batch,
    const std::vector<RaySample>& rays,
    const std::vector<EmissionSample>& emission,
    const BakeExecution& execution
) {
    const auto per_probe = work.settings.rays_per_probe;
    std::mutex mutex;
    double probes = 0, total = 0, same_depth = 0, same_light = 0;
    double error_sum = 0, error_max = 0, emission_error = 0;
    auto luminance = [](Float3 c) {
        return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
    };
    std::vector<std::size_t> picked;

    for (std::size_t i = 0; i < batch; ++i) {
        if ((start + i) % 16 == 0) {
            picked.push_back(i);
        }
    }

    parallel_bake_indices(picked.size(), execution, [&](std::size_t k) {
        const auto i = picked[k];
        std::vector<RaySample> cpu(per_probe);
        std::vector<EmissionSample> cpu_emission(emission.empty() ? 0 : per_probe);
        work.sample(start + i, cpu, cpu_emission);
        std::size_t depth = 0, light = 0;
        double gpu_sum = 0, cpu_sum = 0, gpu_emission = 0, cpu_emission_sum = 0;

        for (std::uint32_t r = 0; r < per_probe; ++r) {
            const auto& g = rays[i * per_probe + r];
            const auto& c = cpu[r];
            depth += std::abs(g.distance - c.distance) <= 1e-3f * std::max(1.f, c.distance) ? 1 : 0;
            const double gl = luminance(g.radiance), cl = luminance(c.radiance);
            light += std::abs(gl - cl) <= 1e-2 * std::max(1e-3, cl) ? 1 : 0;
            gpu_sum += gl;
            cpu_sum += cl;

            if (!emission.empty()) {
                gpu_emission += luminance(emission[i * per_probe + r].radiance_weight);
                cpu_emission_sum += luminance(cpu_emission[r].radiance_weight);
            }
        }

        const double error = std::abs(gpu_sum - cpu_sum) / std::max(1e-6, cpu_sum);
        const double emitted =
            std::abs(gpu_emission - cpu_emission_sum) / std::max(1e-6, cpu_emission_sum);
        const std::lock_guard lock(mutex);
        ++probes;
        total += per_probe;
        same_depth += static_cast<double>(depth);
        same_light += static_cast<double>(light);
        error_sum += error;
        error_max = std::max(error_max, error);
        emission_error += emitted;
    });

    if (probes > 0) {
        log::info(
            "GPU check: {} probes, depth equal {:.2f}%, ray light equal {:.2f}%, probe light error "
            "mean {:.4f}% max {:.4f}%, emitter samples error mean {:.4f}%",
            probes,
            100.0 * same_depth / total,
            100.0 * same_light / total,
            100 * error_sum / probes,
            100 * error_max,
            100 * emission_error / probes
        );
    }
}
#endif

// Reports the controller-selected alignment after all score queries have drained.
void report_alignment(const bake_detail::GridAlignment& alignment) {
    log::verbose(
        "probe alignment: offset {},{},{} score {} -> {}, near {} -> {}, inside {} -> {}",
        alignment.offset[0],
        alignment.offset[1],
        alignment.offset[2],
        alignment.before.mean,
        alignment.after.mean,
        alignment.before.near,
        alignment.after.near,
        alignment.before.inside,
        alignment.after.inside
    );
}

}  // namespace

// ---- Prepared baker ----

// The scene borrows its immutable source; the owning export outlives this implementation.
struct ProbeBaker::Impl {
    BakeSettings settings;
    Scene scene;
    BakeThreadPool* pool = nullptr;  // Borrowed execution budget, never part of bake identity.
    std::mutex direction_mutex;      // protects table creation; workers only read immutable entries
    std::map<std::pair<std::uint32_t, int>, std::unique_ptr<const BakeDirections>> direction_cache;
#if OPENRAC_GPU_BAKE
    std::unique_ptr<bake_detail::GpuTransport> gpu;  // Probe rays on the GPU (OPENRAC_GPU_BAKE).
#endif

    Impl(
        const BakeScene& input,
        const BakeSettings& options,
        std::uint32_t threads,
        BakeThreadPool* workers,
        bake_detail::EmissionAverageCache* emission_cache
    )
        : settings(options),
          scene(
              input,
              options.sunlight_direction,
              {.threads = threads,
               .pool = workers,
               .emission_cache = emission_cache,
               .prepare_nearby_mesh_lights = options.adaptive_rays,
               .high_build_quality = true}
          ),
          pool(workers) {
#if OPENRAC_GPU_BAKE
        if (!settings.adaptive_rays && env::flag(env::Var::GpuBake)) {
            gpu = bake_detail::GpuTransport::create(scene, settings);
        }
#endif
    }

    // Rounding is part of the key so reuse cannot change a later bake's arithmetic results.
    const BakeDirections& directions(std::uint32_t resolution) {
        const std::lock_guard lock(direction_mutex);
        auto& cached = direction_cache[{resolution, std::fegetround()}];

        if (!cached) {
            cached = std::make_unique<BakeDirections>(settings.rays_per_probe, resolution);
        }

        return *cached;
    }
};

ProbeBaker::ProbeBaker(
    const BakeScene& input,
    const BakeSettings& settings,
    std::uint32_t threads,
    BakeThreadPool* pool,
    bake_detail::EmissionAverageCache* emission_cache
) {
    const profiling::Zone trace("Prepare probe scene");

    bake_detail::validate_bake_scene(input, settings);
    m_impl = std::make_unique<Impl>(input, settings, threads, pool, emission_cache);
}

ProbeBaker::~ProbeBaker() = default;

const Scene& ProbeBaker::traversal_scene() const {
    return m_impl->scene;
}

// ---- Probe workers ----

ProbeVolume ProbeBaker::bake(
    const ProbeGrid& grid,
    const std::function<void(std::size_t, std::size_t)>& progress,
    std::uint32_t thread_count
) const {
    const profiling::Zone trace("Bake probe volume");

    grid.validate();
    const auto& settings = m_impl->settings;

    // A grid's moment range must still leave room for the configured ray bias.
    if (settings.ray_bias >= grid.max_distance) {
        throw std::runtime_error("invalid probe bake settings");
    }

    const auto& scene = m_impl->scene;

    // Notify the caller before allocation or worker startup can fail.
    if (progress) {
        progress(0, grid.probe_count());
    }

    auto out = ProbeVolume::allocate(grid);

    // Each probe has its own seed and disjoint storage; scheduling cannot change its arithmetic order.
    if (thread_count == 0) {
        thread_count =
            m_impl->pool ? m_impl->pool->size() : std::max(1u, std::thread::hardware_concurrency());
    }

    // A shared execution budget limits explicit per-grid requests too.
    if (m_impl->pool) {
        thread_count = std::min(thread_count, m_impl->pool->size());
    }

    thread_count =
        static_cast<std::uint32_t>(std::min<std::size_t>(thread_count, out.probes.size()));
    std::unique_ptr<BakeThreadPool> local_pool;

    // Standalone alignment and transport share one worker lifetime instead of creating separate pools.
    if (!m_impl->pool && thread_count > 1) {
        local_pool = std::make_unique<BakeThreadPool>(thread_count);
    }

    BakeExecution execution{thread_count, m_impl->pool ? m_impl->pool : local_pool.get()};
#if OPENRAC_GPU_BAKE
    execution.gpu = m_impl->gpu.get();
#endif
    const auto alignment = bake_detail::align_probe_grid(scene, settings, out, execution);
    report_alignment(alignment);

    const ProbeWork work{scene, settings, m_impl->directions(grid.map_resolution), out};

#if OPENRAC_GPU_BAKE
    // GPU path: relocation and integration on the CPU workers, every probe ray on the device.
    if (m_impl->gpu) {
        const auto count = out.probes.size();
        relocate_on_gpu(work, *m_impl->gpu);
        const auto rays_per_probe = settings.rays_per_probe;
        constexpr std::size_t kBatch = 8192;
        std::vector<RaySample> rays(kBatch * rays_per_probe);
        std::vector<EmissionSample> emission(scene.emitters.empty() ? 0 : rays.size());
        std::vector<Float3> origins;

        for (std::size_t start = 0; start < count; start += kBatch) {
            const auto batch = std::min(kBatch, count - start);
            origins.clear();

            for (std::size_t i = 0; i < batch; ++i) {
                origins.push_back(out.position(start + i));
            }

            {
                m_impl->gpu->sample(
                    origins,
                    static_cast<std::uint32_t>(start),
                    grid.max_distance,
                    std::span(rays).first(batch * rays_per_probe),
                    emission.empty() ? std::span<EmissionSample>{}
                                     : std::span(emission).first(batch * rays_per_probe)
                );
            }

            if (env::flag(env::Var::GpuBakeCheck)) {
                check_gpu(work, start, batch, rays, emission, execution);
            }

            parallel_bake_indices(batch, execution, [&](std::size_t i) {
                const auto at = i * rays_per_probe;
                work.integrate(
                    start + i,
                    std::span<const RaySample>(rays).subspan(at, rays_per_probe),
                    emission.empty()
                        ? std::span<const EmissionSample>{}
                        : std::span<const EmissionSample>(emission).subspan(at, rays_per_probe)
                );
            });

            if (progress) {
                progress(start + batch, count);
            }
        }
    } else
#endif
        BakeWorkers(work, progress, thread_count, execution.pool).run();

    scene.check_error();
    update_probe_gutters(out);
    out.validate();
    return out;
}

ProbeVolume bake_probes(
    const ProbeGrid& grid,
    const BakeScene& scene,
    const BakeSettings& settings,
    const std::function<void(std::size_t, std::size_t)>& progress,
    std::uint32_t thread_count
) {
    grid.validate();
    return ProbeBaker(scene, settings, thread_count).bake(grid, progress, thread_count);
}

}  // namespace openrac::lighting
