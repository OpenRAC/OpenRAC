// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Included inside vkgl_draw.cpp: the native material path leaves GL shader sources unchanged.

namespace {

using namespace shader_transforms;
}  // namespace

bool material_enabled() {
    static const bool& off = stats::probe(
        "OPENRAC_GPU_MATERIALS_OFF",
        "Reference: individual mesh material uniforms and textures (requires shader reload)"
    );
    return g->bindless_supported && !off;
}

bool material_layout() {
    if (g->material_layout) {
        return true;
    }
    const VkDescriptorSetLayoutBinding bindings[] =
        {{0,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          g->material_texture_capacity,
          VK_SHADER_STAGE_FRAGMENT_BIT,
          nullptr},
         {1,
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          1,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          nullptr}};
    const VkDescriptorBindingFlags flags[] = {
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, 0
    };
    VkDescriptorSetLayoutBindingFlagsCreateInfo options{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
    };
    options.bindingCount = 2;
    options.pBindingFlags = flags;
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = &options;
    info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    info.bindingCount = 2;
    info.pBindings = bindings;
    return check(
        vkCreateDescriptorSetLayout(g->device, &info, nullptr, &g->material_layout),
        "bindless material layout"
    );
}

bool material_takes(const std::string& vertex, std::string_view name) {
    if (!material_enabled() || vertex.find("in mat4 model;") == std::string::npos) {
        return false;
    }
    const bool field = std::any_of(
        std::begin(kMaterialFields),
        std::end(kMaterialFields),
        [&](const MaterialField& f) { return name == f.name; }
    );
    return field && material_per_draw(material_specialised(vertex), name);
}

// Compiles mesh shaders with per-draw material fields and bindless texture selection.
bool material_program(Program& p, const std::string& vertex, const std::string& fragment) {
    p.gpu_materials = false;
    if (p.compiled.fallback != GraphicsFallback::None || !material_enabled()
        || vertex.find("in mat4 model;") == std::string::npos) {
        return true;
    }
    const auto& original = p.compiled;
    if (std::none_of(original.uniforms.begin(), original.uniforms.end(), [](const auto& u) {
            return u.name == "material_colour" || u.name == "shadow_view";
        })) {
        return true;
    }
    if (!material_layout()) {
        return false;
    }

    std::string vs, fs;
    if (!shader_transforms::material_sources(original, vertex, fragment, vs, fs, p.log)) {
        return false;
    }
    CompiledProgram compiled;
    if (!compile_program(vs, fs, compiled, p.log)) {
        return false;
    }
    if (compiled.fallback == GraphicsFallback::Fullscreen) {
        p.compiled = std::move(compiled);
        return true;
    }
    std::erase_if(compiled.uniforms, [](const auto& u) { return u.name == "openrac_textures"; });
    std::erase_if(compiled.storage, [](const auto& b) { return b.name == "OpenracMaterials"; });
    std::erase_if(compiled.blocks, [](const auto& b) { return b.name == "OpenracDrawBase"; });
    p.material_start = (static_cast<unsigned>(std::max(compiled.default_size, 0)) + 15u) & ~15u;
    for (auto u : original.uniforms) {
        // Preserve GL sampler locations while moving their bindings into the material array.
        if (u.sampler && material_sampler(u.name)) {
            u.binding = -2;
            compiled.uniforms.push_back(std::move(u));
            continue;
        }
        for (const auto& field : kMaterialFields) {
            if (material_per_draw(material_specialised(vertex), field.name)
                && u.name == field.name) {
                u.offset = static_cast<int>(p.material_start + field.offset);
                compiled.uniforms.push_back(std::move(u));
                break;
            }
        }
    }
    p.compiled = std::move(compiled);
    p.gpu_materials = true;
    return true;
}

void material_reset(Frame& f) {
    f.material_pages_used = 0;
    for (auto& page : f.material_pages) {
        page.draws = 0;
        page.materials.clear();
        page.textures.clear();
    }
}

std::array<std::uint64_t, 2> material_texture_key(const Sampled& sample) {
    return {
        reinterpret_cast<std::uint64_t>(sample.view),
        reinterpret_cast<std::uint64_t>(sample.sampler)
    };
}

namespace {

// Checks page capacity without counting a repeated sampler/image pair more than once.
bool material_page_fits(
    const MaterialPage& page, std::span<const Sampled> textures, unsigned draws
) {
    unsigned additional = 0;

    // Several material roles can use the same image and sampler.
    for (std::size_t n = 0; n < textures.size(); ++n) {
        const auto& sample = textures[n];

        // Missing roles consume no texture descriptor, and existing entries already fit.
        if (!sample.image || page.textures.contains(material_texture_key(sample))) {
            continue;
        }

        const auto key = material_texture_key(sample);
        // Called immediately on the graphics owner to detect earlier roles with identical sampling.
        const bool repeated =
            std::any_of(textures.begin(), textures.begin() + n, [&](const auto& earlier) {
                return earlier.image && material_texture_key(earlier) == key;
            });
        additional += repeated ? 0u : 1u;
    }

    return draws <= kMaterialDraws - page.draws
           && page.textures.size() + additional <= g->material_texture_capacity;
}

}  // namespace

// Returns a fence-owned page with room for the draw and its distinct material textures.
MaterialPage& material_page(std::span<const Sampled> textures, unsigned draws) {
    auto& f = frame();
    if (!f.material_pages_used
        || !material_page_fits(f.material_pages[f.material_pages_used - 1], textures, draws)) {
        if (f.material_pages_used == f.material_pages.size()) {
            auto& page = f.material_pages.emplace_back();
            page.data = gpu_buffer(
                VkDeviceSize(kMaterialDraws) * (4 + kMaterialBytes),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                true
            );
            const VkDescriptorPoolSize sizes[] =
                {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, g->material_texture_capacity},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
            VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            pool.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
            pool.maxSets = 1;
            pool.poolSizeCount = 2;
            pool.pPoolSizes = sizes;
            if (!check(
                    vkCreateDescriptorPool(g->device, &pool, nullptr, &page.pool), "material pool"
                )) {
                throw std::runtime_error("cannot allocate material pool");
            }
            VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            alloc.descriptorPool = page.pool;
            alloc.descriptorSetCount = 1;
            alloc.pSetLayouts = &g->material_layout;
            if (!check(
                    vkAllocateDescriptorSets(g->device, &alloc, &page.set), "material descriptors"
                )) {
                throw std::runtime_error("cannot allocate material descriptors");
            }
            ++g->submission.descriptor_sets_allocated;
            const VkDescriptorBufferInfo buffer{page.data.buffer, 0, page.data.size};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = page.set;
            write.dstBinding = 1;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &buffer;
            vkUpdateDescriptorSets(g->device, 1, &write, 0, nullptr);
            ++g->submission.descriptor_writes;
        }
        ++f.material_pages_used;
    }
    return f.material_pages[f.material_pages_used - 1];
}

unsigned material_texture(MaterialPage& page, const Sampled& sample) {
    validate_material_sampler(sample);
    const auto key = material_texture_key(sample);
    const auto [index, added] = page.textures.intern(key);
    if (added) {
        const VkDescriptorImageInfo image{
            sample.sampler, sample.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = page.set;
        write.dstBinding = 0;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        vkUpdateDescriptorSets(g->device, 1, &write, 0, nullptr);
        ++g->submission.descriptor_writes;
        ++g->material_textures;
    }
    return index;
}

struct MaterialBinding {
    VkDescriptorSet set = VK_NULL_HANDLE;
    unsigned draw = 0;
};

// Resolves material textures and retains an immutable record for this draw.
MaterialBinding material_binding(Program& p, const Target& target, unsigned draws) {
    if (!p.gpu_materials) {
        return {};
    }
    // Declare the effective sampled images before publishing native descriptors.
    std::array<Sampled, kMaterialSamplers.size()> textures{};
    for (std::size_t i = 0; i < p.compiled.uniforms.size(); ++i) {
        const auto& u = p.compiled.uniforms[i];
        if (!u.sampler || u.binding != -2) {
            continue;
        }
        auto sample = sampled(p, i);
        if (sample.image == target.colour || sample.image == target.depth) {
            sample.image = g->dummy_image.get();
            sample.view = sample_view(*sample.image, 0, 0);
            sample.range = sampled_range(*sample.image, 0, 0);
        }
        const auto slot = std::find(kMaterialSamplers.begin(), kMaterialSamplers.end(), u.name);
        textures[static_cast<std::size_t>(slot - kMaterialSamplers.begin())] = sample;
    }
    GraphImmediateWork::prepare_material_reads(textures);
    auto& page = material_page(textures, draws);
    std::array<std::uint32_t, kMaterialWords> record;
    std::memcpy(record.data(), p.uniforms.data() + p.material_start, kMaterialBytes);
    // Depth/shadow programs can omit emission and upscaled colour sampling.
    for (std::size_t n = 0; n < textures.size(); ++n) {
        record[kMaterialTextureWords[n]] =
            textures[n].image ? material_texture(page, textures[n]) : 0;
    }
    const auto [index, added] = page.materials.intern(record);
    if (added) {
        std::memcpy(
            page.data.mapped + kMaterialDraws * 4 + index * kMaterialBytes,
            record.data(),
            kMaterialBytes
        );
        ++g->material_records;
    }
    // Each indirect command advances gl_DrawID, even when all ranges share one material.
    const unsigned draw = page.draws;
    for (unsigned i = 0; i < draws; ++i) {
        std::memcpy(page.data.mapped + (draw + i) * 4, &index, 4);
    }
    page.draws += draws;
    g->material_draws += draws;
    return {page.set, draw};
}

void material_release() {
    for (auto& f : g->frames) {
        for (auto& page : f.material_pages) {
            if (page.pool) {
                vkDestroyDescriptorPool(g->device, page.pool, nullptr);
            }
            if (page.data.buffer) {
                vmaDestroyBuffer(g->allocator, page.data.buffer, page.data.allocation);
            }
        }
    }
    if (g->material_layout) {
        vkDestroyDescriptorSetLayout(g->device, g->material_layout, nullptr);
    }
}
