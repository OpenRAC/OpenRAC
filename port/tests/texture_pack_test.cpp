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

bool save(const fs::path& file, const Rgba8Image& image) {
    disk()[file.filename().string()] = image;
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

    // The dump writes each texture once, by the name a pack uses.
    disk().clear();
    texture_pack::configure({}, pack / "dump", {load, save});
    image = small;
    CHECK(!texture_pack::replace(image));
    CHECK(disk().size() == 1 && disk().contains(name));
    disk().clear();
    CHECK(!texture_pack::replace(image) && disk().empty());

    // No pack and no dump: nothing happens.
    texture_pack::configure({}, {}, {load, save});
    CHECK(!texture_pack::replace(image));
    fs::remove_all(pack);
    return test::result();
}
