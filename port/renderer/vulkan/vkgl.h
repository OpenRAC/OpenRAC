// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The renderer's OpenGL (renderer/gl/gl.h, renderer/world/gl_world.h, and the few calls gl_stats.h
// adds) carried out with Vulkan: the port's Vulkan renderer. Every renderer is written against
// that short list of GL calls, so this is the one place that knows Vulkan; the renderers, their
// shaders and the frontend run on it unchanged (docs/port/ANDROID.md, "A Vulkan renderer"). It is
// what draws on a custom Vulkan driver (platform/gpu_driver.h), which no GLES driver can load.
//
// How GL's model becomes Vulkan's:
//   - GL objects are names in tables here; a texture is a VkImage, a buffer a CPU copy uploaded
//     where a draw needs it (into the frame's ring while it changes, into memory of its own once it
//     stays), a framebuffer a render pass begun at its first draw or clear.
//   - A program is compiled by glslang (shader_compiler.h); its loose uniforms are a block that
//     each draw that changed them copies into the frame's ring.
//   - The state a pipeline bakes in (blend, depth, stencil, cull, the vertex layout, the
//     primitive, the program, the render pass) is a key into a cache of pipelines; viewport,
//     scissor, blend constant and stencil reference are dynamic.
//   - Rows: both APIs put a framebuffer's row 0 at NDC y = -1, so draws, viewports, scissors,
//     reads and copies keep their numbers; the default framebuffer is an image of its own, turned
//     right way up when it is copied to the screen at present().
//   - Everything is recorded in one command buffer a frame, two frames in flight.
//
// One context at a time with one host owner. The frontend transfers exclusive ownership to its
// render worker while recording an owned world snapshot, then joins that job before UI or present.
// The window stays on its creating thread; OpenGL contexts do not use this handoff.

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/hdr.h"
#include "renderer/gl/gpu_draw.h"

struct SDL_Window;

namespace openrac::vkgl {

// Recording cannot continue after a command-buffer, submission or fence operation fails.
class RecordingFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Config {
    // The window to present to (made with SDL_WINDOW_VULKAN), or null: drawing without a screen,
    // for the tests (glReadPixels reads the default framebuffer).
    SDL_Window* window = nullptr;
    int width = 0;  // the default framebuffer without a window
    int height = 0;
    bool vsync = true;
    // A Vulkan driver of the player's (platform/gpu_driver.h): its vkGetInstanceProcAddr; null for
    // the system's loader.
    void* get_instance_proc_addr = nullptr;
    bool validation = false;  // the Khronos validation layer, where it is installed
    bool gpu_breadcrumbs = true;
    // For the tests: each presentation extension the backend asks of the device where it is
    // offered (VK_KHR_present_id, VK_KHR_present_wait, the FIFO latest-ready present mode), so a
    // test can open the device without it.
    bool present_id = true;
    bool present_wait = true;
    bool fifo_latest_ready = true;
    // HDR output may be asked for (set_hdr): the instance is given VK_EXT_swapchain_colorspace
    // and the device VK_EXT_hdr_metadata where offered. Off (the player's `hdr = off` at start),
    // the instance, the device and the swapchain are made as they always were.
    bool hdr_colour_spaces = false;

    // An outside maker of the instance and the device, which then shares them (a mod's OpenXR
    // session, openrac_mod.h's openrac_vulkan_setup): each function stands in for the backend's
    // own step, from the create info the backend built; a null one leaves that step the
    // backend's. Pointers stand for the Vulkan types, int for VkResult.
    struct DeviceSetup {
        void* user = nullptr;
        int (*create_instance)(
            void* user, void* get_instance_proc_addr, const void* create_info, void** instance
        ) = nullptr;
        void* (*physical_device)(void* user, void* instance) = nullptr;
        int (*create_device)(
            void* user,
            void* get_instance_proc_addr,
            void* physical_device,
            const void* create_info,
            void** device
        ) = nullptr;
    } setup;
};

// The open context's Vulkan objects, for an outside user of the device (a mod's OpenXR session):
// null and 0 with no context. The queue is the one every submit goes to.
struct Native {
    void* instance = nullptr;
    void* physical_device = nullptr;
    void* device = nullptr;
    void* queue = nullptr;
    std::uint32_t queue_family = 0;
    std::uint32_t queue_index = 0;
    void* get_instance_proc_addr = nullptr;
};

Native native();

// Copies the colour of GL texture `texture` (RGBA8, width by height) into `image`, a VkImage of
// the device made outside the backend (VK_FORMAT_R8G8B8A8_UNORM or _SRGB, the same size, a
// transfer destination), which is in `layout` (a VkImageLayout) and is left in it. Recorded in the
// frame's commands, so done once present() has submitted them. False, with nothing recorded, for a
// texture that is not that size or not colour.
bool copy_to_image(unsigned texture, void* image, std::uint32_t layout, int width, int height);

// Opens the device and the context. False, with why in `error`, when Vulkan cannot be used: the
// caller falls back to OpenGL.
bool open(const Config& config, std::string& error);
void close();
bool active();

// Batches texture copies during a level bake upload on the render thread.
class BulkTextureUpload {
public:
    explicit BulkTextureUpload(std::size_t estimated_bytes);
    ~BulkTextureUpload();
    BulkTextureUpload(const BulkTextureUpload&) = delete;
    BulkTextureUpload& operator=(const BulkTextureUpload&) = delete;

    // Waits for the final batch before the caller publishes the textures.
    void finish();

private:
    bool m_enabled = false;
};

// GPU culling/compaction and device-generated indexed draw arguments. No
// readback participates in visibility decisions. Handles last one recording
// (including readback flush boundaries), not merely one displayed frame.
bool gpu_driven_active();
bool gpu_materials_active();
// Starts a recording if needed and returns its identity; zero without GPU instance rendering.
// Cached instance batches must be rebuilt after the identity changes, including readback flushes.
std::uint64_t gpu_instance_recording();
// Indexed viewports route shadow draws to atlas rectangles from the vertex
// shader. Empty rectangles restore the ordinary single-viewport path.
bool indexed_viewports_supported(unsigned count);
bool extended_dynamic_state_supported();
void indexed_viewports(std::span<const std::array<int, 4>> rectangles);
renderer::GpuInstanceBatch gpu_instances(
    unsigned instance_buffer,
    std::span<const renderer::GpuCullBounds> bounds,
    const renderer::GpuCullView& view
);
bool draw_gpu_instances(
    renderer::GpuInstanceBatch batch, unsigned first_index, unsigned index_count
);

// Consecutive ranges sharing all current state and one instance batch. No per-draw materials.
bool draw_gpu_instance_ranges(
    renderer::GpuInstanceBatch batch, std::span<const renderer::GpuIndexRange> ranges
);

// Draws triangles with uint32 indices from the current vertex array using a compute-written command.
// `arguments` is a storage buffer beginning with VkDrawIndexedIndirectCommand; false for invalid inputs.
[[nodiscard]] bool draw_indexed_indirect(unsigned arguments, unsigned index_count);

// Registers the instances of a static instance buffer once, with their culling bounds: while the
// buffer is unchanged and gpu-resident-instances is on, gpu_instances of it sends the GPU only the
// view and a group record, and the culling reads the instances from device memory
// (docs/port/RENDER_PERF.md, R-15). Registering a buffer again replaces its instances. False when
// GPU-driven drawing is off or the sizes disagree.
bool gpu_register_instances(
    unsigned instance_buffer, std::span<const renderer::GpuCullBounds> bounds
);

// Forgets every registration (a level's release). The device copy goes once every frame that
// reads it has retired.
void gpu_release_instances();

struct GpuDrawCounters {
    std::uint64_t recording = 0, draws = 0, triangles = 0, visible_instances = 0;
    std::uint64_t readback_bytes = 0;
};

// Last completed collected recording, read after its existing frame-slot fence.
// Counters are collected only while stats are enabled; otherwise returns zero.
GpuDrawCounters gpu_completed_draws();

struct GpuMaterialCounters {
    bool supported = false;
    std::uint64_t draws = 0, records = 0, textures = 0, native_draws = 0;
};

GpuMaterialCounters gpu_material_counters();

// Copy RGBA8, RG32F or RGBA32F compute output to an allocated 2D texture without a readback.
// Includes compute-write -> transfer-read and image-sampling dependencies.
// reuse_source orders the copy before subsequent compute writes to the same buffer.
bool copy_buffer_to_texture(
    unsigned buffer, unsigned texture, int width, int height, bool reuse_source = false
);

// Reversed-depth union: wrapped static depth plus an ordinary rectangle of dynamic depth.
// The destination is an equally sized R32F storage texture; no CPU readback or image copy.
bool combine_shadow_depth(
    unsigned cached,
    unsigned dynamic,
    unsigned destination,
    std::array<int, 2> offset,
    std::array<int, 4> dynamic_rect
);

// renderer/gl/gl.h's loader: the GL functions by name, null for one this does not carry out.
using Proc = void (*)();
Proc get_proc(const char* name);

// The kinds of Vulkan command the backend records, as SubmissionCounters::commands splits them
// (docs/port/RENDER_PERF.md, "The counters").
enum class Command : std::uint8_t {
    BindPipeline,     // vkCmdBindPipeline
    BindDescriptors,  // vkCmdBindDescriptorSets, vkCmdPushConstants
    BindBuffers,      // vkCmdBindVertexBuffers, vkCmdBindIndexBuffer
    DynamicState,     // vkCmdSetViewport, vkCmdSetScissor, vkCmdSetBlendConstants, vkCmdSetStencil*
    Draw,             // vkCmdDraw, vkCmdDrawIndexed, vkCmdDrawIndexedIndirect
    Barrier,          // vkCmdPipelineBarrier
    Copy,             // vkCmdCopy*, vkCmdBlitImage, vkCmdResolveImage
    Dispatch,         // vkCmdDispatch
    RenderPass,       // vkCmdBeginRendering, vkCmdEndRendering
    Query,            // vkCmdWriteTimestamp, vkCmdResetQueryPool
    Other,            // vkCmdClearAttachments and the debug labels
};

// The number of kinds: Other is the last enumerator of Command.
inline constexpr std::size_t kCommandKinds = static_cast<std::size_t>(Command::Other) + 1;

// Cumulative submission counters for profiling and offscreen regression tests.
// Zero without an active context; reset when a new context opens.
//
// The members from `commands` on are the render counters of docs/port/RENDER_PERF.md: exact counts
// and bytes, the same on every machine for the same work, which decide whether a render
// optimisation pays. Counting them is an add where the work is recorded; present() publishes them
// as the frame's vk_* gauges while the debug tools measure (common/stats.h).
struct SubmissionCounters {
    std::uint64_t surface_chains =
        0;  // Successfully executed shared surface, colour or temporal sections.
    std::uint64_t validation_errors = 0;
    std::uint64_t uniform_writes = 0, uniform_writes_skipped = 0;
    std::uint64_t uniform_uploads = 0, uniform_upload_bytes = 0;
    std::uint64_t descriptor_sets = 0, descriptor_reuses = 0;
    std::uint64_t gpu_counter_dispatches = 0, gpu_counter_copies = 0;

    // The vkCmd* recorded, by Command; commands_recorded() adds them up.
    std::array<std::uint64_t, kCommandKinds> commands{};
    std::uint64_t pipelines_created = 0;  // Draw-thread creations; excludes prewarm worker calls.
    // Pipelines a draw linked from parts (vk-pipeline-library), not counted in pipelines_created.
    std::uint64_t pipelines_linked = 0;
    // Pipelines specialised with a program's modes (vk-mode-constants): asked of the workers, or
    // made at their draw where there are none.
    std::uint64_t mode_pipelines = 0;
    // Pipelines made once the warm-up begin_pipeline_warmup started had passed.
    std::uint64_t pipelines_created_after_warmup = 0;
    std::uint64_t descriptor_sets_allocated = 0;  // Every vkAllocateDescriptorSets set.
    std::uint64_t descriptor_writes = 0;          // VkWriteDescriptorSet entries written.
    std::uint64_t render_passes = 0;              // vkCmdBeginRendering calls.
    // Bytes the passes read in from memory (load op LOAD) and write back (store op STORE, and a
    // resolve's target): each attachment's texel bytes times the render area times its samples.
    std::uint64_t attachment_load_bytes = 0;
    std::uint64_t attachment_store_bytes = 0;
    std::uint64_t barriers = 0;               // vkCmdPipelineBarrier calls.
    std::uint64_t barriers_all_commands = 0;  // Of those, the ones with an ALL_COMMANDS stage.
    std::uint64_t submits = 0;                // vkQueueSubmit calls.
    std::uint64_t frames_presented = 0;       // present() calls: frames ended.
    // Copies, blits and resolves whose extent is the default framebuffer's or the screen's.
    std::uint64_t fullscreen_copies = 0;
    std::uint64_t ring_bytes = 0;            // Bytes taken from the frames' upload rings.
    std::uint64_t texture_creations = 0;     // Images made: textures, render targets, mip chains.
    std::uint64_t texture_upload_bytes = 0;  // Texel bytes staged for upload into images.
    // Bytes of the buffers that stopped changing and got memory of their own, in memory the host
    // can see (VmaAllocationInfo's memory type), made so far.
    std::uint64_t buffer_static_bytes_host_visible = 0;
    std::uint64_t cpu_mirror_bytes = 0;  // Now: the CPU copies GL's buffers keep (glBufferData).
    std::uint64_t buffer_zero_fill_bytes =
        0;  // Cumulative: glBufferData mirror zero initialisation.
    std::uint64_t buffer_mirror_restore_bytes = 0;  // Cumulative: discarded mirrors recovered.
    // Bytes the CPU writes for the GPU-driven instances: each batch's payload as it is built, and
    // its copy into the mapped input page at the end of the recording.
    std::uint64_t gpu_payload_bytes = 0;
    std::uint64_t shader_fp16_ops = 0, shader_fp32_ops = 0, shader_fp16_modules = 0;
    std::uint64_t shader_compiles = 0;  // glslang runs: shaders checked, programs compiled.
    double shader_compile_s = 0.0;      // The seconds those compiles took.
    std::uint64_t test_program_reuses = 0;  // Immutable driver object reuse in an opted-in test.
    std::uint64_t program_reuses = 0;       // Links that took a kept program's objects.
    double pipeline_compile_s = 0.0;    // Driver pipeline creation, including prewarm workers.
    std::uint64_t pipeline_creation_calls =
        0;  // Driver calls, including speculative and failed ones.
};

SubmissionCounters submission_counters();

// Returns the commands of `counters` added up over every kind: vk_cmds_recorded.
inline std::uint64_t commands_recorded(const SubmissionCounters& counters) {
    std::uint64_t total = 0;
    for (const std::uint64_t kind : counters.commands) {
        total += kind;
    }
    return total;
}

// Starts a warm-up of `frames` frames now, for the benchmark after it uploads a level: a pipeline
// made once that many frames have been presented counts in pipelines_created_after_warmup as well.
// Until the first call no pipeline counts there.
void begin_pipeline_warmup(std::uint32_t frames);

// For the tests: with `on`, an attachment glInvalidateFramebuffer let go is overwritten with
// garbage (colour 1.0 in every channel, depth 0, stencil 0xA5) instead of being left undefined, so
// that a later read of it shows in the picture. False when no context is open.
bool test_poison_discards(bool on);

// Enables test-only reuse of identical compiled programs/layouts; disabled for player launches.
// Disabling bypasses the pool without discarding its objects; close() releases them.
// borrow=false retains objects after an independent fresh-driver oracle without reusing them.
void test_reuse_programs(bool on, bool borrow = true);

// For the game: with `on`, a deleted program's shader modules, layouts and pipelines are kept, and
// the next link of the same shaders takes them (vk-program-keep, docs/port/RENDER_PERF.md). A
// level's load deletes and links the same programs, so it makes no pipelines again. Uniform values
// are never kept. Off (the default, and the tests'), a deleted program's objects are destroyed.
void keep_programs(bool on);

// While an upscaler draws the world at a smaller size, its mipmapped textures take this bias
// (log2 of render over display width, below zero) so that they keep the display's detail; 0 for
// everything else. Read when a draw binds its textures.
void set_texture_lod_bias(float bias);

// For the tests: draws the copy into the default framebuffer that waits for the present
// (present-single-copy) into two stand-ins of the screen, B8G8R8A8 at the default framebuffer's
// size: `single` the one blit the present makes, `two_step` the copy into the default
// framebuffer and its copy to the screen. The copy is made in the default framebuffer too. False
// when no copy waits.
bool test_present_paths(std::vector<std::uint8_t>& single, std::vector<std::uint8_t>& two_step);

// Waits until the pre-warm worker has made every pipeline queued for it (R-10, pipeline-prewarm)
// and returns how many it has made since the backend opened; 0 at once while it is closed. For
// the tests, which then draw and count the pipelines the drawing thread had to make itself.
std::uint64_t wait_for_pipeline_prewarm();

// Whether the pre-warm workers still have a pipeline to make; false while the backend is closed.
// A draw whose specialised pipeline they have not made yet uses one that reads its modes from the
// uniforms (vk-mode-constants), so a measurement waits for this to be false.
bool pipeline_prewarm_busy();

// For a level's load, behind its loading screen: waits until the pre-warm workers have made every
// pipeline and part queued when the level's programs linked (the pipelines earlier runs drew them
// with), so that its first frames make none, and saves the pipeline caches on a thread of their
// own if this run made new pipelines. Optional upgrades discovered during gameplay are recorded
// in the manifest for a future load. While waiting, pump_events can keep the window responsive.
// Nothing while the backend is closed.
void finish_pipelines(void (*pump_events)() = nullptr);

// When the presents have been seen to reach the screen (VK_KHR_present_wait, under the registry's
// present-timing). `visible` is the moment the backend saw present `id` finished, on the steady
// clock: no later than a little after it was shown, so a display phase taken from it is late by at
// most the time between the poll and the refresh.
struct PresentTiming {
    bool supported = false;  // The device and swapchain give present ids and their waits.
    bool known = false;      // A present has been seen finished: `visible` and `id` hold.
    std::chrono::steady_clock::time_point visible{};
    std::uint64_t id = 0;  // The present it was, counted from 1 since the swapchain was made.
};

// Returns what is known of the presents now, after looking, without waiting, whether the most
// recent one has finished. Never blocks.
PresentTiming present_timing();

// Waits at most `timeout` for the most recent present to reach the screen, for a caller that
// sleeps anyway (the frame pacer); true when it did, with its moment kept for present_timing().
// False at once when present-timing is off or the device has no VK_KHR_present_wait.
bool wait_for_present(std::chrono::nanoseconds timeout);

// Shows the default framebuffer. The next frame slot is reclaimed on its first graphics use.
void present();

// The wait for the display, as platform::SwapInterval counts it: -1 adaptive, 0 none, 1 vsync.
// Returns what the screen allows: the nearest of those.
int set_swap_interval(int interval);

// Whether a swap that does not wait for a refresh is shown at once (the immediate present mode),
// rather than at the next refresh as the latest image (mailbox, as Android's only is).
bool swap_shows_at_once();

// Low-latency frames: as few swapchain images as the display allows at full rate
// (swapchain_images.h) and a frame queued only once the one before it is done on the GPU, so that
// fewer finished frames wait to be shown. Off: one more image than the surface needs (at least
// three) and two frames in flight.
void set_low_latency(bool on);

// HDR output (common/hdr.h): the screen is sent `wanted` at `levels` from the next present on,
// where the surface offers it (the swapchain made again when it changes); else, and always for
// Output::Sdr, the picture's bytes as before. Needs Config::hdr_colour_spaces for any HDR.
struct HdrRequest {
    hdr::Output wanted = hdr::Output::Sdr;
    hdr::Levels levels;

    bool operator==(const HdrRequest&) const = default;
};

void set_hdr(const HdrRequest& request);

// What the screen is sent now (SDR before the first present), and whether the surface offers an
// HDR colour space at all (false without Config::hdr_colour_spaces).
hdr::Output hdr_output();
bool hdr_offered();

// HDR output's frame (docs/port/HDR.md): true while the screen is sent HDR. The renderer then
// draws its frame in half floats (RGBA16F) with clamp_colour_writes, so that what the game's
// blending adds up (glows, explosions, lasers) may go above white instead of stopping at it, and
// the default framebuffer is RGBA16F too. False in SDR: 8-bit frames, as always.
bool hdr_frame();

// Draws into `framebuffer`'s colour image are given each fragment's colour clamped to 0..1 (the
// stage made again with a clamp at its end), as an 8-bit target clamps what it is given before
// blending: in a float frame only blending's sums go above 1. Until the image is made again.
void clamp_colour_writes(unsigned framebuffer);

// Draws into the default framebuffer leave its depth and stencil image out of their pass while
// `attached` is false. On Adreno with 2x MSAA, a pass that keeps that unused image broke the
// overlay drawn in it.
void default_framebuffer_depth(bool attached);

// For the tests: the encode pass run on the default framebuffer (or on framebuffer `source`'s
// picture, such as a float one above white) into a stand-in of an HDR screen (`output`'s format,
// `width` by `height`), read back as stored: one 32-bit word a texel for Output::Hdr10
// (A2B10G10R10) and four half floats for Output::ScRgb. `through_float`: as on a device that
// can't store the screen's format (half floats, blitted). False without a context.
bool test_hdr_encode(
    hdr::Output output,
    const hdr::Levels& levels,
    int width,
    int height,
    std::vector<std::uint8_t>& out,
    bool staged = false,
    unsigned source = 0,
    bool through_float = false
);

// With low-latency frames, called by the window's thread before a frame begins, before it reads
// the time it is drawn for and the input: sleeps until the frame, begun then, would be queued just
// as the GPU is free for it (latency_sleep.h), never past `limit`. Returns the seconds slept.
double latency_sleep(std::chrono::steady_clock::time_point limit);

// With low-latency frames, as a frame begins (after its latency_sleep, if any, which slept
// `slept` seconds): notes when. A frame that is noted is recorded while the one before it is
// still on the GPU, and waits for it only as it is queued; one that is not (or one before the CPU
// and GPU times are known, or without GPU timestamps) waits as its recording begins, as before.
void latency_begin(double slept);

// The device and driver, for the log: "Adreno (TM) 740", "Qualcomm 512.744.0", "1.3.284".
const std::string& device_name();
const std::string& driver_version();
const std::string& api_version();
bool software();

// True when the open device has memory that is device-local and not host-visible, where a static
// buffer's staged copy goes (docs/port/RENDER_PERF.md, "R-05"). False with no context, and on a
// driver whose every memory type the host sees (lavapipe, some integrated GPUs).
bool device_only_memory();

// True when the open device draws a pass tile by tile in on-chip memory: Arm Mali, Qualcomm
// Adreno, Apple and Imagination GPUs, told by the vendor's id (Vulkan has no property for it).
// There vertex binning and attachment traffic cost most, so the optimisations that save them
// default on (docs/port/RENDER_PERF.md, "Tile-based GPUs"). OPENRAC_TILE_BASED=1 or 0 overrides
// the guess, to try a tile-based default on a desktop GPU or to compare without it. False with
// no context.
bool tile_based();

// The open device's PCI vendor id (0x5143 Qualcomm, 0x13B5 Arm, ...); 0 with no context.
std::uint32_t vendor_id();

// Split pass timing: each section timed on the GPU (stats::kGpu) ends the render pass it falls
// in, at its start and at its end, so that its timestamps lie between passes. A tile-based GPU
// runs a pass's draws together, so timestamps inside one pass read about zero; split, every
// timed section reads its own time, at the price of the attachments each split stores and loads
// again (the frame is slower while it is on). Off by default; OPENRAC_GPU_PASS_TIMING turns it on.
// Without it, a performance capture splits one frame in a few (stats::set_pass_sampling).
bool split_pass_timing();

}  // namespace openrac::vkgl
