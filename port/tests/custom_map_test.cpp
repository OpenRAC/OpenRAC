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
    CHECK(map.heading == 0 && map.skip_movies);
    CHECK(parse_custom_map(
        prefix + R"("level":18,"spawn":[-2,3,4],"heading":1.25,"skip_movies":false})",
        "rac1",
        "pal",
        map,
        error
    ));
    CHECK(map.level == 18 && map.heading == 1.25f && !map.skip_movies);

    for (const std::string fields :
         {R"("level":19,"spawn":[1,2,3])",
          R"("level":-1,"spawn":[1,2,3])",
          R"("level":0.5,"spawn":[1,2,3])",
          R"("level":true,"spawn":[1,2,3])",
          R"("level":0,"spawn":[1,2])",
          R"("level":0,"spawn":[1,2,"3"])",
          R"("level":0,"spawn":[1,2,1e40])",
          R"("level":0,"spawn":[1,2,3],"heading":1e40)",
          R"("level":0,"spawn":[1,2,3],"skip_movies":1)"}) {
        CHECK(!parse_custom_map(prefix + fields + "}", "rac1", "pal", map, error));
        CHECK(!error.empty());
        // A rejected file never changes the previously accepted configuration.
        CHECK(map.level == 18 && !map.skip_movies);
    }

    CHECK(!parse_custom_map(prefix + R"("level":0,"spawn":[1,2,3]})", "rac1", "ntsc", map, error));
    CHECK(!parse_custom_map(prefix + R"("level":0,"spawn":[1,2,3]})", "rac2", "pal", map, error));
    return openrac::test::result();
}
