// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Compact, checked extraction and posing for the developer material studio.
#include <algorithm>
#include <cmath>
#include <map>

#include "scene/object_preview.h"

namespace openrac::scene {
namespace {
// Normal-model transforms match the mesh shader, including nonuniform instance scales.
renderer::Vec3 normal_transform(const renderer::Mat4& m, renderer::Vec3 n) {
    const renderer::Vec3 a{m[0], m[1], m[2]}, b{m[4], m[5], m[6]}, c{m[8], m[9], m[10]};
    const auto x = renderer::cross(b, c), y = renderer::cross(c, a), z = renderer::cross(a, b);
    const float determinant = renderer::dot(a, x);
    renderer::Vec3 result{};

    // Degenerate instances have no meaningful transformed normal.
    if (std::abs(determinant) > 1e-12f) {
        for (unsigned i = 0; i < 3; ++i) {
            result[i] = (x[i] * n[0] + y[i] * n[1] + z[i] * n[2]) / determinant;
        }
    }

    return renderer::normalize(result);
}

// Copies one vertex through exactly the displayed weighted skin matrix, without world translation.
scene::Vertex pose_vertex(
    const scene::Vertex& source,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    scene::Vertex out = source;
    renderer::Vec3 p{source.position[0], source.position[1], source.position[2]};
    renderer::Vec3 n{source.normal[0], source.normal[1], source.normal[2]};
    float total = 0;

    for (float weight : source.weights) {
        total += weight;
    }

    // A negative palette or zero weights denotes rigid geometry, as in mesh.vert.
    if (instance.palette >= 0 && total > 0) {
        renderer::Mat4 skin{};

        for (unsigned j = 0; j < 4; ++j) {
            if (source.weights[j] == 0) {
                continue;
            }

            const auto& joint = palette[std::size_t(instance.palette) + source.joints[j]];

            for (unsigned c = 0; c < 16; ++c) {
                skin[c] += source.weights[j] * joint[c];
            }
        }

        p = renderer::transform_point(skin, p);
        n =
            {skin[0] * n[0] + skin[4] * n[1] + skin[8] * n[2],
             skin[1] * n[0] + skin[5] * n[1] + skin[9] * n[2],
             skin[2] * n[0] + skin[6] * n[1] + skin[10] * n[2]};
    }

    auto matrix = instance.matrix;
    matrix[12] = matrix[13] = matrix[14] = 0;  // Moving objects stay at the studio origin.
    p = renderer::transform_point(matrix, p);
    n = normal_transform(matrix, n);

    // Preserve per-instance colour and alpha independently of world lighting.
    for (unsigned c = 0; c < 4; ++c) {
        out.colour[c] *= instance.tint[c];
    }

    std::copy(p.begin(), p.end(), out.position);
    std::copy(n.begin(), n.end(), out.normal);
    return out;
}

// Preserve both colour and emissive maps while copying each referenced image only once.
void copy_image(
    int& image, const scene::LevelData& level, PreviewMesh& mesh, std::map<int, int>& remap
) {
    // Missing images keep the renderer's opaque white fallback.
    if (image < 0 || std::size_t(image) >= level.images.size()) {
        image = -1;
        return;
    }

    const auto [found, inserted] = remap.emplace(image, int(mesh.images.size()));

    if (inserted) {
        mesh.images.push_back(level.images[std::size_t(image)]);
    }

    image = found->second;
}

// Validation stays separate from posing so a bad palette cannot partially replace a frozen pose.
bool valid_palette(const PreviewMesh& mesh, const scene::Instance& instance, std::size_t count) {
    // Negative palette selects the rigid path.
    if (instance.palette < 0) {
        return true;
    }

    for (const auto& vertex : mesh.source) {
        for (unsigned j = 0; j < 4; ++j) {
            if (vertex.weights[j] > 0
                && std::size_t(instance.palette) + vertex.joints[j] >= count) {
                return false;
            }
        }
    }

    return true;
}

// Frame the first displayed pose once; later animation keeps that stable studio scale and pivot.
void centre_mesh(PreviewMesh& mesh) {
    renderer::Vec3 low{}, high{};
    std::copy_n(mesh.vertices.front().position, 3, low.begin());
    high = low;

    for (const auto& vertex : mesh.vertices) {
        for (unsigned c = 0; c < 3; ++c) {
            low[c] = std::min(low[c], vertex.position[c]);
            high[c] = std::max(high[c], vertex.position[c]);
        }
    }

    float radius = 0;

    for (unsigned c = 0; c < 3; ++c) {
        mesh.pivot[c] = (low[c] + high[c]) * .5f;
        radius += (high[c] - low[c]) * (high[c] - low[c]) * .25f;
    }

    mesh.radius =
        std::max(std::sqrt(radius), .001f);  // A point-sized marker still has a finite camera.
}

}  // namespace

const scene::Instance* preview_instance(
    const scene::LevelData& level, const scene::PickHit& hit, std::span<const scene::Instance> live
) {
    // A corrupt or stale report cannot index the layer array.
    if (unsigned(hit.layer) >= scene::kLayerCount) {
        return nullptr;
    }

    const auto instances =
        hit.layer == scene::Layer::Mobys
            ? live
            : std::span<const scene::Instance>(level.instances[unsigned(hit.layer)]);

    // Live indices change when objects disappear; use the captured stable identity instead.
    if (hit.layer == scene::Layer::Mobys && hit.identity) {
        for (const auto& instance : instances) {
            if (instance.temporal_id == hit.identity && instance.model == hit.model) {
                return &instance;
            }
        }

        return nullptr;
    }

    return hit.instance < instances.size() && instances[hit.instance].model == hit.model
               ? &instances[hit.instance]
               : nullptr;
}

Result<void> preview_pose(
    PreviewMesh& mesh, const scene::Instance& instance, std::span<const renderer::Mat4> palette
) {
    // Validate the whole palette before replacing any part of the last displayed pose.
    if (!valid_palette(mesh, instance, palette.size())) {
        return unexpected(Error{"selected object's displayed joint palette is incomplete"});
    }

    mesh.instance = instance;
    mesh.bone_pose.assign(mesh.bones.size(), renderer::identity());

    for (std::size_t j = 0; j < mesh.bones.size(); ++j) {
        if (instance.palette >= 0 && std::size_t(instance.palette) + j < palette.size()) {
            mesh.bone_pose[j] =
                renderer::multiply(palette[std::size_t(instance.palette) + j], mesh.bones[j].bind);
        } else {
            mesh.bone_pose[j] = mesh.bones[j].bind;
        }
    }

    mesh.vertices.resize(mesh.source.size());

    for (std::size_t i = 0; i < mesh.source.size(); ++i) {
        auto vertex = pose_vertex(mesh.source[i], instance, palette);

        for (unsigned c = 0; c < 3; ++c) {
            vertex.position[c] = (vertex.position[c] - mesh.pivot[c]) / mesh.radius;
        }

        mesh.vertices[i] = vertex;
    }

    return {};
}

Result<PreviewMesh> preview_mesh(
    const scene::LevelData& level,
    const scene::PickHit& hit,
    const scene::Instance& instance,
    std::span<const renderer::Mat4> palette
) {
    if (hit.model >= level.models.size() || instance.model != hit.model) {
        return unexpected(Error{"selected object no longer has a valid model"});
    }

    PreviewMesh mesh;
    mesh.bones = level.models[hit.model].bones;
    std::map<int, int> images;
    const auto& primitives = level.models[hit.model].primitives;

    for (std::size_t i = 0; i < primitives.size(); ++i) {
        // Terrain selections isolate one fragment; props and characters keep every material.
        if (hit.layer == scene::Layer::Terrain && i != hit.primitive) {
            continue;
        }

        const auto& p = primitives[i];

        if (std::size_t(p.first_index) + p.index_count > level.indices.size() || p.material < 0
            || std::size_t(p.material) >= level.materials.size()) {
            return unexpected(Error{"selected model has an invalid material or index range"});
        }

        PreviewPart part{
            level.materials[std::size_t(p.material)], unsigned(mesh.source.size()), p.index_count
        };
        copy_image(part.material.image, level, mesh, images);
        copy_image(part.material.emission_image, level, mesh, images);
        mesh.parts.push_back(part);

        for (unsigned j = 0; j < p.index_count; ++j) {
            const auto at = level.indices[p.first_index + j];

            if (at >= level.vertices.size()) {
                return unexpected(Error{"selected model has an invalid vertex index"});
            }

            mesh.source.push_back(level.vertices[at]);
        }
    }

    if (mesh.source.empty()) {
        return unexpected(Error{"selected object has no previewable triangles"});
    }

    auto posed = preview_pose(mesh, instance, palette);

    if (!posed) {
        return unexpected(posed.error());
    }

    centre_mesh(mesh);
    posed = preview_pose(mesh, instance, palette);

    if (!posed) {
        return unexpected(posed.error());
    }

    return mesh;
}

void PreviewOrbit::move(float x, float y, float wheel) {
    yaw = std::remainder(yaw - x * .01f, 6.2831853f);     // One hundred pixels is one radian.
    pitch = std::clamp(pitch + y * .01f, -1.45f, 1.45f);  // Stay clear of the Z-up pole.
    distance = std::clamp(distance * std::exp(-wheel * .15f), 1.05f, 30.f);
}

renderer::Camera PreviewOrbit::camera(float aspect) const {
    renderer::Camera result;
    const renderer::Vec3 toward{
        std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch), std::sin(pitch)
    };
    renderer::Vec3 eye{}, forward{};

    for (unsigned c = 0; c < 3; ++c) {
        eye[c] = focus[c] + toward[c] * distance;
        forward[c] = -toward[c];
    }

    result.position = eye;
    result.view = renderer::look_along(eye, forward, {0, 0, 1});
    result.projection =
        renderer::perspective_reversed(1.0471976f, std::max(aspect, .01f), .01f, 100.f);
    return result;
}

void PreviewOrbit::navigate(float x, float y, bool fly) {
    const auto before = camera(1).position;
    move(x, y, 0);

    // Free look turns about the eye, while Alt+left orbit turns about the inspected focus.
    if (fly) {
        const auto after = camera(1).position;

        for (unsigned c = 0; c < 3; ++c) {
            focus[c] += before[c] - after[c];
        }
    }
}

void PreviewOrbit::travel(const renderer::Vec3& movement) {
    const auto view = camera(1).view;
    const auto inverse = renderer::inverse_affine(view);

    if (!inverse) {
        return;
    }

    const auto world = renderer::transform_vector(*inverse, movement);

    for (unsigned c = 0; c < 3; ++c) {
        focus[c] += world[c];
    }
}

std::optional<renderer::Mat4> preview_attachment(const PreviewMesh& mesh, int joint) {
    auto frame = mesh.instance.matrix;
    frame[12] = frame[13] = frame[14] = 0;

    if (joint >= 0) {
        if (std::size_t(joint) >= mesh.bone_pose.size()
            || mesh.bones[std::size_t(joint)].name.empty()) {
            return std::nullopt;
        }

        frame = renderer::multiply(frame, mesh.bone_pose[std::size_t(joint)]);
    }

    renderer::Vec3 offset{};

    for (unsigned c = 0; c < 3; ++c) {
        offset[c] = -mesh.pivot[c] / mesh.radius;
    }

    return renderer::multiply(
        renderer::translation(offset), renderer::multiply(renderer::scale(1 / mesh.radius), frame)
    );
}
}  // namespace openrac::scene
