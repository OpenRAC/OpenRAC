// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <string>

#include "check.h"
#include "program/custom_map.h"

using openrac::program::CustomMap;
using openrac::program::parse_custom_map;

int main() {
    const std::string prefix =
        R"({"format":1,"id":"arena","name":"New arena","game":"rac1","version":"pal",)";
    CustomMap map;
    std::string error;
    CHECK(parse_custom_map(prefix + R"("level":0,"spawn":[20,30,4]})", "rac1", "pal", map, error));
    CHECK(map.id == "arena" && map.name == "New arena" && map.level == 0);
    CHECK(map.spawn[0] == 20 && map.spawn[1] == 30 && map.spawn[2] == 4);
    CHECK(map.heading == 0 && map.skip_movies && map.skip_cutscenes);
    CHECK(parse_custom_map(
        prefix
            + R"("level":18,"spawn":[-2,3,4],"heading":1.25,"skip_movies":false,"skip_cutscenes":false})",
        "rac1",
        "pal",
        map,
        error
    ));
    CHECK(map.level == 18 && map.heading == 1.25f && !map.skip_movies && !map.skip_cutscenes);

    for (const std::string fields :
         {R"("level":19,"spawn":[1,2,3])",
          R"("level":-1,"spawn":[1,2,3])",
          R"("level":0.5,"spawn":[1,2,3])",
          R"("level":true,"spawn":[1,2,3])",
          R"("level":0,"spawn":[1,2])",
          R"("level":0,"spawn":[1,2,"3"])",
          R"("level":0,"spawn":[1,2,1e40])",
          R"("level":0,"spawn":[1,2,3],"heading":1e40)",
          R"("level":0,"spawn":[1,2,3],"skip_movies":1)",
          R"("level":0,"spawn":[1,2,3],"skip_cutscenes":1)"}) {
        CHECK(!parse_custom_map(prefix + fields + "}", "rac1", "pal", map, error));
        CHECK(!error.empty());
        // A rejected file never changes the previously accepted configuration.
        CHECK(map.level == 18 && !map.skip_movies && !map.skip_cutscenes);
    }

    CHECK(!parse_custom_map(prefix + R"("level":0,"spawn":[1,2,3]})", "rac1", "ntsc", map, error));
    CHECK(!parse_custom_map(prefix + R"("level":0,"spawn":[1,2,3]})", "rac2", "pal", map, error));
    std::vector<CustomMap> destinations;
    const std::string harbor =
        R"({"format":1,"game":"rac1","version":"pal","id":"harbor","name":"Harbor","level":2,"spawn":[1,2,3]})";
    const std::string arena =
        R"({"format":1,"game":"rac1","version":"pal","id":"arena","name":"Arena","level":2,"spawn":[4,5,6]})";
    const auto registry = [&](const std::string& entries) {
        return R"({"format":1,"game":"rac1","version":"pal","destinations":[)" + entries + "]}";
    };
    CHECK(
        openrac::program::parse_destinations(
            registry(harbor + "," + arena), "rac1", "pal", destinations, error
        )
    );
    CHECK(
        destinations.size() == 2 && destinations[0].id == "harbor" && destinations[1].spawn[0] == 4
    );
    CHECK(!openrac::program::parse_destinations(
        registry(harbor + "," + harbor), "rac1", "pal", destinations, error
    ));
    CHECK(destinations.size() == 2);  // invalid registry never partly replaces a valid one
    CHECK(!openrac::program::parse_destinations(registry(""), "rac1", "pal", destinations, error));
    CHECK(
        !openrac::program::parse_destinations(registry(harbor), "rac1", "ntsc", destinations, error)
    );
    std::string traversal = harbor;
    traversal.replace(traversal.find("harbor"), 6, "../escape");
    CHECK(!openrac::program::parse_destinations(
        registry(traversal), "rac1", "pal", destinations, error
    ));
    return openrac::test::result();
}
