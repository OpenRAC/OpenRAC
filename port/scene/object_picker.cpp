// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/object_picker.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace openrac::scene {
namespace {
Vec3 minus(Vec3 a, Vec3 b) {
    for (unsigned i = 0; i < 3; ++i) {
        a[i] -= b[i];
    }
    return a;
}

Vec3 point(const Vertex& v) {
    return {v.position[0], v.position[1], v.position[2]};
}

// Affine inverse, including nonuniform scale. Singular objects cannot be picked.
bool inverse(const Mat4& m, Mat4& out) {
    Vec3 a{m[0], m[1], m[2]}, b{m[4], m[5], m[6]}, c{m[8], m[9], m[10]};
    Vec3 x = renderer::cross(b, c), y = renderer::cross(c, a), z = renderer::cross(a, b);
    float determinant = renderer::dot(a, x);
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f) {
        return false;
    }
    out = renderer::identity();
    for (unsigned i = 0; i < 3; ++i) {
        out[i * 4] = x[i] / determinant;
        out[i * 4 + 1] = y[i] / determinant;
        out[i * 4 + 2] = z[i] / determinant;
    }
    auto t = renderer::transform_point(out, {m[12], m[13], m[14]});
    for (unsigned i = 0; i < 3; ++i) {
        out[12 + i] = -t[i];
    }
    return true;
}

Vec3 direction(const Mat4& m, Vec3 d) {
    Vec3 out{};
    for (unsigned i = 0; i < 3; ++i) {
        out[i] = m[i] * d[0] + m[4 + i] * d[1] + m[8 + i] * d[2];
    }
    return out;
}

bool box(Vec3 lo, Vec3 hi, Vec3 origin, Vec3 ray, float limit) {
    float near = .05f, far = limit;
    for (unsigned i = 0; i < 3; ++i) {
        if (std::abs(ray[i]) < 1e-12f) {
            if (origin[i] < lo[i] || origin[i] > hi[i]) {
                return false;
            }
            continue;
        }
        float a = (lo[i] - origin[i]) / ray[i], b = (hi[i] - origin[i]) / ray[i];
        if (a > b) {
            std::swap(a, b);
        }
        near = std::max(near, a);
        far = std::min(far, b);
        if (near > far) {
            return false;
        }
    }
    return true;
}

std::optional<std::array<float, 3>> triangle(
    Vec3 a, Vec3 b, Vec3 c, Vec3 origin, Vec3 ray, float limit
) {
    auto e1 = minus(b, a), e2 = minus(c, a), p = renderer::cross(ray, e2);
    float determinant = renderer::dot(e1, p);
    if (std::abs(determinant) < 1e-10f) {
        return {};
    }
    auto t = minus(origin, a);
    float u = renderer::dot(t, p) / determinant;
    if (u < 0 || u > 1) {
        return {};
    }
    auto q = renderer::cross(t, e1);
    float v = renderer::dot(ray, q) / determinant;
    if (v < 0 || u + v > 1) {
        return {};
    }
    float distance = renderer::dot(e2, q) / determinant;
    return std::isfinite(distance) && distance >= .05f && distance < limit
               ? std::optional(std::array<float, 3>{distance, u, v})
               : std::nullopt;
}
}  // namespace

void ObjectPicker::clear() {
    m_meshes.clear();
}

unsigned ObjectPicker::build(Mesh& mesh, const LevelData& level, unsigned begin, unsigned end) {
    const float infinity = std::numeric_limits<float>::infinity();
    Bounds bounds{{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
    for (unsigned i = begin; i < end; ++i) {
        for (auto vertex : mesh.triangles[i].vertices) {
            for (unsigned c = 0; c < 3; ++c) {
                float value = level.vertices[vertex].position[c];
                bounds.low[c] = std::min(bounds.low[c], value);
                bounds.high[c] = std::max(bounds.high[c], value);
            }
        }
    }
    unsigned at = unsigned(mesh.nodes.size());
    mesh.nodes.push_back({bounds, begin, end - begin});
    if (end - begin > 8) {
        unsigned axis = 0;
        for (unsigned c = 1; c < 3; ++c) {
            if (bounds.high[c] - bounds.low[c] > bounds.high[axis] - bounds.low[axis]) {
                axis = c;
            }
        }
        auto centroid = [&](const Triangle& t) {
            float sum = 0;
            for (auto v : t.vertices) {
                sum += level.vertices[v].position[axis];
            }
            return sum;
        };
        unsigned middle = begin + (end - begin) / 2;
        std::nth_element(
            mesh.triangles.begin() + begin,
            mesh.triangles.begin() + middle,
            mesh.triangles.begin() + end,
            [&](const auto& a, const auto& b) { return centroid(a) < centroid(b); }
        );
        unsigned left = build(mesh, level, begin, middle), right = build(mesh, level, middle, end);
        mesh.nodes[at].left = left;
        mesh.nodes[at].right = right;
        mesh.nodes[at].count = 0;
    }
    return at;
}

void ObjectPicker::load(const LevelData& level) {
    m_meshes.resize(level.models.size());
    for (unsigned m = 0; m < level.models.size(); ++m) {
        auto& mesh = m_meshes[m];
        for (unsigned p = 0; p < level.models[m].primitives.size(); ++p) {
            const auto& primitive = level.models[m].primitives[p];
            for (unsigned i = 0; i + 2 < primitive.index_count; i += 3) {
                if (std::size_t(primitive.first_index) + i + 2 >= level.indices.size()) {
                    break;
                }
                Triangle t{
                    {level.indices[primitive.first_index + i],
                     level.indices[primitive.first_index + i + 1],
                     level.indices[primitive.first_index + i + 2]},
                    p,
                    i / 3
                };
                if (std::all_of(t.vertices.begin(), t.vertices.end(), [&](auto v) {
                        return v < level.vertices.size();
                    })) {
                    mesh.triangles.push_back(t);
                }
            }
        }
        if (!mesh.triangles.empty()) {
            build(mesh, level, 0, unsigned(mesh.triangles.size()));
        }
    }
}

Vec3 ObjectPicker::posed_vertex(
    const LevelData& level, const Instance& instance, std::span<const Mat4> palette, unsigned at
) const {
    const auto& source = level.vertices[at];
    Vec3 p = point(source);
    float total = 0;
    for (float w : source.weights) {
        total += w;
    }
    if (instance.palette >= 0 && total > 0) {
        p = {};
        for (unsigned j = 0; j < 4; ++j) {
            if (source.weights[j] == 0) {
                continue;
            }
            std::size_t joint = std::size_t(instance.palette) + source.joints[j];
            if (joint >= palette.size()) {
                continue;
            }
            Vec3 q = renderer::transform_point(palette[joint], point(source));
            for (unsigned c = 0; c < 3; ++c) {
                p[c] += source.weights[j] * q[c];
            }
        }
    }
    return p;
}

bool ObjectPicker::refresh(
    const LevelData& level,
    std::span<const Instance> live,
    std::span<const Mat4> palette,
    const renderer::Camera& camera,
    PickHit& hit
) {
    if (m_meshes.empty() || hit.model >= m_meshes.size()) {
        return false;
    }
    auto instances = hit.layer == Layer::Mobys
                         ? live
                         : std::span<const Instance>(level.instances[unsigned(hit.layer)]);
    const Instance* instance = nullptr;
    if (hit.layer == Layer::Mobys && hit.identity) {
        for (const auto& i : instances) {
            if (i.temporal_id == hit.identity && i.model == hit.model) {
                instance = &i;
                break;
            }
        }
    } else if (hit.instance < instances.size() && instances[hit.instance].model == hit.model) {
        instance = &instances[hit.instance];
    }
    if (!instance) {
        return false;
    }
    const auto& mesh = m_meshes[hit.model];
    auto selected = std::find_if(mesh.triangles.begin(), mesh.triangles.end(), [&](const auto& t) {
        return t.primitive == hit.primitive && t.number == hit.triangle;
    });
    if (selected == mesh.triangles.end()) {
        return false;
    }
    auto bounds = mesh.nodes[0].bounds;
    if (instance->palette >= 0 || hit.layer == Layer::Terrain) {
        bool first = true;
        for (const auto& t : mesh.triangles) {
            if (hit.layer == Layer::Terrain && t.primitive != hit.primitive) {
                continue;
            }
            for (auto at : t.vertices) {
                auto p = posed_vertex(level, *instance, palette, at);
                if (first) {
                    bounds = {p, p};
                    first = false;
                } else {
                    for (unsigned c = 0; c < 3; ++c) {
                        bounds.low[c] = std::min(bounds.low[c], p[c]);
                        bounds.high[c] = std::max(bounds.high[c], p[c]);
                    }
                }
            }
        }
    }
    hit.object_position = {instance->matrix[12], instance->matrix[13], instance->matrix[14]};
    hit.position = {};
    for (unsigned c = 0; c < 3; ++c) {
        hit.face[c] = renderer::transform_point(
            instance->matrix, posed_vertex(level, *instance, palette, selected->vertices[c])
        );
        for (unsigned k = 0; k < 3; ++k) {
            hit.position[k] += hit.face[c][k] * hit.barycentric[c];
        }
    }
    for (unsigned c = 0; c < 8; ++c) {
        Vec3 p;
        for (unsigned k = 0; k < 3; ++k) {
            p[k] = (c & (1u << k)) ? bounds.high[k] : bounds.low[k];
        }
        hit.bounds[c] = renderer::transform_point(instance->matrix, p);
    }
    auto delta = minus(hit.position, camera.position);
    hit.distance = std::sqrt(renderer::dot(delta, delta));
    return true;
}

std::optional<PickHit> ObjectPicker::cast(
    const LevelData& level,
    std::span<const Instance> live,
    std::span<const Mat4> palette,
    const renderer::Camera& camera,
    unsigned layers,
    std::array<float, 2> ndc
) {
    if (m_meshes.empty()) {
        load(level);
    }
    Vec3 view_ray{0, 0, -1};
    if (std::abs(camera.projection[0]) > 1e-8f) {
        view_ray[0] = (ndc[0] + camera.projection[8]) / camera.projection[0];
    }
    if (std::abs(camera.projection[5]) > 1e-8f) {
        view_ray[1] = (ndc[1] + camera.projection[9]) / camera.projection[5];
    }
    const auto& v = camera.view;
    Vec3 ray = renderer::normalize(
        {v[0] * view_ray[0] + v[1] * view_ray[1] + v[2] * view_ray[2],
         v[4] * view_ray[0] + v[5] * view_ray[1] + v[6] * view_ray[2],
         v[8] * view_ray[0] + v[9] * view_ray[1] + v[10] * view_ray[2]}
    );
    if (!std::all_of(ray.begin(), ray.end(), [](float value) { return std::isfinite(value); })
        || renderer::dot(ray, ray) < .5f) {
        return {};
    }
    std::optional<PickHit> result;
    float nearest = 2000;
    for (unsigned layer = 1; layer < kLayerCount; ++layer) {
        if (!(layers & (1u << layer))) {
            continue;
        }
        auto instances = layer == unsigned(Layer::Mobys)
                             ? live
                             : std::span<const Instance>(level.instances[layer]);
        for (unsigned index = 0; index < instances.size(); ++index) {
            const auto& instance = instances[index];
            if (instance.model >= m_meshes.size() || instance.moby_light[3] > 1.5f
                || instance.tint[3] <= 0) {
                continue;
            }
            const auto& mesh = m_meshes[instance.model];
            if (mesh.nodes.empty()) {
                continue;
            }
            Mat4 inv;
            if (!inverse(instance.matrix, inv)) {
                continue;
            }
            Vec3 origin = renderer::transform_point(inv, camera.position),
                 local_ray = direction(inv, ray);
            bool skinned = instance.palette >= 0;
            if (skinned && instance.world_sphere[3] > 0) {
                const auto& sphere = instance.world_sphere;
                float r = sphere[3];
                if (!box(
                        {sphere[0] - r, sphere[1] - r, sphere[2] - r},
                        {sphere[0] + r, sphere[1] + r, sphere[2] + r},
                        camera.position,
                        ray,
                        nearest
                    )) {
                    continue;
                }
            } else if (!skinned
                       && !box(
                           mesh.nodes[0].bounds.low,
                           mesh.nodes[0].bounds.high,
                           origin,
                           local_ray,
                           nearest
                       )) {
                continue;
            }
            std::unordered_map<unsigned, Vec3> posed;
            auto vertex = [&](unsigned at) {
                if (auto found = posed.find(at); found != posed.end()) {
                    return found->second;
                }
                Vec3 p = posed_vertex(level, instance, palette, at);
                if (skinned) {
                    posed.emplace(at, p);
                }
                return p;
            };
            Bounds posed_bounds = mesh.nodes[0].bounds;
            if (skinned) {
                bool first = true;
                for (const auto& t : mesh.triangles) {
                    for (auto at : t.vertices) {
                        auto p = vertex(at);
                        if (first) {
                            posed_bounds = {p, p};
                            first = false;
                        } else {
                            for (unsigned c = 0; c < 3; ++c) {
                                posed_bounds.low[c] = std::min(posed_bounds.low[c], p[c]);
                                posed_bounds.high[c] = std::max(posed_bounds.high[c], p[c]);
                            }
                        }
                    }
                }
                if (!box(posed_bounds.low, posed_bounds.high, origin, local_ray, nearest)) {
                    continue;
                }
            }
            auto test = [&](const Triangle& t) {
                std::array<Vec3, 3> face{
                    vertex(t.vertices[0]), vertex(t.vertices[1]), vertex(t.vertices[2])
                };
                auto distance = triangle(face[0], face[1], face[2], origin, local_ray, nearest);
                if (!distance) {
                    return;
                }
                nearest = (*distance)[0];
                PickHit hit;
                hit.barycentric =
                    {1 - (*distance)[1] - (*distance)[2], (*distance)[1], (*distance)[2]};
                hit.layer = Layer(layer);
                hit.instance = index;
                hit.model = instance.model;
                hit.primitive = t.primitive;
                hit.triangle = t.number;
                hit.material = level.models[instance.model].primitives[t.primitive].material;
                hit.identity = instance.temporal_id;
                hit.distance = nearest;
                hit.name = level.models[instance.model].name;
                hit.object_position =
                    {instance.matrix[12], instance.matrix[13], instance.matrix[14]};
                for (unsigned c = 0; c < 3; ++c) {
                    hit.position[c] = camera.position[c] + ray[c] * nearest;
                }
                for (unsigned c = 0; c < 3; ++c) {
                    hit.face[c] = renderer::transform_point(instance.matrix, face[c]);
                }
                for (unsigned c = 0; c < 8; ++c) {
                    Vec3 p;
                    for (unsigned k = 0; k < 3; ++k) {
                        p[k] = (c & (1u << k)) ? posed_bounds.high[k] : posed_bounds.low[k];
                    }
                    hit.bounds[c] = renderer::transform_point(instance.matrix, p);
                }
                if (layer == unsigned(Layer::Mobys)) {
                    for (const auto& [id, model] : level.moby_classes) {
                        if (model.first == instance.model) {
                            hit.class_id = id;
                            break;
                        }
                    }
                }
                if (hit.material >= 0 && unsigned(hit.material) < level.materials.size()) {
                    int image = level.materials[unsigned(hit.material)].image;
                    if (image >= 0 && unsigned(image) < level.image_paths.size()) {
                        hit.texture = level.image_paths[unsigned(image)];
                    }
                }
                result = std::move(hit);
            };
            if (skinned) {
                for (const auto& t : mesh.triangles) {
                    test(t);
                }
            } else {
                std::vector<unsigned> stack{0};
                while (!stack.empty()) {
                    unsigned at = stack.back();
                    stack.pop_back();
                    const auto& node = mesh.nodes[at];
                    if (!box(node.bounds.low, node.bounds.high, origin, local_ray, nearest)) {
                        continue;
                    }
                    if (node.count) {
                        for (unsigned i = node.first; i < node.first + node.count; ++i) {
                            test(mesh.triangles[i]);
                        }
                    } else {
                        stack.push_back(node.left);
                        stack.push_back(node.right);
                    }
                }
            }
        }
    }
    if (result) {
        refresh(level, live, palette, camera, *result);
    }
    return result;
}

std::string picker_report_json(
    const PickResult& result, bool enabled, std::string_view game, std::string_view class_name
) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(9);
    auto string = [&](std::string_view text) {
        out << '"';
        for (unsigned char c : text) {
            if (c == '"' || c == '\\') {
                out << '\\' << char(c);
            } else if (c < 32) {
                out << "\\u00" << "0123456789abcdef"[c >> 4] << "0123456789abcdef"[c & 15];
            } else {
                out << char(c);
            }
        }
        out << '"';
    };
    auto vector = [&](Vec3 p) {
        out << '[';
        for (unsigned i = 0; i < 3; ++i) {
            if (i) {
                out << ',';
            }
            out << (std::isfinite(p[i]) ? p[i] : 0.f);
        }
        out << ']';
    };
    out << "{\"schema\":1,\"enabled\":" << (enabled ? "true" : "false") << ",\"game\":";
    string(game);
    out << ",\"level\":" << result.level << ",\"tick\":" << result.tick << ",\"hit\":";
    if (!enabled || !result.hit) {
        out << "null";
    } else {
        const auto& h = *result.hit;
        out << "{\"layer\":";
        string(layer_name(h.layer));
        out << ",\"name\":";
        string(h.name);
        out << ",\"class_name\":";
        string(class_name);
        out << ",\"class_id\":" << h.class_id;
        out << ",\"instance\":" << h.instance << ",\"model\":" << h.model
            << ",\"primitive\":" << h.primitive << ",\"triangle\":" << h.triangle;
        out << ",\"material\":" << h.material << ",\"identity\":" << h.identity << ",\"texture\":";
        string(h.texture);
        out << ",\"distance\":" << h.distance << ",\"position\":";
        vector(h.position);
        out << ",\"object_position\":";
        vector(h.object_position);
        out << ",\"bounds\":[";
        for (unsigned i = 0; i < 8; ++i) {
            if (i) {
                out << ',';
            }
            vector(h.bounds[i]);
        }
        out << "]}";
    }
    out << "}\n";
    return out.str();
}
}  // namespace openrac::scene
