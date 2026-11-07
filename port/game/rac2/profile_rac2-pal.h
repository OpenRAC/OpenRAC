// Written by port/tools/hostgen/release_carry.py from profile.cpp's kNtsc and this release's maps. Do not edit.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// PAL (SCES-51607): port/game/rac2/profile.cpp's table (Release) for this disc, v1.01's addresses as this
// release has them. Written by port/tools/hostgen/release_carry.py; do not edit.

namespace openrac::game::rac2 {
namespace {

constexpr Release kPal{
    {0x001B5570, 0x001B9040, 0x001B1B60, 0x001B1B64},  // versionmap: exact
    {
    {0x001B9030, 0x001BD300, 0x001B1EA0, 0x001B1EA4},  // 0 (the release's build, versionmap: exact)
    {0x001B9730, 0x001BDA00, 0x001B2560, 0x001B2564},  // 1 (the release's build, versionmap: exact)
    {0x001BA7F0, 0x001BEAC0, 0x001B3620, 0x001B3624},  // 2 (the release's build, versionmap: exact)
    {0x001B9BF0, 0x001BDEC0, 0x001B2A20, 0x001B2A24},  // 3 (the release's build, versionmap: exact)
    {0x001B9970, 0x001BDC40, 0x001B2560, 0x001B2564},  // 4 (the release's build, versionmap: exact)
    {0x001B98F0, 0x001BDBC0, 0x001B2520, 0x001B2524},  // 5 (versionmap: exact)
    {0x001B9AF0, 0x001BDDC0, 0x001B28A0, 0x001B28A4},  // 6 (the release's build)
    {0x001B9470, 0x001BD740, 0x001B22E0, 0x001B22E4},  // 7 (versionmap: exact)
    {0x001B9D70, 0x001BE040, 0x001B29E0, 0x001B29E4},  // 8 (the release's build, versionmap: exact)
    {0x001B9C70, 0x001BDF40, 0x001B2860, 0x001B2864},  // 9 (the release's build, versionmap: exact)
    {0x001B98B0, 0x001BDB80, 0x001B2520, 0x001B2524},  // 10 (versionmap: exact)
    {0x001BABF0, 0x001BEEC0, 0x001B36A0, 0x001B36A4},  // 11 (the release's build, versionmap: exact)
    {0x001BA070, 0x001BE340, 0x001B2EA0, 0x001B2EA4},  // 12 (versionmap: exact)
    {0x001B9AF0, 0x001BDDC0, 0x001B2920, 0x001B2924},  // 13 (versionmap: exact)
    {0x001B96F0, 0x001BD9C0, 0x001B2520, 0x001B2524},  // 14 (the release's build, versionmap: exact)
    {0x001B9A30, 0x001BDD00, 0x001B2620, 0x001B2624},  // 15 (the release's build, versionmap: exact)
    {0x001B9D30, 0x001BE000, 0x001B2B20, 0x001B2B24},  // 16 (versionmap: exact)
    {0x001B9E70, 0x001BE140, 0x001B2CA0, 0x001B2CA4},  // 17 (versionmap: exact)
    {0x001B9A30, 0x001BDD00, 0x001B2820, 0x001B2824},  // 18 (versionmap: exact)
    {0x001B9CB0, 0x001BDF80, 0x001B2A20, 0x001B2A24},  // 19 (versionmap: exact)
    {0x001BA230, 0x001BE500, 0x001B2FA0, 0x001B2FA4},  // 20 (the release's build, versionmap: exact)
    {},                                                // 21: no level
    {0x001B9EF0, 0x001BE1C0, 0x001B2120, 0x001B2124},  // 22 (the release's build, versionmap: exact)
    {0x001BA8B0, 0x001BEB80, 0x001B27A0, 0x001B27A4},  // 23 (the release's build, versionmap: exact)
    {0x001B88F0, 0x001BCBC0, 0x001B17A0, 0x001B17A4},  // 24 (versionmap: exact)
    {0x001B9430, 0x001BD700, 0x001B20A0, 0x001B20A4},  // 25 (versionmap: exact)
    {0x001B8DF0, 0x001BD0C0, 0x001B1C20, 0x001B1C24},  // 26 (versionmap: exact)
    {},                                                // 27: no level
    {},                                                // 28: no level
    {},                                                // 29: no level
    {0x001B9FF0, 0x001BE2C0, 0x001B2D20, 0x001B2D24},  // 30 (versionmap: exact)
    },
    0x001A7AA0,  // the release's build
    {{{0x001A7AD1, 6}, {0x001A7ACA, 8}, {0x001A7AD3, 7}, {0x001A7AD2, 5}}},  // versionmap: exact
    0x001A7340,  // the release's build
    0x00152C98,  // versionmap: between
    0x00152CB0,  // versionmap: between
    {0x001B21BCu, 0x001B20A0u, 0x001B21B4u, 0x001B2098u},  // the same instruction of the paired function, versionmap: exact
    0x001B1B90u,  // versionmap: exact
    0x00189EA0u,  // the release's build
};

}  // namespace
}  // namespace openrac::game::rac2
