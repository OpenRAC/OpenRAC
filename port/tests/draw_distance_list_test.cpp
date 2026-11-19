// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Each game's draw_distance.tsv reads without errors, and the reader skips comments and names the
// rows it leaves out.
#include <string_view>

#include "draw_distance_lists.h"
#include "game/common/draw_distance_list.h"
#include "tests/check.h"

namespace {
using openrac::host::DrawDistanceList;

void games_lists_read() {
    const DrawDistanceList rac1{openrac::host::kRac1DrawDistanceList};
    CHECK(rac1.errors().empty());
    CHECK(rac1.has(1161));  // Pokitaru's backdrop
    CHECK(!rac1.has(0));

    const DrawDistanceList rac2{openrac::host::kRac2DrawDistanceList};
    CHECK(rac2.errors().empty());
    CHECK(rac2.has(2198));  // an Electrolyzer board's controller
    CHECK(rac2.has(3927));  // Grelbin's Arctic Leviathan
    CHECK(rac2.has(4304));  // Grelbin's Y.E.T.I.
}

void rows_that_do_not_fit_are_named() {
    const DrawDistanceList list{"# a comment\n"
                                "\n"
                                "12\tany\tname\treason\r\n"
                                "7\tany\tname\treason\n"
                                "x\tany\tname\treason\n"
                                "8\tany\tname\n"
                                "9\tany\t\treason\n"
                                "7\tany\tname\treason\n"};
    CHECK(list.classes().size() == 2);
    CHECK(list.classes()[0] == 7 && list.classes()[1] == 12);
    CHECK(list.errors().size() == 4);
}

}  // namespace

int main() {
    games_lists_read();
    rows_that_do_not_fit_are_named();
    return openrac::test::result();
}
