// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Force-included only while compiling the Arm ASR library: the standard headers it uses without
// including them, and OpenRAC's volk loader, whose functions its Vulkan backend calls directly.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <volk.h>
