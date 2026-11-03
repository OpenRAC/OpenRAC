// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Texture packs (renderer/texture_pack.h): names by hash, replacement at any size, the dump.
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "check.h"
#include "renderer/texture_pack.h"

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
    fs::remove_all(pack);
    return test::result();
}
