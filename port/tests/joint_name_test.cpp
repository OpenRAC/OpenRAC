// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Joint names (scene/joint_name.h): joint_ and digits give the number (what follows the digits is
// ignored); any other name gives the fallback, the joint's place in its skin.

#include <climits>
#include <string>

#include "check.h"
#include "scene/joint_name.h"

using namespace openrac;
using scene::joint_number;

int main() {
    // The exporter's names: joint_NNN is NNN.
    CHECK(joint_number("joint_0", 9) == 0);
    CHECK(joint_number("joint_12", 9) == 12);
    CHECK(joint_number("joint_083", 9) == 83);   // leading zeros are digits
    CHECK(joint_number("joint_300", 9) == 300);  // not clamped here: the level loader does that
    // The exporter's post-scale nodes belong to their joint: joint_071_post is joint 71.
    CHECK(joint_number("joint_071_post", 9) == 71);
    CHECK(joint_number("joint_1x", 9) == 1);  // what follows the digits is ignored (as atoi did)
    // Any other name is the fallback: no digit after the prefix, a sign, or no prefix.
    CHECK(joint_number("joint_", 9) == 9);
    CHECK(joint_number("joint_abc", 9) == 9);  // was 0 (atoi), the fallback now
    CHECK(joint_number("joint__post", 9) == 9);
    CHECK(joint_number("joint_-1", 9) == 9);
    CHECK(joint_number("joint_ 1", 9) == 9);
    CHECK(joint_number("bone_12", 9) == 9);
    CHECK(joint_number("Joint_12", 9) == 9);
    CHECK(joint_number("joint", 9) == 9);
    CHECK(joint_number("", 9) == 9);
    // A number too big for an int is INT_MAX, not a wrap.
    CHECK(joint_number("joint_99999999999999999999", 9) == INT_MAX);

    return openrac::test::result();
}
