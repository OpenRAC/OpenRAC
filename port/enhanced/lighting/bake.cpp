// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Cached directions, probe integration and worker lifetime for offline level baking.
#include "enhanced/lighting/bake.h"

#include <atomic>
#include <cfenv>
#include <condition_variable>
#include <exception>
#include <map>
#include <mutex>
#include <thread>

#include "common/log.h"
#include "common/profiler.h"
#include "enhanced/lighting/bake_alignment.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/bake_transport.h"
#include "enhanced/lighting/bake_workers.h"

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
using bake_detail::RaySample;

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

        const auto& grid = out.grid;
        const auto origin = out.position(probe);
        Random random{settings.seed ^ (static_cast<std::uint32_t>(probe) * 747796405u)};
        const float rotation = random.next() * 2 * kPi;
        // Separate sequence: adding emitters cannot perturb first-hit moment rays.
        Random emission_random{
            settings.seed ^ (static_cast<std::uint32_t>(probe) * 747796405u) ^ 0x9e3779b9u
        };

        // Explicit emitters use a separate sequence from primary rays and their moments.
        for (auto& light : emission) {
            light = sample_emitter(scene, origin, settings.ray_bias, emission_random, settings);
            weight_emitter(light, 1 / (4.0 * kPi));
        }

        // Preserve primary order so transport random consumption stays independent of worker scheduling.
        for (std::uint32_t i = 0; i < settings.rays_per_probe; ++i) {
            const auto& base = directions.primary[i];
            const float angle = base.angle + rotation;
            const Float3 direction{
                base.radius * std::cos(angle), base.radius * std::sin(angle), base.z
            };
            auto hit = scene.trace(
                origin, direction, settings.ray_bias, std::numeric_limits<float>::infinity()
            );
            // Distant geometry still blocks GI; only the stored visibility distance is capped.
            const float distance = std::min(hit.ray.tfar, grid.max_distance);
            rays[i] = {direction, radiance(scene, settings, hit, random), distance};
        }
    }

    // Writes integrated texels while leaving odd-resolution tail lanes outside the gutters.
    void integrate(
        std::size_t probe,
        const std::vector<RaySample>& rays,
        const std::vector<EmissionSample>& emission
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
                grid.max_distance
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
    void bake(
        std::size_t probe, std::vector<RaySample>& rays, std::vector<EmissionSample>& emission
    ) const {
        relocate(probe);
        sample(probe, rays, emission);
        integrate(probe, rays, emission);
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
    std::mutex m_mutex;                  // Guards completion count and the first worker failure.
    std::condition_variable m_changed;   // Wakes the caller on completion or failure.
    std::size_t m_completed = 0;         // Guarded by m_mutex.
    std::exception_ptr m_failure;        // Guarded by m_mutex until all workers join.

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

    // Claims probes until exhausted or cancelled, with private reusable ray buffers.
    void worker() {
        const profiling::Zone trace("Bake worker");

        // Every exit reports device errors or records an exception for the caller.
        try {
            // The serial path already uses the caller's environment.
            if (!inline_progress() && std::fesetenv(&m_environment) != 0) {
                throw std::runtime_error("could not set bake floating-point environment");
            }

            std::vector<RaySample> rays(m_work.settings.rays_per_probe);
            std::vector<EmissionSample> emission(
                m_work.scene.emitters.empty() ? 0 : m_work.settings.rays_per_probe
            );

            // Cancellation stops new claims; already claimed probes may finish their disjoint writes.
            while (!m_stopped.load(std::memory_order_relaxed)) {
                const auto probe = m_next.fetch_add(1, std::memory_order_relaxed);

                // The final claims may exceed the probe count when workers finish together.
                if (probe >= m_work.out.probes.size()) {
                    break;
                }

                m_work.bake(probe, rays, emission);
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

    Impl(
        const BakeScene& input,
        const BakeSettings& options,
        std::uint32_t threads,
        BakeThreadPool* workers
    )
        : settings(options),
          scene(input, options.sunlight_direction, threads),
          pool(workers) {}

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
    BakeThreadPool* pool
) {
    const profiling::Zone trace("Prepare probe scene");

    bake_detail::validate_bake_scene(input, settings);
    m_impl = std::make_unique<Impl>(input, settings, threads, pool);
}

ProbeBaker::~ProbeBaker() = default;

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

    const BakeExecution execution{thread_count, m_impl->pool ? m_impl->pool : local_pool.get()};
    const auto alignment = bake_detail::align_probe_grid(scene, settings, out, execution);
    report_alignment(alignment);

    const ProbeWork work{scene, settings, m_impl->directions(grid.map_resolution), out};

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
