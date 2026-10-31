// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Receiver-normal orientation checks shared by the probe GPU test driver.

#pragma once

namespace openrac::test {

// Checks both camera sides, normal signs and triangle windings against directional illumination.
void test_probe_receiver_normals();

}  // namespace openrac::test
