// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Source composition shared by world rendering and the build-time shader recipes.
#pragma once
#include <initializer_list>
#include <string>
#include <string_view>

namespace openrac::renderer::world {
std::string compose(std::string_view stage, std::initializer_list<std::string_view> parts);
}
