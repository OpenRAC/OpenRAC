// Written by port/tools/hostgen/release_carry.py from profile.cpp's kNtsc and this release's maps. Do not edit.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// USA v2.00 (SCUS-97268, Greatest Hits): port/game/rac2/profile.cpp's table (Release) for this disc, v1.01's addresses as this
// release has them. Written by port/tools/hostgen/release_carry.py; do not edit.

namespace openrac::game::rac2 {
namespace {

constexpr Release kNtsc2{
    {0x001B54F0, 0x001B8FC0, 0x001B1AE0, 0x001B1AE4},  // the release's build
    {
    {0x001B8F70, 0x001BD240, 0x001B1E20, 0x001B1E24},  // 0 (the release's build)
    {0x001B96F0, 0x001BD9C0, 0x001B2560, 0x001B2564},  // 1 (versionmap: exact)
    {0x001BA730, 0x001BEA00, 0x001B35A0, 0x001B35A4},  // 2 (versionmap: exact)
    {0x001B9BB0, 0x001BDE80, 0x001B2A20, 0x001B2A24},  // 3 (versionmap: exact)
    {0x001B98F0, 0x001BDBC0, 0x001B24E0, 0x001B24E4},  // 4 (the release's build, versionmap: exact)
    {0x001B9870, 0x001BDB40, 0x001B24A0, 0x001B24A4},  // 5 (the release's build, versionmap: exact)
    {0x001B9A70, 0x001BDD40, 0x001B2820, 0x001B2824},  // 6 (versionmap: exact)
    {0x001B93B0, 0x001BD680, 0x001B2260, 0x001B2264},  // 7 (versionmap: exact)
    {0x001B9D30, 0x001BE000, 0x001B29E0, 0x001B29E4},  // 8 (versionmap: exact)
    {0x001B9BB0, 0x001BDE80, 0x001B27E0, 0x001B27E4},  // 9 (the release's build, versionmap: exact)
    {0x001B9830, 0x001BDB00, 0x001B24A0, 0x001B24A4},  // 10 (versionmap: exact)
    {0x001BAB70, 0x001BEE40, 0x001B3620, 0x001B3624},  // 11 (the release's build, versionmap: exact)
    {0x001B9FB0, 0x001BE280, 0x001B2E20, 0x001B2E24},  // 12 (the release's build, versionmap: exact)
    {0x001B9A70, 0x001BDD40, 0x001B28A0, 0x001B28A4},  // 13 (versionmap: exact)
    {0x001B96B0, 0x001BD980, 0x001B2520, 0x001B2524},  // 14 (the release's build, versionmap: exact)
    {0x001B9970, 0x001BDC40, 0x001B25A0, 0x001B25A4},  // 15 (the release's build, versionmap: exact)
    {0x001B9CB0, 0x001BDF80, 0x001B2AA0, 0x001B2AA4},  // 16 (the release's build, versionmap: exact)
    {0x001B9DF0, 0x001BE0C0, 0x001B2C20, 0x001B2C24},  // 17 (versionmap: exact)
    {0x001B9970, 0x001BDC40, 0x001B27A0, 0x001B27A4},  // 18 (the release's build, versionmap: exact)
    {0x001B9BF0, 0x001BDEC0, 0x001B29A0, 0x001B29A4},  // 19 (the release's build, versionmap: exact)
    {0x001BA170, 0x001BE440, 0x001B2F20, 0x001B2F24},  // 20 (the release's build, versionmap: exact)
    {},                                                // 21: no level
    {0x001B9E30, 0x001BE100, 0x001B20A0, 0x001B20A4},  // 22 (the release's build, versionmap: exact)
    {0x001BA7F0, 0x001BEAC0, 0x001B2720, 0x001B2724},  // 23 (the release's build, versionmap: exact)
    {0x001B8870, 0x001BCB40, 0x001B1720, 0x001B1724},  // 24 (versionmap: exact)
    {0x001B93B0, 0x001BD680, 0x001B2020, 0x001B2024},  // 25 (versionmap: exact)
    {0x001B8D30, 0x001BD000, 0x001B1BA0, 0x001B1BA4},  // 26 (versionmap: exact)
    {},                                                // 27: no level
    {},                                                // 28: no level
    {},                                                // 29: no level
    {0x001B9E70, 0x001BE140, 0x001B2BA0, 0x001B2BA4},  // 30 (the release's build, versionmap: exact)
    },
    0x001A7290,  // the release's build
    {{{0x001A7A51, 6}, {0x001A7A4A, 8}, {0x001A7A53, 7}, {0x001A7A52, 5}}},  // versionmap: exact
    0x001A72C0,  // versionmap: exact
    0x00152C18,  // versionmap: between
    0x00152C30,  // versionmap: between
    {0x001B213Cu, 0x001B2020u, 0x001B2134u, 0x001B2018u},  // the release's build, the same instruction of the paired function
    0x001B1B10u,  // the release's build
    0x00189E20u,  // versionmap: exact
};

}  // namespace
}  // namespace openrac::game::rac2
