// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

// Original receiving motions, read from the player's disc and posed over the current world.
static void load_special_pickup_clips() {
    g->special_pickup_clips.clear();
    if (!openrac_game_disc_files) {
        return;
    }
    namespace disc = assets::disc;
    namespace anim = assets::rac1;
    const bool pal = g->render.game.ends_with("-pal");
    const float fps = pal ? 25.0f : 30.0f;

    struct Source {
        int item, level, scene;
        float start, seconds, item_scale;
    };

    const Source sources[] =
        {{1002, 5, 9, 24.0f, 4.8f, 1.0f / 12},
         {1428, 17, 1, 0, 4.12f, 1.0f / 6},
         {407, 11, 6, 13.2f, 1.08f, 1.0f / 12},
         {37, 12, 2, 33.84f, .76f, 1.0f / 12},
         // Begin after the dispenser interaction, with the can already held.
         {645, 10, 0, 2.0f, 7.76f, .25f},
         {1433, 10, 1, 2.0f, 7.8f, 1.0f / 12}};
    try {
        const auto image = disc::Disc::open(openrac_game_disc_files, disc::rac1_layout());
        for (const auto& source : sources) {
            try {
                const auto bytes = image.scene_region(source.level, source.scene, pal);
                const auto level = std::find_if(
                    image.levels().begin(), image.levels().end(), [&](const auto& entry) {
                        return entry.header.id == unsigned(source.level);
                    }
                );
                if (!bytes || level == image.levels().end()) {
                    continue;
                }
                const auto movie = disc::Scene::load(
                    level->header,
                    assets::ByteView(*bytes),
                    source.scene,
                    pal ? disc::SceneRegion::Pal : disc::SceneRegion::Ntsc
                );
                const auto files = image.level(source.level);
                if (!files.core_data || !files.core_index) {
                    throw std::runtime_error("missing donor rig data");
                }
                const auto core_data = disc::wad_decompress(assets::ByteView(*files.core_data));
                const auto core =
                    disc::parse_level_core(assets::ByteView(*files.core_index), core_data.size());
                scene::SpecialClip clip;
                clip.fps = fps;
                clip.duration = source.seconds;
                const int begin = int(std::round(source.start * fps));
                const int end = begin + int(std::ceil(source.seconds * fps));
                // Pokitaru keeps the scientist's Persuader (407) in the scene;
                // Ratchet's receiving prop is the separate actor 692.
                const int receiving_prop = source.item == 407 ? 692 : source.item;
                for (int klass : {0, receiving_prop}) {
                    const auto mesh =
                        g->levels
                        / std::format("level_{:02d}/mobys/moby_{}.glb", source.level, klass);
                    std::string why;
                    const auto loaded = scene::load_model(mesh, g->render.level, why);
                    if (!loaded) {
                        throw std::runtime_error(why);
                    }
                    auto& model = g->render.level.models[*loaded];
                    if (source.item == 407 && klass == 692) {
                        // The original handoff shows the back of the disc from
                        // this camera. Turn its textured face toward the viewer
                        // around its own center, preserving the hand attachment.
                        const float cx = (model.min[0] + model.max[0]) * .5f;
                        const float cy = (model.min[1] + model.max[1]) * .5f;
                        std::set<std::uint32_t> vertices;
                        for (const auto& primitive : model.primitives) {
                            for (std::uint32_t i = 0; i < primitive.index_count; ++i) {
                                vertices.insert(g->render.level.indices[primitive.first_index + i]);
                            }
                        }
                        for (const auto index : vertices) {
                            auto& vertex = g->render.level.vertices[index];
                            vertex.position[0] = 2 * cx - vertex.position[0];
                            vertex.position[1] = 2 * cy - vertex.position[1];
                            vertex.normal[0] = -vertex.normal[0];
                            vertex.normal[1] = -vertex.normal[1];
                        }
                    }
                    scene::SpecialActor track;
                    track.model = *loaded;
                    track.scale = klass == 0 ? 7.0f / 48 : source.item_scale;
                    const auto entry = std::find_if(
                        core.moby_classes.begin(), core.moby_classes.end(), [&](const auto& e) {
                            return e.o_class == klass && e.offset_in_asset_wad > 0;
                        }
                    );
                    if (entry == core.moby_classes.end()) {
                        throw std::runtime_error("missing native actor class");
                    }
                    const auto native = anim::parse_moby_class(
                        assets::ByteView(core_data)
                            .tail(std::size_t(entry->offset_in_asset_wad), "native receiving rig")
                    );
                    anim::MobyAnimClass original(native, {});
                    if (original.joint_count != model.bones.size()) {
                        throw std::runtime_error("native rig joint mismatch");
                    }
                    auto rig = std::move(original);
                    track.scale = native.header.scale;
                    int cursor = 0;
                    for (const auto& chunk : movie.chunks) {
                        const auto actor = std::find_if(
                            chunk.actors.begin(), chunk.actors.end(), [&](const auto& a) {
                                return a.klass == klass;
                            }
                        );
                        if (actor == chunk.actors.end()) {
                            throw std::runtime_error("missing receiving actor");
                        }
                        const auto data = chunk.sequence(*actor);
                        std::vector<std::uint8_t> sequence(data.data(), data.data() + data.size());
                        if (sequence.size() <= 0x12) {
                            throw std::runtime_error("short actor sequence");
                        }
                        sequence[0x12] = 0;
                        rig.sequences.clear();
                        rig.sequences.emplace_back(
                            anim::parse_sequence(assets::ByteView(sequence), 0)
                        );
                        const auto count = rig.sequences[0]->frames.size();
                        const int used = std::min(int(count), pal ? 40 : 48);
                        for (int i = 0; i < used; ++i, ++cursor) {
                            if (cursor < begin || cursor >= end) {
                                continue;
                            }
                            if (std::size_t(i) >= actor->positions.size()) {
                                throw std::runtime_error("short actor positions");
                            }
                            scene::SpecialActorFrame frame;
                            for (int axis = 0; axis < 3; ++axis) {
                                frame.position[axis] = actor->positions[i][axis];
                            }
                            anim::AnimState state;
                            state.frame_a = state.frame_b = std::uint8_t(i);
                            const auto pose = anim::evaluate(rig, state);
                            for (const auto& joint : pose) {
                                auto matrix = renderer::identity();
                                for (int col = 0; col < 4; ++col) {
                                    for (int row = 0; row < 4; ++row) {
                                        matrix[col * 4 + row] =
                                            joint[col][row]
                                            / (col == 3 && row < 3 ? 1024.0f : 1.0f);
                                    }
                                }
                                frame.pose.push_back(matrix);
                            }
                            track.frames.push_back(std::move(frame));
                        }
                    }
                    if (track.frames.empty()) {
                        throw std::runtime_error("empty receiving motion");
                    }
                    clip.actors.push_back(std::move(track));
                }
                if (source.item == 407) {
                    const auto& prop = g->render.level.models[clip.actors[1].model];
                    auto material = g->render.level.materials[prop.primitives.front().material];
                    auto image = g->render.level.images[material.image];
                    int left = image.width, right = 0, top = image.height, bottom = 0;
                    for (int y = 0; y < image.height; ++y) {
                        for (int x = 0; x < image.width; ++x) {
                            auto* p = &image.pixels[(y * image.width + x) * 4];
                            if (p[1] > 50 && p[1] > p[0] * 1.25f) {
                                left = std::min(left, x);
                                right = std::max(right, x);
                                top = std::min(top, y);
                                bottom = std::max(bottom, y);
                            }
                            const auto green = p[1];
                            p[0] = std::uint8_t(p[0] * .25f);
                            p[1] = std::uint8_t(green * .65f);
                            p[2] = std::max(p[2], green);
                        }
                    }
                    material.image = int(g->render.level.images.size());
                    g->render.level.image_paths.push_back("receiving_persuader_blue_swirl");
                    g->render.level.image_animations.resize(g->render.level.images.size() + 1);
                    g->render.level.translucent_images.push_back(false);
                    g->render.level.images.push_back(std::move(image));
                    material.emission = {0, 0, 0};
                    material.authored_emission = false;
                    const int surface = int(g->render.level.materials.size());
                    g->render.level.materials.push_back(material);
                    const auto& texture = g->render.level.images[material.image];
                    const float u = (left + right + 1) * .5f / texture.width,
                                v = (top + bottom + 1) * .5f / texture.height;
                    const float ur = (right - left + 1) * .5f / texture.width,
                                vr = (bottom - top + 1) * .5f / texture.height;
                    const renderer::Vec3 center{
                        (prop.min[0] + prop.max[0]) * .5f, 0, (prop.min[2] + prop.max[2]) * .5f
                    };
                    const float radius = (prop.max[2] - prop.min[2]) * .42f;
                    auto prototype =
                        g->render.level
                            .vertices[g->render.level.indices[prop.primitives.front().first_index]];
                    scene::Model disc;
                    disc.name = "receiving_persuader_swirl";
                    disc.bones = prop.bones;
                    disc.joints = prop.joints;
                    disc.min = prop.min;
                    disc.max = prop.max;
                    scene::Primitive primitive;
                    primitive.material = surface;
                    primitive.first_index = std::uint32_t(g->render.level.indices.size());
                    const auto first = std::uint32_t(g->render.level.vertices.size());
                    for (int side = 0; side < 2; ++side) {
                        for (int i = 0; i <= 32; ++i) {
                            const float angle = i ? float(i - 1) * 6.2831853f / 32 : 0;
                            const float y = i ? std::cos(angle) : 0, z = i ? std::sin(angle) : 0;
                            auto vertex = prototype;
                            vertex.position[0] = side ? prop.max[0] + .005f : prop.min[0] - .005f;
                            vertex.position[1] = center[1] + radius * y;
                            vertex.position[2] = center[2] + radius * z;
                            vertex.normal[0] = 1;
                            vertex.normal[1] = vertex.normal[2] = 0;
                            vertex.uv[0] = u + ur * y;
                            vertex.uv[1] = v - vr * z;
                            g->render.level.vertices.push_back(vertex);
                        }
                    }
                    for (unsigned side = 0; side < 2; ++side) {
                        for (unsigned i = 0; i < 32; ++i) {
                            const auto start = first + side * 33;
                            const std::uint32_t a = start + 1 + i, b = start + 1 + (i + 1) % 32;
                            for (auto index : {start, a, b, start, b, a}) {
                                g->render.level.indices.push_back(index);
                            }
                        }
                    }
                    primitive.index_count = 384;
                    disc.primitives.push_back(primitive);
                    auto swirl = clip.actors[1];
                    swirl.model = std::uint32_t(g->render.level.models.size());
                    g->render.level.models.push_back(std::move(disc));
                    for (std::size_t i = 0; i < swirl.frames.size(); ++i) {
                        auto rotation = renderer::identity();
                        const float a = float(i) / fps * 3;
                        rotation[5] = rotation[10] = std::cos(a);
                        rotation[6] = std::sin(a);
                        rotation[9] = -rotation[6];
                        const auto spin = renderer::multiply(
                            renderer::translation(center),
                            renderer::multiply(
                                rotation,
                                renderer::translation({-center[0], -center[1], -center[2]})
                            )
                        );
                        for (auto& joint : swirl.frames[i].pose) {
                            joint = renderer::multiply(joint, spin);
                        }
                    }
                    clip.actors.push_back(std::move(swirl));
                }
                clip.origin = clip.actors.front().frames.front().position;
                const auto& hero = clip.actors.front();
                if (!hero.frames.front().pose.empty()
                    && !g->render.level.models[hero.model].bones.empty()) {
                    const auto root = renderer::multiply(
                        hero.frames.front().pose.front(),
                        g->render.level.models[hero.model].bones.front().bind
                    );
                    clip.yaw = std::atan2(root[1], root[0]);
                    auto orientation = renderer::identity();
                    for (int col = 0; col < 3; ++col) {
                        const float length = std::sqrt(
                            root[col * 4] * root[col * 4] + root[col * 4 + 1] * root[col * 4 + 1]
                            + root[col * 4 + 2] * root[col * 4 + 2]
                        );
                        for (int row = 0; row < 3; ++row) {
                            orientation[col * 4 + row] =
                                root[col * 4 + row] / std::max(length, .0001f);
                        }
                    }
                    clip.upright =
                        renderer::inverse_affine(orientation).value_or(renderer::identity());
                }
                log::info(
                    "Special receiving motion {}: {} frames, {} actors",
                    source.item,
                    clip.actors.front().frames.size(),
                    clip.actors.size()
                );
                if (source.item == 1002 || source.item == 407 || source.item == 37) {
                    const auto lumps = image.global_lumps();
                    const auto cue = std::find_if(lumps.begin(), lumps.end(), [](const auto& l) {
                        return l.name == "vendor_audio/036";
                    });
                    if (cue == lumps.end()) {
                        throw std::runtime_error("missing weapon acquisition fanfare");
                    }
                    const auto bytes = image.read_lump(*cue);
                    const auto vag = assets::parse_vag(assets::ByteView(bytes));
                    const auto pcm = assets::decode_adpcm(vag.body);
                    // Keep the original pitch. Delay Zoomerator's fanfare so its tail
                    // lands on the last frame of the receiving motion.
                    const std::size_t delay =
                        source.item == 1002
                            ? std::size_t(
                                  std::max(
                                      0.0,
                                      double(source.seconds) * vag.header.sample_rate - pcm.size()
                                  )
                              )
                            : 0;
                    std::vector<std::int16_t> timed(delay, 0);
                    timed.insert(timed.end(), pcm.begin(), pcm.end());
                    audio::ui_sounds::set_pickup_audio(source.item, timed, vag.header.sample_rate);
                    log::info(
                        "Weapon receiving fanfare {}: {:.3f} seconds, delay {:.3f}",
                        source.item,
                        double(pcm.size()) / vag.header.sample_rate,
                        double(delay) / vag.header.sample_rate
                    );
                } else {
                    if (const auto voice = image.scene_speech(source.level, source.scene, 0)) {
                        const auto vag = assets::parse_vag(assets::ByteView(*voice));
                        const auto pcm = assets::decode_adpcm(vag.body);
                        const float lead =
                            std::max(0, -movie.audio_start()) / (pal ? 50.0f : 60.0f);
                        const auto skip = std::min(
                            pcm.size(), std::size_t((lead + source.start) * vag.header.sample_rate)
                        );
                        audio::ui_sounds::set_pickup_audio(
                            source.item, std::span(pcm).subspan(skip), vag.header.sample_rate
                        );
                        log::info(
                            "Receiving scene audio {}: {:.3f} seconds, native lead {:.3f}",
                            source.item,
                            double(pcm.size() - skip) / vag.header.sample_rate,
                            lead
                        );
                    }
                }
                g->special_pickup_clips[source.item] = std::move(clip);
            } catch (const std::exception& error) {
                log::warn("Special receiving motion {}: {}", source.item, error.what());
            }
        }
    } catch (const std::exception& error) {
        log::warn("Special receiving motions: {}", error.what());
    }
}
