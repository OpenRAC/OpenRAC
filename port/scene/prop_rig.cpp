// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/prop_rig.h"

#include <algorithm>
#include <cgltf.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>

#include "renderer/math.h"

namespace openrac::scene {

namespace {

using Mat4 = std::array<float, 16>;
using Quat = std::array<float, 4>;

int node_index(const cgltf_data* data, const cgltf_node* node) {
    return node == nullptr ? -1 : static_cast<int>(node - data->nodes);
}

// As load_model numbers a skin's joints.
int joint_number(const cgltf_node* joint, int index) {
    if (joint != nullptr && joint->name != nullptr && std::strncmp(joint->name, "joint_", 6) == 0) {
        return std::clamp(std::atoi(joint->name + 6), 0, 255);
    }
    return std::clamp(index, 0, 255);
}

Mat4 compose(const PropRig::Node& n) {
    const float x = n.rotation[0], y = n.rotation[1], z = n.rotation[2], w = n.rotation[3];
    const float r[9] = {
        1 - 2 * (y * y + z * z),
        2 * (x * y + z * w),
        2 * (x * z - y * w),
        2 * (x * y - z * w),
        1 - 2 * (x * x + z * z),
        2 * (y * z + x * w),
        2 * (x * z + y * w),
        2 * (y * z - x * w),
        1 - 2 * (x * x + y * y),
    };
    Mat4 m{};
    for (int c = 0; c < 3; ++c) {
        for (int k = 0; k < 3; ++k) {
            m[static_cast<std::size_t>(4 * c + k)] =
                r[3 * c + k] * n.scale[static_cast<std::size_t>(c)];
        }
    }
    m[12] = n.translation[0];
    m[13] = n.translation[1];
    m[14] = n.translation[2];
    m[15] = 1.0f;
    return m;
}

Quat normalized(Quat q) {
    const float length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (length > 0.0f) {
        for (float& v : q) {
            v /= length;
        }
    }
    return q;
}

Quat slerp(Quat a, Quat b, float t) {
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (dot < 0.0f) {
        dot = -dot;
        for (float& v : b) {
            v = -v;
        }
    }
    if (dot > 0.9995f) {
        Quat out{};
        for (std::size_t i = 0; i < 4; ++i) {
            out[i] = a[i] + (b[i] - a[i]) * t;
        }
        return normalized(out);
    }
    const float theta = std::acos(dot), s = std::sin(theta);
    const float wa = std::sin((1.0f - t) * theta) / s, wb = std::sin(t * theta) / s;
    return {
        wa * a[0] + wb * b[0], wa * a[1] + wb * b[1], wa * a[2] + wb * b[2], wa * a[3] + wb * b[3]
    };
}

// A channel's value at `time` into `out` (its width: 3 or 4).
void sample(const PropRig::Channel& c, float time, float* out) {
    const std::size_t width = c.path == PropRig::Path::Rotation ? 4 : 3;
    const std::size_t keys = c.times.size();
    const std::size_t stride = c.interpolation == PropRig::Interpolation::Cubic ? 3 * width : width;
    const std::size_t at =
        c.interpolation == PropRig::Interpolation::Cubic ? width : 0;  // the value
    const auto value = [&](std::size_t k) {
        return c.values.data() + k * stride + at;
    };
    if (keys == 0 || c.values.size() < keys * stride) {
        return;
    }
    if (keys == 1 || time <= c.times.front()) {
        std::copy(value(0), value(0) + width, out);
        return;
    }
    if (time >= c.times.back()) {
        std::copy(value(keys - 1), value(keys - 1) + width, out);
        return;
    }
    const std::size_t k =
        static_cast<std::size_t>(
            std::upper_bound(c.times.begin(), c.times.end(), time) - c.times.begin()
        )
        - 1;
    const float t0 = c.times[k], t1 = c.times[k + 1], span = t1 - t0;
    const float t = span > 0.0f ? (time - t0) / span : 0.0f;
    if (c.interpolation == PropRig::Interpolation::Step) {
        std::copy(value(k), value(k) + width, out);
        return;
    }
    if (c.interpolation == PropRig::Interpolation::Cubic) {
        // Hermite between the two values, with the out tangent of one and the in tangent of the next.
        const float* p0 = value(k);
        const float* m0 = c.values.data() + k * stride + 2 * width;
        const float* p1 = value(k + 1);
        const float* m1 = c.values.data() + (k + 1) * stride;
        const float t2 = t * t, t3 = t2 * t;
        for (std::size_t i = 0; i < width; ++i) {
            out[i] = (2 * t3 - 3 * t2 + 1) * p0[i] + (t3 - 2 * t2 + t) * span * m0[i]
                     + (-2 * t3 + 3 * t2) * p1[i] + (t3 - t2) * span * m1[i];
        }
        if (width == 4) {
            const Quat q = normalized({out[0], out[1], out[2], out[3]});
            std::copy(q.begin(), q.end(), out);
        }
        return;
    }
    if (width == 4) {
        const Quat q = slerp(
            {value(k)[0], value(k)[1], value(k)[2], value(k)[3]},
            {value(k + 1)[0], value(k + 1)[1], value(k + 1)[2], value(k + 1)[3]},
            t
        );
        std::copy(q.begin(), q.end(), out);
        return;
    }
    for (std::size_t i = 0; i < width; ++i) {
        out[i] = value(k)[i] + (value(k + 1)[i] - value(k)[i]) * t;
    }
}

}  // namespace

std::optional<PropRig> load_prop_rig(const std::filesystem::path& path, std::string& error) {
    cgltf_options options{};
    cgltf_data* data = nullptr;
    const std::string name = path.string();
    if (cgltf_parse_file(&options, name.c_str(), &data) != cgltf_result_success) {
        error = std::format("cannot read glTF {}", name);
        return std::nullopt;
    }
    if (cgltf_load_buffers(&options, data, name.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        error = std::format("cannot read the buffers of {}", name);
        return std::nullopt;
    }
    // Accessors past their buffers, joints with no matrix: read nothing of a broken file.
    if (cgltf_validate(data) != cgltf_result_success) {
        cgltf_free(data);
        error = std::format("{} is not valid glTF", name);
        return std::nullopt;
    }
    PropRig rig;
    rig.nodes.resize(data->nodes_count);
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        PropRig::Node& out = rig.nodes[n];
        out.parent = node_index(data, node.parent);
        if (node.has_translation) {
            std::copy(node.translation, node.translation + 3, out.translation.begin());
        }
        if (node.has_rotation) {
            std::copy(node.rotation, node.rotation + 4, out.rotation.begin());
        }
        if (node.has_scale) {
            std::copy(node.scale, node.scale + 3, out.scale.begin());
        }
    }
    // The skins of the meshes load_model reads, joints numbered as it numbers them.
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_skin* skin = data->nodes[n].skin;
        if (data->nodes[n].mesh == nullptr || skin == nullptr) {
            continue;
        }
        for (cgltf_size j = 0; j < skin->joints_count; ++j) {
            PropRig::Joint joint;
            joint.node = node_index(data, skin->joints[j]);
            joint.number = joint_number(skin->joints[j], static_cast<int>(j));
            joint.inverse_bind = renderer::identity();
            if (skin->inverse_bind_matrices != nullptr && j < skin->inverse_bind_matrices->count) {
                cgltf_accessor_read_float(
                    skin->inverse_bind_matrices, j, joint.inverse_bind.data(), 16
                );
            }
            if (joint.node >= 0) {
                rig.joints.push_back(joint);
                rig.palette_size = std::max(rig.palette_size, joint.number + 1);
            }
        }
    }
    if (rig.joints.empty()) {
        cgltf_free(data);
        return std::nullopt;
    }
    for (cgltf_size a = 0; a < data->animations_count; ++a) {
        const cgltf_animation& source = data->animations[a];
        PropRig::Animation animation;
        animation.name = source.name != nullptr ? source.name : std::format("animation_{}", a);
        for (cgltf_size c = 0; c < source.channels_count; ++c) {
            const cgltf_animation_channel& channel = source.channels[c];
            if (channel.target_node == nullptr || channel.sampler == nullptr) {
                continue;
            }
            PropRig::Channel out;
            out.node = node_index(data, channel.target_node);
            switch (channel.target_path) {
                case cgltf_animation_path_type_translation:
                    out.path = PropRig::Path::Translation;
                    break;
                case cgltf_animation_path_type_rotation:
                    out.path = PropRig::Path::Rotation;
                    break;
                case cgltf_animation_path_type_scale:
                    out.path = PropRig::Path::Scale;
                    break;
                default:
                    continue;  // morph weights: the game has none
            }
            switch (channel.sampler->interpolation) {
                case cgltf_interpolation_type_step:
                    out.interpolation = PropRig::Interpolation::Step;
                    break;
                case cgltf_interpolation_type_cubic_spline:
                    out.interpolation = PropRig::Interpolation::Cubic;
                    break;
                default:
                    out.interpolation = PropRig::Interpolation::Linear;
                    break;
            }
            const cgltf_accessor* input = channel.sampler->input;
            const cgltf_accessor* output = channel.sampler->output;
            if (input == nullptr || output == nullptr) {
                continue;
            }
            out.times.resize(input->count);
            for (cgltf_size k = 0; k < input->count; ++k) {
                cgltf_accessor_read_float(input, k, &out.times[k], 1);
            }
            const cgltf_size width = out.path == PropRig::Path::Rotation ? 4 : 3;
            out.values.resize(output->count * width);
            for (cgltf_size k = 0; k < output->count; ++k) {
                cgltf_accessor_read_float(output, k, &out.values[k * width], width);
            }
            if (!out.times.empty()) {
                animation.duration = std::max(animation.duration, out.times.back());
            }
            animation.channels.push_back(std::move(out));
        }
        rig.animations.push_back(std::move(animation));
    }
    cgltf_free(data);
    return rig;
}

std::vector<std::array<float, 16>> prop_rig_palette(
    const PropRig& rig, int animation, float seconds, bool loop
) {
    if (animation < 0 || static_cast<std::size_t>(animation) >= rig.animations.size()
        || !std::isfinite(seconds)) {
        return {};
    }
    const PropRig::Animation& a = rig.animations[static_cast<std::size_t>(animation)];
    float time = seconds;
    if (a.duration > 0.0f) {
        time = loop ? std::fmod(seconds, a.duration) : std::clamp(seconds, 0.0f, a.duration);
        if (time < 0.0f) {
            time += a.duration;
        }
    }
    std::vector<PropRig::Node> nodes = rig.nodes;
    for (const PropRig::Channel& c : a.channels) {
        if (c.node < 0 || static_cast<std::size_t>(c.node) >= nodes.size()) {
            continue;
        }
        PropRig::Node& n = nodes[static_cast<std::size_t>(c.node)];
        switch (c.path) {
            case PropRig::Path::Translation:
                sample(c, time, n.translation.data());
                break;
            case PropRig::Path::Rotation:
                sample(c, time, n.rotation.data());
                break;
            case PropRig::Path::Scale:
                sample(c, time, n.scale.data());
                break;
        }
    }
    // Each node's matrix in the scene, from its parents' (in any order the file has them).
    std::vector<Mat4> global(nodes.size());
    std::vector<char> done(nodes.size(), 0);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        std::vector<std::size_t> chain;
        for (int k = static_cast<int>(i); k >= 0 && !done[static_cast<std::size_t>(k)];
             k = nodes[static_cast<std::size_t>(k)].parent) {
            chain.push_back(static_cast<std::size_t>(k));
            if (chain.size() > nodes.size()) {
                break;  // a loop: not a tree
            }
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const int parent = nodes[*it].parent;
            const Mat4 local = compose(nodes[*it]);
            global[*it] = parent >= 0 && done[static_cast<std::size_t>(parent)]
                              ? renderer::multiply(global[static_cast<std::size_t>(parent)], local)
                              : local;
            done[*it] = 1;
        }
    }
    std::vector<std::array<float, 16>> palette(
        static_cast<std::size_t>(rig.palette_size), renderer::identity()
    );
    for (const PropRig::Joint& j : rig.joints) {
        palette[static_cast<std::size_t>(j.number)] =
            renderer::multiply(global[static_cast<std::size_t>(j.node)], j.inverse_bind);
    }
    return palette;
}

}  // namespace openrac::scene
