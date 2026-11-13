// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture packs (renderer/textures/texture_pack.h): names by hash, replacement at any size, the dump,
// animated replacements (frame by tick, the folder and GIF loaders) and the dump's cycle groups.
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "check.h"
#include "renderer/textures/texture_pack.h"

using namespace openrac;
using namespace openrac::renderer;
namespace fs = std::filesystem;

namespace {

// Pictures "on disk" for the fake reader and writer: by file name.
std::map<std::string, Rgba8Image>& disk() {
    static std::map<std::string, Rgba8Image> files;
    return files;
}

std::optional<Rgba8Image> load(const fs::path& file) {
    const auto found = disk().find(file.filename().string());
    return found == disk().end() ? std::nullopt : std::optional<Rgba8Image>(found->second);
}

// Saves by the path under the dump folder ("Interface/<hash>.png").
bool save(const fs::path& file, const Rgba8Image& image) {
    const fs::path in = file.parent_path().filename();
    const fs::path parent = file.parent_path().parent_path().filename();
    const bool nested = parent == "Planets";
    disk()[((nested ? parent / in : in) / file.filename()).generic_string()] = image;
    return true;
}

Rgba8Image solid(int w, int h, std::uint8_t red) {
    Rgba8Image image(w, h);
    for (std::size_t i = 0; i < image.pixels.size(); i += 4) {
        image.pixels[i] = red;
        image.pixels[i + 3] = 255;
    }
    return image;
}

// Pictures by whole path, for the animation tests (frames of different folders share names).
std::map<std::string, Rgba8Image>& frames_on_disk() {
    static std::map<std::string, Rgba8Image> files;
    return files;
}

std::optional<Rgba8Image> load_frame(const fs::path& file) {
    const auto found = frames_on_disk().find(file.string());
    return found == frames_on_disk().end() ? std::nullopt
                                           : std::optional<Rgba8Image>(found->second);
}

// A fake GIF of three frames, with the delays a GIF gives.
std::optional<texture_pack::Animation> load_gif(const fs::path&) {
    texture_pack::Animation gif;
    for (const std::uint8_t red : {std::uint8_t{1}, std::uint8_t{2}, std::uint8_t{3}}) {
        gif.frames.push_back(solid(4, 4, red));
        gif.durations_us.push_back(50000);
    }
    return gif;
}

// `count` frames of 1/60 s each (one tick), in `mode`.
texture_pack::Animation ticks(std::size_t count, texture_pack::AnimationMode mode) {
    texture_pack::Animation a;
    a.mode = mode;
    a.frames.resize(count);
    a.durations_us.assign(count, 1000000 / texture_pack::kTicksPerSecond);
    return a;
}

// Writes frames 1..count of `side` texels square (red = the frame's number) into `folder`.
void write_frames(const fs::path& folder, int count, int side) {
    fs::create_directories(folder);
    for (int i = 1; i <= count; ++i) {
        const fs::path file = folder / std::format("{:04}.png", i);
        std::ofstream(file) << "png";
        frames_on_disk()[file.string()] = solid(side, side, static_cast<std::uint8_t>(i));
    }
}

// Frame selection by tick in the three modes.
void check_frame_at() {
    using texture_pack::AnimationMode;
    const auto loop = ticks(3, AnimationMode::Loop);
    std::vector<std::size_t> seen;
    for (std::uint64_t t = 0; t < 7; ++t) {
        seen.push_back(texture_pack::frame_at(loop, t));
    }
    CHECK((seen == std::vector<std::size_t>{0, 1, 2, 0, 1, 2, 0}));

    const auto ping = ticks(4, AnimationMode::PingPong);
    seen.clear();
    for (std::uint64_t t = 0; t < 9; ++t) {
        seen.push_back(texture_pack::frame_at(ping, t));
    }
    CHECK((seen == std::vector<std::size_t>{0, 1, 2, 3, 2, 1, 0, 1, 2}));

    const auto once = ticks(3, AnimationMode::Once);
    CHECK(texture_pack::frame_at(once, 1) == 1 && texture_pack::frame_at(once, 2) == 2);
    CHECK(texture_pack::frame_at(once, 3) == 2 && texture_pack::frame_at(once, 1000) == 2);

    // At 10 a second (the default) each frame lasts 6 ticks; one frame never changes.
    auto slow = ticks(2, AnimationMode::Loop);
    slow.durations_us.assign(2, 100000);
    CHECK(texture_pack::frame_at(slow, 5) == 0 && texture_pack::frame_at(slow, 6) == 1);
    CHECK(texture_pack::frame_at(slow, 12) == 0);
    CHECK(texture_pack::frame_at(ticks(1, AnimationMode::PingPong), 99) == 0);
    CHECK(texture_pack::frame_at(ticks(2, AnimationMode::PingPong), 1) == 1);
    CHECK(texture_pack::frame_at(ticks(2, AnimationMode::PingPong), 2) == 0);
}

// The folder loader: numbered frames in order, animation.json and its defaults, a size mismatch.
void check_folders(const fs::path& root) {
    const texture_pack::Io io{load_frame, save, load_gif};
    std::string why;
    write_frames(root / "plain", 3, 4);
    std::ofstream(root / "plain" / "notes.txt") << "not a frame";
    auto plain = texture_pack::load_folder(root / "plain", io, why);
    CHECK(plain && plain->frames.size() == 3 && plain->frames[2].pixels[0] == 3);
    CHECK(plain && plain->mode == texture_pack::AnimationMode::Loop);
    CHECK(plain && plain->durations_us[0] == 100000);  // kDefaultFps
    CHECK(plain && plain->bytes() == 3 * 4 * 4 * 4);

    write_frames(root / "set", 2, 4);
    std::ofstream(root / "set" / "animation.json") << R"({"fps": 20, "mode": "pingpong"})";
    auto set = texture_pack::load_folder(root / "set", io, why);
    CHECK(
        set && set->durations_us[0] == 50000 && set->mode == texture_pack::AnimationMode::PingPong
    );

    write_frames(root / "fpsonly", 2, 4);
    std::ofstream(root / "fpsonly" / "animation.json") << R"({"mode": "once"})";
    auto once = texture_pack::load_folder(root / "fpsonly", io, why);
    CHECK(
        once && once->durations_us[0] == 100000 && once->mode == texture_pack::AnimationMode::Once
    );

    write_frames(root / "broken", 2, 4);
    std::ofstream(root / "broken" / "animation.json") << "{fps: oops";
    auto broken = texture_pack::load_folder(root / "broken", io, why);
    CHECK(
        broken && broken->durations_us[0] == 100000
        && broken->mode == texture_pack::AnimationMode::Loop
    );

    write_frames(root / "mixed", 2, 4);
    frames_on_disk()[(root / "mixed" / "0002.png").string()] = solid(8, 8, 2);
    why.clear();
    CHECK(!texture_pack::load_folder(root / "mixed", io, why));
    CHECK(
        why.find("one size") != std::string::npos
        && why.find("0002.png is 8x8") != std::string::npos
    );

    fs::create_directories(root / "empty");
    CHECK(!texture_pack::load_folder(root / "empty", io, why));
}

// A pack with a folder animation, a GIF and a mismatched folder, through replace().
void check_animated_pack(const fs::path& root) {
    const texture_pack::Io io{load_frame, save, load_gif};
    const auto first = solid(2, 2, 40);
    const auto second = solid(2, 2, 41);
    const auto third = solid(2, 2, 42);
    const auto stem = [](const Rgba8Image& image) {
        return texture_pack::file_name(texture_pack::image_hash(image)).substr(0, 16);
    };
    const fs::path pack = root / "pack";
    write_frames(pack / "Interface" / stem(first), 4, 16);
    std::ofstream(pack / (stem(second) + ".gif")) << "gif";
    write_frames(pack / stem(third), 2, 4);
    frames_on_disk()[(pack / stem(third) / "0002.png").string()] = solid(8, 8, 2);
    // A still picture by the same name as the mismatched animation is what it falls back to.
    std::ofstream(pack / (stem(third) + ".png")) << "png";
    frames_on_disk()[(pack / (stem(third) + ".png")).string()] = solid(6, 6, 7);
    texture_pack::configure(pack, {}, io);

    auto image = first;
    texture_pack::Replacement found;
    CHECK(texture_pack::replace(image, {}, &found) && image.width == 16 && image.pixels[0] == 1);
    CHECK(found.animation && found.animation->frames.size() == 4);
    CHECK(found.hash == texture_pack::image_hash(first));

    image = second;
    CHECK(texture_pack::replace(image, {}, &found) && image.width == 4 && image.pixels[0] == 1);
    CHECK(
        found.animation && found.animation->frames.size() == 3
        && found.animation->durations_us[0] == 50000
    );

    image = third;
    CHECK(
        texture_pack::replace(image, {}, &found) && image.width == 6 && found.animation == nullptr
    );

    // Without `found`, an animation's first frame is a still picture.
    image = first;
    CHECK(texture_pack::replace(image) && image.width == 16 && image.pixels[0] == 1);
}

// The dump's cycle groups: a slot that repeats a sequence twice, and one that does not.
void check_cycles(const fs::path& root) {
    texture_pack::CycleWatch watch;
    std::optional<std::vector<std::uint64_t>> cycle;
    std::uint64_t frame = 0;
    for (const std::uint64_t hash : {7, 7, 5, 5, 9, 7, 5, 9, 7}) {
        // A slot showing 7, 5, 9 and over again; reported once, when seen twice.
        if (auto found = watch.see(1, hash, frame += 3)) {
            CHECK(!cycle);
            cycle = found;
        }
    }
    CHECK(cycle && (*cycle == std::vector<std::uint64_t>{5, 9, 7}));
    // A pingpong lists its middle frame twice.
    std::optional<std::vector<std::uint64_t>> ping;
    for (const std::uint64_t hash : {1, 2, 3, 2, 1, 2, 3, 2, 1}) {
        if (auto found = watch.see(2, hash, frame += 3)) {
            ping = found;
        }
    }
    CHECK(ping && (*ping == std::vector<std::uint64_t>{1, 2, 3, 2}));
    // Changes too far apart, or new textures every time (a fade), are no cycle.
    bool any = false;
    for (const std::uint64_t hash : {1, 2, 1, 2, 1}) {
        frame += texture_pack::CycleWatch::kMaxFrameGap + 1;
        any = any || watch.see(3, hash, frame).has_value();
    }
    for (std::uint64_t hash = 100; hash < 200; ++hash) {
        any = any || watch.see(4, hash, frame += 3).has_value();
    }
    // Two textures taking turns every tick, or within one tick, are two things in one slot.
    for (const std::uint64_t hash : {1, 2, 1, 2, 1, 2, 1, 2}) {
        any = any || watch.see(5, hash, ++frame).has_value();
    }
    for (int tick = 0; tick < 8; ++tick) {
        frame += 3;
        any = any || watch.see(6, 1, frame).has_value() || watch.see(6, 2, frame).has_value();
    }
    CHECK(!any);

    // Written under Animated/<first>/, each frame once, order.txt in full.
    disk().clear();
    texture_pack::configure({}, root / "dump", {load, save, nullptr});
    CHECK(texture_pack::dumping());
    const auto a = solid(2, 2, 1);
    const auto b = solid(2, 2, 2);
    const std::vector<std::uint64_t> order{
        texture_pack::image_hash(a), texture_pack::image_hash(b), texture_pack::image_hash(a)
    };
    const std::vector<const Rgba8Image*> images{&a, &b, &a};
    texture_pack::dump_animated(order, images);
    CHECK(disk().size() == 2);
    const fs::path list =
        root / "dump" / "Animated" / texture_pack::file_name(order[0]).substr(0, 16) / "order.txt";
    std::ifstream in(list);
    std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    CHECK(text.find(texture_pack::file_name(order[1])) != std::string::npos);
    texture_pack::dump_animated(order, images);
    CHECK(disk().size() == 2);
}

}  // namespace

int main() {
    const auto small = solid(2, 2, 10);
    // The hash takes the size and every texel; the name is 16 hex digits.
    CHECK(texture_pack::image_hash(small) == texture_pack::image_hash(solid(2, 2, 10)));
    CHECK(texture_pack::image_hash(small) != texture_pack::image_hash(solid(2, 2, 11)));
    CHECK(texture_pack::image_hash(solid(4, 1, 10)) != texture_pack::image_hash(solid(1, 4, 10)));
    const std::string name = texture_pack::file_name(texture_pack::image_hash(small));
    CHECK(name.size() == 20 && name.ends_with(".png"));

    // A pack folder with that name (and a file that is not one) replaces the texture at its size.
    const fs::path pack = fs::temp_directory_path() / "openrac_texture_pack_test";
    fs::remove_all(pack);
    fs::create_directories(pack / "sub");
    std::ofstream(pack / "sub" / name) << "png";
    std::ofstream(pack / "notes.txt") << "not a texture";
    disk()[name] = solid(8, 8, 200);
    texture_pack::configure(pack, {}, {load, save});
    auto image = small;
    CHECK(texture_pack::replace(image) && image.width == 8 && image.pixels[0] == 200);
    auto other = solid(2, 2, 99);
    CHECK(!texture_pack::replace(other) && other.width == 2);
    CHECK(texture_pack::replaced_count() == 1);

    // A pack file that cannot be read keeps the original.
    disk().clear();
    texture_pack::configure(pack, {}, {load, save});
    image = small;
    CHECK(!texture_pack::replace(image) && image.width == 2);

    // The dump writes each texture once, by the name a pack uses, into its origin's folder.
    disk().clear();
    texture_pack::configure({}, pack / "dump", {load, save});
    image = small;
    CHECK(!texture_pack::replace(image, {.interface = true}));
    CHECK(disk().size() == 1 && disk().contains("Interface/" + name));
    disk().clear();
    CHECK(!texture_pack::replace(image, {.interface = true}) && disk().empty());

    // A level's texture goes under its planet, and moves to Shared/ when another planet has it.
    const auto planet = [](int level) {
        return level == 1 ? std::string("Oozla") : std::string("Bad/Name:");
    };
    disk().clear();
    texture_pack::configure({}, pack / "dump2", {load, save}, planet);
    CHECK(texture_pack::dump_folder({.interface = true}) == "Interface");
    CHECK(texture_pack::dump_folder({.level = 1}) == fs::path("Planets") / "Oozla");
    CHECK(texture_pack::dump_folder({.level = 2}) == fs::path("Planets") / "Bad Name");
    CHECK(texture_pack::dump_folder({}) == "Shared");
    auto world = solid(4, 4, 30);
    const std::string world_name = texture_pack::file_name(texture_pack::image_hash(world));
    CHECK(!texture_pack::replace(world, {.level = 1}));
    CHECK(disk().contains("Planets/Oozla/" + world_name));
    // The fake wrote nothing; a file for the move to find.
    std::ofstream(pack / "dump2" / "Planets" / "Oozla" / world_name) << "png";
    CHECK(!texture_pack::replace(world, {.level = 1}));  // the same planet: stays
    CHECK(fs::exists(pack / "dump2" / "Planets" / "Oozla" / world_name));
    CHECK(!texture_pack::replace(world, {.level = 3}));  // another planet: shared
    CHECK(fs::exists(pack / "dump2" / "Shared" / world_name));
    CHECK(!fs::exists(pack / "dump2" / "Planets" / "Oozla" / world_name));

    // A later run knows what the folder has: nothing is written again, a move still happens.
    disk().clear();
    fs::create_directories(pack / "dump2" / "Planets" / "Oozla");
    auto later = solid(4, 4, 31);
    const std::string later_name = texture_pack::file_name(texture_pack::image_hash(later));
    std::ofstream(pack / "dump2" / "Planets" / "Oozla" / later_name) << "png";
    texture_pack::configure({}, pack / "dump2", {load, save}, planet);
    CHECK(!texture_pack::replace(later, {.level = 1}) && disk().empty());
    CHECK(!texture_pack::replace(later, {.level = 5}) && disk().empty());
    CHECK(fs::exists(pack / "dump2" / "Shared" / later_name));

    // No pack and no dump: nothing happens.
    texture_pack::configure({}, {}, {load, save});
    CHECK(!texture_pack::replace(image));
    CHECK(!texture_pack::dumping());

    check_frame_at();
    check_folders(pack / "folders");
    check_animated_pack(pack);
    check_cycles(pack);
    fs::remove_all(pack);
    return test::result();
}
