// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What Android says of its display's HDR, which SDL does not (platform/window.cpp's hdr_display):
// GameActivity.displayHdr through JNI (android/display_hdr.cpp). Only on Android.
#pragma once

#include "common/hdr.h"

namespace openrac::platform {

#if defined(__ANDROID__)
// The display the game's activity is on: whether the game can show HDR there, by its HDR/SDR
// headroom (Android 14 and later: relative to the system's SDR white) or as HDR10, and its peak.
// False, with `display` left unknown, when Java cannot be asked.
bool android_display_hdr(hdr::Display& display);

// The window asks the system for HDR's headroom while `on` (Window.setColorMode, Android 14 and
// later; nothing before), `headroom` times SDR white at most (Android 15 and later).
void android_window_hdr(bool on, float headroom);
#endif

}  // namespace openrac::platform
