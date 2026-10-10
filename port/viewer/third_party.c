/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The implementations of the viewer's single-header libraries, compiled once
 * here with warnings off (port/cmake/Window.cmake): cgltf (MIT, Johannes
 * Kuhlmann) reads glTF; stb_image and stb_image_write (MIT or public domain,
 * Sean Barrett) read and write PNG. Both are fetched by CMake at pinned
 * versions (port/cmake/Dependencies.cmake); neither is in the repository. */

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
