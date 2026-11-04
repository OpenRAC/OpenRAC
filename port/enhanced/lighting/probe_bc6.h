// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Offline BC6H tiles with a shared endpoint palette for octahedral border pairs.
#pragma once
#include "enhanced/lighting/probe_compression_tile.h"

namespace openrac::lighting::compression {
// Compresses four BC6H blocks while preserving exact paired-border decoded values.
std::array<Block, 4> encode_bc6_probe(const Tile& tile);
// Compresses a normal four-by-four HDR block using Compressonator's CPU encoder.
Block encode_bc6_block(std::span<const Float3, 16> pixels);
}  // namespace openrac::lighting::compression
