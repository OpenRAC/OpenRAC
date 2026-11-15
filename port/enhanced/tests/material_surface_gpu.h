// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Production surface-pass regression cases, called with the test runner's graphics context current.
#pragma once

namespace openrac::test {
// Checks cutout transparency in ordinary static and moby metal glass passes.
void test_glass_cutout_coverage();
// Checks Lit/Original switching, neighboring surfaces, classic output and Studio extraction.
void test_original_chrome_coverage();
}  // namespace openrac::test
