// Written by port/tools/hostgen/release_rac1.py from game_state.cpp's and profile.cpp's PAL tables. Do not edit.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// PAL v1.00 (SCES-50916, disc v1.00): the profile's tables for this disc, PAL v2.00's addresses as this release
// has them (the release's check, or versionmap's exact pair; release_carry.json lists any other).
#pragma once

namespace openrac::game::rac1 {

inline constexpr GameAddresses kRac1Pal1{0x00187000, 0x00187210, 0x0018CC80, 0x0015FEDC, 0x0015FEE0, 0.756f, 0x00160EB8, 0x00160ED0};  // proven
inline constexpr GameAddresses kRac1Pal1Levels[19] = {
    {0x00166DC0, 0x00166FD0, 0x0016CA40, 0x0015FF9C, 0x0015FFA0, 0.756f, 0x00161178, 0x00161190},  // 0 (proven)
    {0x001671C0, 0x001673D0, 0x0016CE40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 1 (proven)
    {0x00167340, 0x00167550, 0x0016CFC0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 2 (proven)
    {0x00166E40, 0x00167050, 0x0016CAC0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 3 (proven)
    {0x00166EC0, 0x001670D0, 0x0016CB40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 4 (proven)
    {0x001671C0, 0x001673D0, 0x0016CE40, 0x0015FF9C, 0x0015FFA0, 0.756f, 0x00161178, 0x00161190},  // 5 (proven)
    {0x001674C0, 0x001676D0, 0x0016D140, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 6 (proven)
    {0x00166DC0, 0x00166FD0, 0x0016CA40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 7 (proven)
    {0x00167540, 0x00167750, 0x0016D1C0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 8 (proven)
    {0x00166EC0, 0x001670D0, 0x0016CB40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 9 (proven)
    {0x001671C0, 0x001673D0, 0x0016CE40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 10 (proven)
    {0x00167740, 0x00167950, 0x0016D3C0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 11 (proven)
    {0x001671C0, 0x001673D0, 0x0016CE40, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 12 (proven)
    {0x00167040, 0x00167250, 0x0016CCC0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 13 (proven)
    {0x001674C0, 0x001676D0, 0x0016D140, 0x0015FF9C, 0x0015FFA0, 0.756f, 0x00161178, 0x00161190},  // 14 (proven)
    {0x00167340, 0x00167550, 0x0016CFC0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 15 (proven)
    {0x00167140, 0x00167350, 0x0016CDC0, 0x0015FF9C, 0x0015FFA0, 0.756f, 0x00161178, 0x00161190},  // 16 (proven)
    {0x00167640, 0x00167850, 0x0016D2C0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 17 (proven)
    {0x00167740, 0x00167950, 0x0016D3C0, 0x0015FF5C, 0x0015FF60, 0.756f, 0x00161138, 0x00161150},  // 18 (proven)
};

// The executable's addresses (profile.cpp's ExecutableAddresses).
#define OPENRAC_RAC1_PAL1_EXECUTABLE {0x001CAA00u, 0x0013F350u, 0x0015F5ECu, 0x001CDD00u}

}  // namespace openrac::game::rac1
