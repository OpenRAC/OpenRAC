// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The display lists of one picture (program/vsync.cpp, openrac_game_dma_send). A list sent with no
// vertical blank waited for since the last one ends the frame at the send, as on the console the
// loops paced by the DMA alone do (the title loop never waits). A game whose frame sends several
// lists and then waits for its vertical blank says so (openrac_game_picture_spans_sends): Up Your
// Arsenal's split-screen maps send each player's view in a list of its own, then the last list with
// the other player's view and the 2D over both. Those sends still run the vertical blank's work
// for the game, but the window draws nothing there: it draws the lists together at the blank the
// game waits for, as the console's frame buffer held them all.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openrac::program {

// The most lists one picture gathers: a game that said its picture spans sends and then stops
// waiting for vertical blanks has its picture ended at the next send. Four players' views, their
// header lists and the last list fit.
inline constexpr std::size_t kMostPictureLists = 8;

// The lists sent so far in the picture the game said spans its sends.
//
// A value, copied freely. vsync's one instance is used by the game's thread only.
class PictureLists {
public:
    // The lists the game sends until its next vertical blank are one picture.
    void span_sends() { m_spans = true; }

    // A list is sent with no vertical blank since `previous`, the list sent before it, was: returns
    // whether the picture goes on (the window draws nothing at that blank and keeps `previous` for
    // the one that ends the picture). Not without the game's word, nor past kMostPictureLists.
    [[nodiscard]] bool send_within(std::uint32_t previous) {
        if (!m_spans || m_lists.size() >= kMostPictureLists) {
            return false;
        }
        m_lists.push_back(previous);
        return true;
    }

    // At the vertical blank that ends the picture, whose last list is `last`: the lists sent before
    // it, in the order the game last sent each, each once and `last` not among them (a buffer sent
    // again holds what it was last sent with). The game's word ends with the picture.
    [[nodiscard]] std::vector<std::uint32_t> take(std::uint32_t last) {
        std::vector<std::uint32_t> lists;
        for (auto at = m_lists.rbegin(); at != m_lists.rend(); ++at) {
            if (*at != last && std::find(lists.begin(), lists.end(), *at) == lists.end()) {
                lists.push_back(*at);
            }
        }
        std::reverse(lists.begin(), lists.end());
        m_lists.clear();
        m_spans = false;
        return lists;
    }

private:
    std::vector<std::uint32_t> m_lists;  // In the order sent, as the sends came.
    bool m_spans = false;                // The game said this picture spans its sends.
};

}  // namespace openrac::program
