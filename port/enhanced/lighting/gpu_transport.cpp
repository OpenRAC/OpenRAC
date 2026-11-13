// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Vulkan ray-query transport: scene tables, one BLAS/TLAS, and chunked probe dispatches.
#include "enhanced/lighting/gpu_transport.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <volk.h>

#include "common/log.h"
#include "enhanced/lighting/bake_math.h"
#include "enhanced/lighting/gpu_probe_trace_source.h"
#include "renderer/graph/command_buffer.h"
#include "renderer/graph/recording_scope.h"
#include "renderer/vulkan/commands/graph_queue.h"
#include "renderer/vulkan/commands/native_command_encoder.h"
#include "renderer/vulkan/graph/graph_buffer_execution.h"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

namespace openrac::lighting::bake_detail {
namespace {
namespace graph = renderer::graph;
constexpr std::uint32_t kBindings = 20;
constexpr std::uint32_t kMaxPairs = 64;  // placement direction pairs per query
constexpr std::uint32_t kLocalSize = 32;
constexpr std::uint32_t kChunk = 1024;  // probes per dispatch: bounded memory and GPU time

// Shared submission ownership also cleans up partially constructed allocations.
struct BufferAllocation {
    VkDevice device;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    explicit BufferAllocation(VkDevice owner) : device(owner) {}

    BufferAllocation(const BufferAllocation&) = delete;
    BufferAllocation& operator=(const BufferAllocation&) = delete;

    ~BufferAllocation() {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
};

void check(VkResult result, const char* what) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(
            std::string("GPU bake: ") + what + " failed (" + std::to_string(int(result)) + ")"
        );
    }
}

// ---- Shader tables (std430, mirrored in gpu_probe_trace.comp) ----
using Vec4 = std::array<float, 4>;
using UVec4 = std::array<std::uint32_t, 4>;
using IVec4 = std::array<std::int32_t, 4>;

Vec4 v4(Float3 v, float w = 0) {
    return {v[0], v[1], v[2], w};
}

float bits(std::int32_t value) {
    float f;
    std::memcpy(&f, &value, sizeof f);
    return f;
}

struct Material {
    Vec4 diffuse, uv01, uv2;
    std::array<Vec4, 3> multiplier;
    IVec4 extra;
};

struct Terrain {
    std::array<Vec4, 3> colour;
};

struct Emission {
    Vec4 emission;
    std::array<Vec4, 3> colour;
    IVec4 flags;
    Vec4 scale;
};

struct Emitter {
    Vec4 normal_area;
    UVec4 kind_source;
    Vec4 weights;
};

struct AreaLight {
    Vec4 position, axis_u, axis_v, radiance;
};

struct Rectangle {
    Vec4 position, normal, dual_u, dual_v, radiance, area_pdf;
};

struct PointLight {
    Vec4 position, intensity;
};

struct Sun {
    Vec4 to_sun, irradiance;
};

struct Params {
    Vec4 environment, primary_colour, sunlight_scale, albedo;
    UVec4 counts, more;
    Vec4 emission_weight;
    UVec4 groups;
};

struct Push {
    std::uint32_t first, count;
    float max_distance;
    std::uint32_t pair_count;
    float cap;
    std::uint32_t pad[3];
};

static_assert(sizeof(Material) == 112 && sizeof(Emission) == 96 && sizeof(Emitter) == 48);
static_assert(sizeof(Rectangle) == 96 && sizeof(Params) == 128);

// The CPU's per-source gain, the last containing region winning (bake_emitters.cpp).
float emission_scale(const BakeTriangle& triangle, const BakeSettings& settings) {
    float scale = settings.surface_emission_scale;

    for (const auto& region : settings.surface_emission_regions) {
        bool inside = true;

        for (const auto& p : triangle.vertices) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                inside =
                    inside && p[axis] >= region.minimum[axis] && p[axis] <= region.maximum[axis];
            }
        }

        if (inside) {
            scale = region.scale;
        }
    }

    return scale;
}

// The placement kernel is the same source with OPENRAC_PLACEMENT defined.
std::vector<std::uint32_t> compile(bool placement, std::string& error) {
    static std::once_flag once;
    std::call_once(once, [] { glslang::InitializeProcess(); });
    glslang::TShader shader(EShLangCompute);
    const char* text = kGpuProbeTraceSource;
    shader.setStrings(&text, 1);
    shader.setPreamble(placement ? "#define OPENRAC_PLACEMENT 1\n" : "");
    shader.setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_4);
    const auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(GetDefaultResources(), 460, ECoreProfile, false, false, messages)) {
        error = shader.getInfoLog();
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        error = program.getInfoLog();
        return {};
    }

    std::vector<std::uint32_t> code;
    glslang::SpvOptions options;
    options.disableOptimizer = false;
    glslang::GlslangToSpv(*program.getIntermediate(EShLangCompute), code, &options);
    return code;
}
}  // namespace

struct GpuTransport::Impl {
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        void* mapped = nullptr;
        VkDeviceAddress address = 0;
        VkBufferUsageFlags usage = 0;
        std::shared_ptr<void> lifetime;
    };

    const Scene& scene;
    const BakeSettings& settings;
    std::string name;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    std::uint32_t family = 0;
    VkPhysicalDeviceMemoryProperties memory{};
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE, placement = VK_NULL_HANDLE;
    std::mutex mutex;  // one query on the device at a time; volumes may bake concurrently
    double trace_seconds = 0, place_seconds = 0;  // device time waited for, guarded by mutex
    std::uint64_t traced_probes = 0, placed = 0;
    VkDescriptorPool descriptors = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkAccelerationStructureKHR blas = VK_NULL_HANDLE, tlas = VK_NULL_HANDLE;
    std::vector<Buffer> owned;  // every buffer, released with the device
    renderer::graph::SubmissionLifetime submission_lifetime;
    std::array<Buffer, kBindings> bound{};
    std::uint32_t emitter_count = 0;

    Impl(const Scene& s, const BakeSettings& b) : scene(s), settings(b) {}

    ~Impl() {
        if (device) {
            vkDeviceWaitIdle(device);

            if (blas) {
                vkDestroyAccelerationStructureKHR(device, blas, nullptr);
            }
            if (tlas) {
                vkDestroyAccelerationStructureKHR(device, tlas, nullptr);
            }
            const auto completed = submission_lifetime.complete(true);
            (void)completed;
            bound = {};
            owned.clear();
            vkDestroyDescriptorPool(device, descriptors, nullptr);
            vkDestroyPipeline(device, pipeline, nullptr);
            vkDestroyPipeline(device, placement, nullptr);
            vkDestroyPipelineLayout(device, layout, nullptr);
            vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
            vkDestroyFence(device, fence, nullptr);
            vkDestroyCommandPool(device, pool, nullptr);
            vkDestroyDevice(device, nullptr);
        }

        if (instance) {
            vkDestroyInstance(instance, nullptr);
        }
    }

    // ---- Device ----
    bool open(std::string& reason) {
        if (volkInitialize() != VK_SUCCESS) {
            reason = "no Vulkan loader";
            return false;
        }

        VkApplicationInfo app{};

        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "openrac-probe-bake";
        app.apiVersion = VK_API_VERSION_1_2;
        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &app;

        if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
            reason = "no Vulkan 1.2 instance";
            return false;
        }

        volkLoadInstanceOnly(instance);
        std::uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());
        int best = -1;

        for (auto candidate : devices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(candidate, &properties);
            std::uint32_t extension_count = 0;
            vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extension_count, nullptr);
            std::vector<VkExtensionProperties> extensions(extension_count);
            vkEnumerateDeviceExtensionProperties(
                candidate, nullptr, &extension_count, extensions.data()
            );
            auto has = [&](const char* wanted) {
                return std::any_of(extensions.begin(), extensions.end(), [&](const auto& e) {
                    return std::strcmp(e.extensionName, wanted) == 0;
                });
            };

            if (properties.apiVersion < VK_API_VERSION_1_2 || !has(VK_KHR_RAY_QUERY_EXTENSION_NAME)
                || !has(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                || !has(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) {
                continue;
            }

            const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU     ? 4
                              : properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 3
                              : properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU            ? 1
                                                                                                : 2;

            if (score > best) {
                best = score;
                physical = candidate;
                name = properties.deviceName;
            }
        }

        if (!physical) {
            reason = "no Vulkan device with ray queries";
            return false;
        }

        std::uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, families.data());
        family = family_count;

        for (std::uint32_t i = 0; i < family_count; ++i) {
            if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                family = i;
                break;
            }
        }

        if (family == family_count) {
            reason = "no compute queue";
            return false;
        }

        VkPhysicalDeviceRayQueryFeaturesKHR ray_query{};

        ray_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ray_query.rayQuery = VK_TRUE;
        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration{};
        acceleration.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        acceleration.accelerationStructure = VK_TRUE;
        acceleration.pNext = &ray_query;
        VkPhysicalDeviceVulkan12Features vulkan12{};
        vulkan12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12.bufferDeviceAddress = VK_TRUE;
        vulkan12.pNext = &acceleration;
        const float priority = 1;
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        const char* extensions[] = {
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        };
        VkDeviceCreateInfo device_info{};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = &vulkan12;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = 3;
        device_info.ppEnabledExtensionNames = extensions;

        if (vkCreateDevice(physical, &device_info, nullptr, &device) != VK_SUCCESS) {
            reason = "could not create the ray-query device";
            return false;
        }

        volkLoadDevice(device);
        vkGetDeviceQueue(device, family, 0, &queue);
        vkGetPhysicalDeviceMemoryProperties(physical, &memory);
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = family;
        check(vkCreateCommandPool(device, &pool_info, nullptr, &pool), "command pool");
        VkCommandBufferAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate.commandPool = pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        check(vkAllocateCommandBuffers(device, &allocate, &commands), "command buffer");
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        check(vkCreateFence(device, &fence_info, nullptr, &fence), "fence");
        return true;
    }

    std::uint32_t memory_type(std::uint32_t allowed, VkMemoryPropertyFlags wanted) const {
        for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
            if ((allowed & (1u << i)) && (memory.memoryTypes[i].propertyFlags & wanted) == wanted) {
                return i;
            }
        }

        throw std::runtime_error("GPU bake: no suitable memory type");
    }

    // Host-visible buffers keep upload and readback to a memcpy; the bake reads each value once.
    Buffer make_buffer(VkDeviceSize size, VkBufferUsageFlags usage, bool host = true) {
        Buffer b;
        auto allocation = std::make_shared<BufferAllocation>(device);
        b.size = std::max<VkDeviceSize>(size, 16);
        b.usage = usage;
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = b.size;
        info.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        check(vkCreateBuffer(device, &info, nullptr, &allocation->buffer), "buffer");
        b.buffer = allocation->buffer;
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device, b.buffer, &requirements);
        VkMemoryAllocateFlagsInfo flags{};
        flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        VkMemoryAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate.pNext = &flags;
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = memory_type(
            requirements.memoryTypeBits,
            host ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                 : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        check(vkAllocateMemory(device, &allocate, nullptr, &allocation->memory), "memory");
        b.memory = allocation->memory;
        check(vkBindBufferMemory(device, b.buffer, b.memory, 0), "bind memory");

        if (host) {
            check(vkMapMemory(device, b.memory, 0, VK_WHOLE_SIZE, 0, &b.mapped), "map");
        }

        VkBufferDeviceAddressInfo address{};

        address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        address.buffer = b.buffer;
        b.address = vkGetBufferDeviceAddress(device, &address);
        b.lifetime = std::move(allocation);
        owned.push_back(b);
        return b;
    }

    template <class T>
    Buffer upload(const std::vector<T>& data, VkBufferUsageFlags usage) {
        auto b = make_buffer(data.size() * sizeof(T), usage);
        if (!data.empty()) {
            std::memcpy(b.mapped, data.data(), data.size() * sizeof(T));
        }
        return b;
    }

    static graph::BufferHandle graph_buffer(
        graph::Graph& builder,
        const Buffer& buffer,
        bool defined,
        graph::State initial,
        graph::BufferKind kind = graph::BufferKind::Data
    ) {
        if (buffer.usage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR) {
            kind = graph::BufferKind::AccelerationStructure;
        }
        const auto handle = builder.import_buffer(
            "baker buffer",
            {{std::bit_cast<std::uint64_t>(buffer.buffer), 1}, buffer.size, initial, defined, kind}
        );
        if (!handle) {
            throw std::runtime_error(handle.error().message);
        }
        return *handle;
    }

    static graph::Plan compile_graph(const graph::Graph& builder) {
        const auto plan = builder.compile(graph::DeclarationRetention::Retain);
        if (!plan) {
            throw std::runtime_error(plan.error().message);
        }
        return *plan;
    }

    void submit(const std::function<void(VkCommandBuffer)>& record) {
        check(vkResetCommandBuffer(commands, 0), "reset commands");
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(commands, &begin), "begin commands");
        for (const auto& buffer : owned) {
            submission_lifetime.retain(buffer.lifetime);
        }
        record(commands);
        check(vkEndCommandBuffer(commands), "end commands");
        renderer::graph::SubmissionGraph scheduler;
        scheduler.add_stream({"probe bake", std::bit_cast<std::uint64_t>(commands)});
        const auto schedule = scheduler.compile();
        if (!schedule) {
            throw std::runtime_error(schedule.error().message);
        }
        check(vkResetFences(device, 1, &fence), "reset fence");
        check(vkgl::submit_graph_queue(queue, *schedule, fence), "submit");
        submission_lifetime.submitted();
        check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "wait");
        const auto completed = submission_lifetime.complete(true);
        if (!completed) {
            throw std::runtime_error(completed.error().message);
        }
    }

    // ---- Acceleration ----
    VkAccelerationStructureKHR build(
        VkAccelerationStructureTypeKHR type,
        std::vector<VkAccelerationStructureGeometryKHR>& geometries,
        const std::vector<std::uint32_t>& primitives
    ) {
        VkAccelerationStructureBuildGeometryInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        info.type = type;
        info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        info.geometryCount = static_cast<std::uint32_t>(geometries.size());
        info.pGeometries = geometries.data();
        VkAccelerationStructureBuildSizesInfoKHR sizes{};
        sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &info,
            primitives.data(),
            &sizes
        );
        const auto storage = make_buffer(
            sizes.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            false
        );
        const auto scratch =
            make_buffer(sizes.buildScratchSize + 256, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);
        VkAccelerationStructureCreateInfoKHR create{};
        create.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create.buffer = storage.buffer;
        create.size = sizes.accelerationStructureSize;
        create.type = type;
        VkAccelerationStructureKHR acceleration;
        check(
            vkCreateAccelerationStructureKHR(device, &create, nullptr, &acceleration),
            "acceleration structure"
        );
        info.dstAccelerationStructure = acceleration;
        info.scratchData.deviceAddress = (scratch.address + 255) & ~VkDeviceAddress{255};
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;

        for (auto count : primitives) {
            ranges.push_back({count, 0, 0, 0});
        }

        const auto* range = ranges.data();
        graph::Graph builder;
        graph::Pass pass{"build acceleration structure"};
        pass.execution = graph::Stage::AccelerationBuild;
        for (const auto& input : owned) {
            if (input.buffer == storage.buffer || input.buffer == scratch.buffer
                || !(
                    input.usage
                    & (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
                       | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
                )) {
                continue;
            }
            const auto stage = input.mapped ? graph::Stage::Host : graph::Stage::AccelerationBuild;
            const auto handle = graph_buffer(
                builder, input, true, {stage, graph::Access::Write, graph::Layout::Buffer}
            );
            pass.buffers.push_back(
                {handle,
                 {0, input.size},
                 {graph::Stage::AccelerationBuild, graph::Access::Read, graph::Layout::Buffer}}
            );
        }
        for (const auto& output : {storage, scratch}) {
            const auto handle = graph_buffer(
                builder,
                output,
                false,
                {graph::Stage::None, graph::Access::None, graph::Layout::Buffer},
                output.buffer == scratch.buffer ? graph::BufferKind::AccelerationScratch
                                                : graph::BufferKind::AccelerationStructure
            );
            pass.buffers.push_back(
                {handle,
                 {0, output.size},
                 {graph::Stage::AccelerationBuild, graph::Access::Write, graph::Layout::Buffer}}
            );
        }
        builder.add_pass(std::move(pass));
        const auto plan = compile_graph(builder);
        submit([&](VkCommandBuffer c) {
            vkgl::record_buffer_graph(c, plan, [&](unsigned) {
                vkgl::NativeCommandEncoder{c}.build_acceleration_structures(1, &info, &range);
            });
        });
        return acceleration;
    }

    void build_acceleration() {
        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<std::uint32_t> primitives;

        for (unsigned group = 0; group < 2; ++group) {
            const auto& ids = scene.triangle_ids[group];

            if (ids.empty()) {
                continue;
            }

            std::vector<float> vertices;
            vertices.reserve(ids.size() * 9);

            for (auto id : ids) {
                for (const auto& p : scene.input.triangles[id].vertices) {
                    vertices.insert(vertices.end(), p.begin(), p.end());
                }
            }

            const auto buffer = upload(
                vertices, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            );
            VkAccelerationStructureGeometryKHR geometry{};
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.flags = group == 0 ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;
            auto& triangles = geometry.geometry.triangles;
            triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            triangles.vertexData.deviceAddress = buffer.address;
            triangles.vertexStride = 12;
            triangles.maxVertex = static_cast<std::uint32_t>(ids.size() * 3 - 1);
            triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
            geometries.push_back(geometry);
            primitives.push_back(static_cast<std::uint32_t>(ids.size()));
        }

        if (geometries.empty()) {
            throw std::runtime_error("GPU bake: the scene has no traceable triangles");
        }

        blas = build(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, geometries, primitives);
        VkAccelerationStructureDeviceAddressInfoKHR address{};
        address.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        address.accelerationStructure = blas;
        VkAccelerationStructureInstanceKHR instance_data{};
        instance_data.transform.matrix[0][0] = 1;
        instance_data.transform.matrix[1][1] = 1;
        instance_data.transform.matrix[2][2] = 1;
        instance_data.mask = 0xFF;
        instance_data.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance_data.accelerationStructureReference =
            vkGetAccelerationStructureDeviceAddressKHR(device, &address);
        const auto instances = upload(
            std::vector<VkAccelerationStructureInstanceKHR>{instance_data},
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        );
        VkAccelerationStructureGeometryKHR top{};
        top.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        top.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        top.geometry.instances.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        top.geometry.instances.data.deviceAddress = instances.address;
        std::vector<VkAccelerationStructureGeometryKHR> tops{top};
        tlas = build(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, tops, {1});
    }

    // ---- Scene tables ----
    void build_tables() {
        constexpr auto kStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        const auto& input = scene.input;
        const auto triangle_count = input.triangles.size();
        std::vector<float> positions;
        std::vector<Material> materials;
        std::vector<Terrain> terrains;
        std::vector<Emission> emissions;
        positions.reserve(triangle_count * 9);
        materials.reserve(triangle_count);
        std::vector<std::int32_t> emitter_of(triangle_count, -1);

        for (std::size_t i = 0; i < scene.emitters.size(); ++i) {
            if (scene.emitters[i].triangle < triangle_count) {
                emitter_of[scene.emitters[i].triangle] = static_cast<std::int32_t>(i);
            }
        }

        for (std::size_t i = 0; i < triangle_count; ++i) {
            const auto& t = input.triangles[i];

            for (const auto& p : t.vertices) {
                positions.insert(positions.end(), p.begin(), p.end());
            }

            Material m{};
            m.diffuse = v4(t.diffuse, t.diffuse_scale);
            m.uv01 = {t.uv[0][0], t.uv[0][1], t.uv[1][0], t.uv[1][1]};
            m.uv2 = {t.uv[2][0], t.uv[2][1], bits(t.texture), t.alpha_cutoff};

            for (unsigned k = 0; k < 3; ++k) {
                m.multiplier[k] = t.multipliers[k];
            }

            m.extra = {-1, -1, emitter_of[i], 0};

            if (t.terrain_prelighting) {
                m.extra[0] = static_cast<std::int32_t>(terrains.size());
                terrains.push_back(
                    {{v4(t.authored_colours[0]),
                      v4(t.authored_colours[1]),
                      v4(t.authored_colours[2])}}
                );
            }

            if (t.emission != Float3{}) {
                const auto index = emitter_of[i];
                const auto average =
                    index < 0 ? std::optional<Float3>{}
                              : scene.emitters[static_cast<std::size_t>(index)].average_emission;
                m.extra[1] = static_cast<std::int32_t>(emissions.size());
                emissions.push_back(
                    {v4(average.value_or(t.emission), t.emission_contrast),
                     {v4(t.emission_colours[0]),
                      v4(t.emission_colours[1]),
                      v4(t.emission_colours[2])},
                     {t.emission_texture,
                      t.emission_vertex_colour ? 1 : 0,
                      (average || t.mesh_emission) ? 1 : 0,
                      t.emission_uses_cutout ? 1 : 0},
                     {emission_scale(t, settings), 0, 0, 0}}
                );
            }

            materials.push_back(m);
        }

        std::vector<std::uint32_t> sources(scene.triangle_ids[0]);
        sources.insert(sources.end(), scene.triangle_ids[1].begin(), scene.triangle_ids[1].end());
        std::vector<Emitter> emitters;
        const auto points_start = triangle_count;
        const auto rectangles_start = triangle_count + input.point_emitters.size();

        for (std::size_t i = 0; i < scene.emitters.size(); ++i) {
            const auto& e = scene.emitters[i];
            const double previous = i == 0 ? 0 : scene.emitters[i - 1].cumulative;
            const double probability = (e.cumulative - previous) / scene.emission_weight;
            const std::uint32_t kind = e.triangle >= rectangles_start ? 2
                                       : e.triangle >= points_start   ? 1
                                                                      : 0;
            const auto source = static_cast<std::uint32_t>(
                kind == 2   ? e.triangle - rectangles_start
                : kind == 1 ? e.triangle - points_start
                            : e.triangle
            );
            emitters.push_back(
                {v4(e.normal, e.area),
                 {kind, source, 0, 0},
                 {static_cast<float>(e.cumulative),
                  static_cast<float>(probability),
                  e.area > 0 ? static_cast<float>(probability / e.area) : 0.f,
                  0}}
            );
        }

        emitter_count = static_cast<std::uint32_t>(emitters.size());
        std::vector<AreaLight> areas;

        for (const auto& a : input.area_emitters) {
            areas.push_back({v4(a.position), v4(a.axis_u), v4(a.axis_v), v4(a.radiance)});
        }

        std::vector<Rectangle> rectangles;

        for (const auto& r : scene.rectangles) {
            rectangles.push_back(
                {v4(r.position),
                 v4(r.normal),
                 v4(r.dual_u),
                 v4(r.dual_v),
                 v4(r.radiance),
                 {static_cast<float>(r.area_pdf), 0, 0, 0}}
            );
        }

        std::vector<PointLight> points;

        for (const auto& p : input.point_emitters) {
            points.push_back({v4(p.position), v4(p.intensity)});
        }

        std::vector<Sun> suns;

        for (std::size_t i = 0; i < input.suns.size(); ++i) {
            suns.push_back(
                {v4(scene.to_suns[i], input.suns[i].primary ? 1.f : 0.f),
                 v4(input.suns[i].irradiance)}
            );
        }

        std::vector<UVec4> textures;
        std::vector<std::uint32_t> texels;

        for (const auto& t : input.textures) {
            textures.push_back({static_cast<std::uint32_t>(texels.size()), t.width, t.height, 0});
            const auto offset = texels.size();
            texels.resize(offset + t.rgba.size() / 4);
            std::memcpy(texels.data() + offset, t.rgba.data(), t.rgba.size());
        }

        std::vector<Vec4> sky;

        for (const auto& s : input.sky_radiance) {
            sky.push_back(v4(s));
        }

        std::vector<Vec4> primary;
        const auto rays = settings.rays_per_probe;

        // The CPU's spiral (bake.cpp BakeDirections), computed with the same float steps.
        for (std::uint32_t i = 0; i < rays; ++i) {
            const float z = 1 - 2 * (static_cast<float>(i) + 0.5f) / static_cast<float>(rays);
            primary.push_back(
                {z, std::sqrt(std::max(0.0f, 1 - z * z)), static_cast<float>(i) * 2.39996323f, 0}
            );
        }

        Params params{};
        params.environment = v4(input.environment, settings.use_level_sky ? 1.f : 0.f);
        params.primary_colour = v4(input.primary_directional_colour, settings.sky_weight);
        params.sunlight_scale = v4(settings.sunlight_scale, settings.sky_intensity);
        params.albedo =
            {settings.terrain_vertex_albedo ? 1.f : 0.f,
             settings.terrain_vertex_albedo_strength,
             settings.ray_bias,
             0};
        params.counts = {rays, settings.bounces, settings.seed, emitter_count};
        params.more =
            {static_cast<std::uint32_t>(suns.size()),
             static_cast<std::uint32_t>(rectangles.size()),
             input.sky_radiance.empty() ? 0u : input.sky_width,
             input.sky_radiance.empty() ? 0u : input.sky_height};
        params.emission_weight = {static_cast<float>(scene.emission_weight), 0, 0, 0};
        params.groups = {static_cast<std::uint32_t>(scene.triangle_ids[0].size()), 0, 0, 0};

        bound[1] = upload(positions, kStorage);
        bound[2] = upload(materials, kStorage);
        bound[3] = upload(sources, kStorage);
        bound[4] = upload(terrains, kStorage);
        bound[5] = upload(emissions, kStorage);
        bound[6] = upload(emitters, kStorage);
        bound[7] = upload(areas, kStorage);
        bound[8] = upload(rectangles, kStorage);
        bound[9] = upload(points, kStorage);
        bound[10] = upload(suns, kStorage);
        bound[11] = upload(textures, kStorage);
        bound[12] = upload(texels, kStorage);
        bound[13] = upload(sky, kStorage);
        bound[14] = upload(primary, kStorage);
        bound[18] = upload(std::vector<Params>{params}, kStorage);
        const VkDeviceSize ray_bytes = VkDeviceSize{kChunk} * rays * 2 * sizeof(Vec4);
        bound[15] = make_buffer(VkDeviceSize{kChunk} * sizeof(Vec4), kStorage);
        bound[16] = make_buffer(ray_bytes, kStorage);
        bound[17] = make_buffer(emitter_count ? ray_bytes : 16, kStorage);
        bound[19] = make_buffer(VkDeviceSize{kMaxPairs} * 2 * sizeof(Vec4), kStorage);
    }

    // ---- Pipeline ----
    VkPipeline make_pipeline(bool placing) {
        std::string error;
        const auto code = compile(placing, error);

        if (code.empty()) {
            throw std::runtime_error("GPU bake: shader: " + error);
        }

        VkShaderModuleCreateInfo module_info{};
        module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_info.codeSize = code.size() * 4;
        module_info.pCode = code.data();
        VkShaderModule module;
        check(vkCreateShaderModule(device, &module_info, nullptr, &module), "shader module");
        VkComputePipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipeline_info.stage.module = module;
        pipeline_info.stage.pName = "main";
        pipeline_info.layout = layout;
        VkPipeline created_pipeline = VK_NULL_HANDLE;
        const auto created = vkCreateComputePipelines(
            device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &created_pipeline
        );
        vkDestroyShaderModule(device, module, nullptr);
        check(created, "compute pipeline");
        return created_pipeline;
    }

    // Runs one bound kernel over count invocations and waits for its results on the host.
    void dispatch(VkPipeline kernel, const Push& push) {
        graph::Graph builder;
        graph::Pass pass{kernel == placement ? "probe placement" : "probe trace"};
        pass.execution = graph::Stage::ComputeShader;
        graph::Pass host{"baker host readback", {}, {}, graph::PassKind::Export};
        for (unsigned index = 1; index < bound.size(); ++index) {
            if ((index == 17 && (kernel == placement || !emitter_count))
                || (index == 19 && kernel != placement)) {
                continue;
            }
            const auto& buffer = bound[index];
            const bool output = index == 16 || index == 17;
            const auto handle = graph_buffer(
                builder,
                buffer,
                !output,
                {output ? graph::Stage::ComputeShader : graph::Stage::Host,
                 graph::Access::Write,
                 graph::Layout::Buffer}
            );
            auto size = buffer.size;
            if (output) {
                size = VkDeviceSize{push.count} * 2 * sizeof(Vec4)
                       * (kernel == placement ? 1 : settings.rays_per_probe);
            } else if (index == 15) {
                size = VkDeviceSize{push.count} * sizeof(Vec4);
            } else if (index == 19) {
                size = VkDeviceSize{push.pair_count} * 2 * sizeof(Vec4);
                if (!size) {
                    continue;
                }
            }
            pass.buffers.push_back(
                {handle,
                 {0, size},
                 {graph::Stage::ComputeShader,
                  output ? graph::Access::Write : graph::Access::Read,
                  graph::Layout::Buffer}}
            );
            if (output) {
                host.buffers.push_back(
                    {handle,
                     {0, size},
                     {graph::Stage::Host, graph::Access::Read, graph::Layout::Buffer}}
                );
            }
        }
        for (const auto& buffer : owned) {
            if (buffer.usage & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR) {
                const auto handle = graph_buffer(
                    builder,
                    buffer,
                    true,
                    {graph::Stage::AccelerationBuild, graph::Access::Write, graph::Layout::Buffer}
                );
                pass.buffers.push_back(
                    {handle,
                     {0, buffer.size},
                     {graph::Stage::ComputeShader, graph::Access::Read, graph::Layout::Buffer}}
                );
            }
        }
        builder.add_pass(std::move(pass));
        builder.add_pass(std::move(host));
        const auto plan = compile_graph(builder);
        const graph::Dispatch dispatch{{(push.count + kLocalSize - 1) / kLocalSize, 1, 1}};
        const auto valid = graph::validate_pipeline_command(plan.declarations[0], dispatch);
        if (!valid) {
            throw std::runtime_error(valid.error().message);
        }
        submit([&](VkCommandBuffer c) {
            vkgl::record_buffer_graph(c, plan, [&](unsigned) {
                vkgl::NativeCommandEncoder encoder{c};
                encoder.bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, kernel);
                encoder.bind_descriptor_sets(
                    VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, nullptr
                );
                encoder.push_constants(layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof push, &push);
                encoder.dispatch(dispatch.groups[0], dispatch.groups[1], dispatch.groups[2]);
            });
        });
    }

    void build_pipeline() {
        std::array<VkDescriptorSetLayoutBinding, kBindings> bindings{};

        for (std::uint32_t i = 0; i < kBindings; ++i) {
            bindings[i] =
                {i,
                 i == 0 ? VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
                        : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 1,
                 VK_SHADER_STAGE_COMPUTE_BIT,
                 nullptr};
        }

        VkDescriptorSetLayoutCreateInfo set_info{};

        set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_info.bindingCount = kBindings;
        set_info.pBindings = bindings.data();
        check(vkCreateDescriptorSetLayout(device, &set_info, nullptr, &set_layout), "set layout");
        const VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Push)};
        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &range;
        check(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout), "pipeline layout");
        pipeline = make_pipeline(false);
        placement = make_pipeline(true);

        const std::array<VkDescriptorPoolSize, 2> sizes{
            {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kBindings - 1}}
        };
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = 2;
        pool_info.pPoolSizes = sizes.data();
        check(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptors), "descriptor pool");
        VkDescriptorSetAllocateInfo allocate{};
        allocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate.descriptorPool = descriptors;
        allocate.descriptorSetCount = 1;
        allocate.pSetLayouts = &set_layout;
        check(vkAllocateDescriptorSets(device, &allocate, &set), "descriptor set");

        std::array<VkDescriptorBufferInfo, kBindings> infos{};
        std::array<VkWriteDescriptorSet, kBindings> writes{};
        VkWriteDescriptorSetAccelerationStructureKHR acceleration{};
        acceleration.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        acceleration.accelerationStructureCount = 1;
        acceleration.pAccelerationStructures = &tlas;

        for (std::uint32_t i = 0; i < kBindings; ++i) {
            writes[i] = {};
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;

            if (i == 0) {
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                writes[i].pNext = &acceleration;
            } else {
                infos[i] = {bound[i].buffer, 0, VK_WHOLE_SIZE};
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].pBufferInfo = &infos[i];
            }
        }

        vkUpdateDescriptorSets(device, kBindings, writes.data(), 0, nullptr);
    }
};

GpuTransport::GpuTransport(Key, std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

GpuTransport::~GpuTransport() {
    log::info(
        "GPU probe bake: {} probes traced in {:.2f} s, {} placement queries in {:.2f} s",
        m_impl->traced_probes,
        m_impl->trace_seconds,
        m_impl->placed,
        m_impl->place_seconds
    );
}

std::unique_ptr<GpuTransport> GpuTransport::create(
    const Scene& scene, const BakeSettings& settings
) {
    auto impl = std::make_unique<Impl>(scene, settings);
    std::string reason;

    if (!impl->open(reason)) {
        log::info("GPU probe bake unavailable: {}", reason);
        return nullptr;
    }

    impl->build_tables();
    impl->build_acceleration();
    impl->build_pipeline();
    log::info("GPU probe bake on {}", impl->name);
    return std::make_unique<GpuTransport>(Key{}, std::move(impl));
}

const std::string& GpuTransport::device_name() const {
    return m_impl->name;
}

void GpuTransport::sample(
    std::span<const Float3> origins,
    std::uint32_t first,
    float max_distance,
    std::span<RaySample> rays,
    std::span<EmissionSample> emission
) {
    auto& g = *m_impl;
    const std::lock_guard lock(g.mutex);
    const auto began = std::chrono::steady_clock::now();
    g.traced_probes += origins.size();
    const auto per_probe = g.settings.rays_per_probe;

    for (std::size_t start = 0; start < origins.size(); start += kChunk) {
        const auto count =
            static_cast<std::uint32_t>(std::min<std::size_t>(kChunk, origins.size() - start));
        auto* staged = static_cast<Vec4*>(g.bound[15].mapped);

        for (std::uint32_t i = 0; i < count; ++i) {
            staged[i] = v4(origins[start + i]);
        }

        g.dispatch(
            g.pipeline, {first + static_cast<std::uint32_t>(start), count, max_distance, 0, 0, {}}
        );

        const auto* out = static_cast<const Vec4*>(g.bound[16].mapped);
        const auto total = std::size_t{count} * per_probe;

        for (std::size_t i = 0; i < total; ++i) {
            const auto& d = out[2 * i];
            const auto& r = out[2 * i + 1];
            rays[start * per_probe + i] = {{d[0], d[1], d[2]}, {r[0], r[1], r[2]}, d[3]};
        }

        if (!emission.empty()) {
            const auto* lights = static_cast<const Vec4*>(g.bound[17].mapped);

            for (std::size_t i = 0; i < total; ++i) {
                const auto& d = lights[2 * i];
                const auto& r = lights[2 * i + 1];
                emission[start * per_probe + i] = {{d[0], d[1], d[2]}, {r[0], r[1], r[2]}};
            }
        }
    }

    g.trace_seconds +=
        std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
}

std::vector<GpuPlacement> GpuTransport::place(
    std::span<const Float3> positions, std::span<const std::array<Float3, 2>> pairs, float cap
) {
    auto& g = *m_impl;
    const std::lock_guard lock(g.mutex);
    const auto began = std::chrono::steady_clock::now();
    g.placed += positions.size();

    if (pairs.size() > kMaxPairs) {
        throw std::runtime_error("GPU bake: too many placement directions");
    }

    auto* directions = static_cast<Vec4*>(g.bound[19].mapped);

    for (std::size_t i = 0; i < pairs.size(); ++i) {
        directions[2 * i] = v4(pairs[i][0]);
        directions[2 * i + 1] = v4(pairs[i][1]);
    }

    std::vector<GpuPlacement> result(positions.size());

    for (std::size_t start = 0; start < positions.size(); start += kChunk) {
        const auto count =
            static_cast<std::uint32_t>(std::min<std::size_t>(kChunk, positions.size() - start));
        auto* staged = static_cast<Vec4*>(g.bound[15].mapped);

        for (std::uint32_t i = 0; i < count; ++i) {
            staged[i] = v4(positions[start + i]);
        }

        g.dispatch(g.placement, {0, count, 0, static_cast<std::uint32_t>(pairs.size()), cap, {}});
        const auto* out = static_cast<const Vec4*>(g.bound[16].mapped);

        for (std::uint32_t i = 0; i < count; ++i) {
            const auto& a = out[2 * i];
            const auto& b = out[2 * i + 1];
            result[start + i] = {a[3], {a[0], a[1], a[2]}, static_cast<std::uint32_t>(b[0]), b[1]};
        }
    }

    g.place_seconds +=
        std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    return result;
}
}  // namespace openrac::lighting::bake_detail
