// Written by port/tools/hostgen/release_rac1.py from game_state.cpp's and profile.cpp's PAL tables. Do not edit.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// NTSC-J v1.02 (SCPS-15037): the profile's tables for this disc, PAL v2.00's addresses as this release
// has them (the release's check, or versionmap's exact pair; release_carry.json lists any other).
#pragma once

namespace openrac::game::rac1 {

inline constexpr GameAddresses kRac1Jp{0x00187180, 0x00187390, 0x0018CE00, 0x0016001C, 0x00160020, 0.756f, 0x00160FF8, 0x00161010};  // proven
inline constexpr GameAddresses kRac1JpLevels[19] = {
    {0x00166EC0, 0x001670D0, 0x0016CB40, 0x0016009C, 0x001600A0, 0.756f, 0x00161278, 0x00161290},  // 0 (proven)
    {0x001672C0, 0x001674D0, 0x0016CF40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 1 (proven)
    {0x00167440, 0x00167650, 0x0016D0C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 2 (proven)
    {0x00166F40, 0x00167150, 0x0016CBC0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 3 (proven)
    {0x00166FC0, 0x001671D0, 0x0016CC40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 4 (proven)
    {0x001672C0, 0x001674D0, 0x0016CF40, 0x0016009C, 0x001600A0, 0.756f, 0x00161278, 0x00161290},  // 5 (proven)
    {0x001675C0, 0x001677D0, 0x0016D240, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 6 (proven)
    {0x00166EC0, 0x001670D0, 0x0016CB40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 7 (proven)
    {0x00167640, 0x00167850, 0x0016D2C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 8 (proven)
    {0x00166FC0, 0x001671D0, 0x0016CC40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 9 (proven)
    {0x001672C0, 0x001674D0, 0x0016CF40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 10 (proven)
    {0x00167840, 0x00167A50, 0x0016D4C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 11 (proven)
    {0x001672C0, 0x001674D0, 0x0016CF40, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 12 (proven)
    {0x00167140, 0x00167350, 0x0016CDC0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 13 (proven)
    {0x001675C0, 0x001677D0, 0x0016D240, 0x0016009C, 0x001600A0, 0.756f, 0x00161278, 0x00161290},  // 14 (proven)
    {0x00167440, 0x00167650, 0x0016D0C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 15 (proven)
    {0x00167240, 0x00167450, 0x0016CEC0, 0x0016009C, 0x001600A0, 0.756f, 0x00161278, 0x00161290},  // 16 (proven)
    {0x00167740, 0x00167950, 0x0016D3C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 17 (proven)
    {0x00167840, 0x00167A50, 0x0016D4C0, 0x0016005C, 0x00160060, 0.756f, 0x00161238, 0x00161250},  // 18 (proven)
};

// The executable's addresses (profile.cpp's ExecutableAddresses).
#define OPENRAC_RAC1_JP_EXECUTABLE {0x001CAC00u, 0x0013F450u, 0x0015F6F0u, 0x001CDF00u}

}  // namespace openrac::game::rac1
