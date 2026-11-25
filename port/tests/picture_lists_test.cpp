// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests program::PictureLists (game/common/program/picture_lists.h): which display lists one
// picture draws when the game said its picture spans its sends. The expected values follow from the
// rule its header states and from Up Your Arsenal's measured split-screen sends.

#include <cstdint>
#include <vector>

#include "game/common/program/picture_lists.h"
#include "tests/check.h"

namespace {

using openrac::program::kMostPictureLists;
using openrac::program::PictureLists;

// Without the game's word every send ends the picture, as before: nothing is gathered.
void test_sends_end_the_picture_by_default() {
    PictureLists lists;
    CHECK(!lists.send_within(0x0056F000u));
    CHECK(lists.take(0x0068F000u).empty());
}

// Up Your Arsenal's two-player frame: the header list in buffer H, player 0's view in buffer O,
// then H again with player 1's view and the 2D. The blank that ends the picture draws O, then H.
void test_split_screen_frame_draws_every_list() {
    constexpr std::uint32_t kHeader = 0x0068F000u;
    constexpr std::uint32_t kOther = 0x0056F000u;
    PictureLists lists;
    lists.span_sends();
    CHECK(lists.send_within(kHeader));  // O is sent: the header list was sent before it.
    lists.span_sends();                 // The second player's layout says it again.
    CHECK(lists.send_within(kOther));   // H is sent again: O was sent before it.
    CHECK((lists.take(kHeader) == std::vector<std::uint32_t>{kOther}));
    // The word ends with the picture: the next frame's sends end at the send until it is given.
    CHECK(!lists.send_within(kOther));
    CHECK(lists.take(kHeader).empty());
}

// Each list once, in the order of its last send, the picture's last list not among them.
void test_lists_in_the_order_last_sent() {
    PictureLists lists;
    lists.span_sends();
    CHECK(lists.send_within(1));
    CHECK(lists.send_within(2));
    CHECK(lists.send_within(1));
    CHECK(lists.send_within(3));
    CHECK((lists.take(4) == std::vector<std::uint32_t>{2, 1, 3}));
    lists.span_sends();
    CHECK(lists.send_within(1));
    CHECK(lists.send_within(2));
    CHECK(lists.send_within(3));
    CHECK((lists.take(2) == std::vector<std::uint32_t>{1, 3}));
}

// A picture that never meets a vertical blank ends at a send after kMostPictureLists lists.
void test_picture_ends_after_the_most_lists() {
    PictureLists lists;
    lists.span_sends();
    for (std::uint32_t n = 0; n < kMostPictureLists; ++n) {
        CHECK(lists.send_within(n + 1));
    }
    CHECK(!lists.send_within(100));
    CHECK(lists.take(100).size() == kMostPictureLists);
    CHECK(!lists.send_within(101));
}

}  // namespace

int main() {
    test_sends_end_the_picture_by_default();
    test_split_screen_frame_draws_every_list();
    test_lists_in_the_order_last_sent();
    test_picture_ends_after_the_most_lists();
    return openrac::test::result();
}
