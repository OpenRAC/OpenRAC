// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include "ps2/gs.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace host {

// A window that shows one image per frame, scaled to fit with a fixed aspect
// ratio. The games' frame buffers are not square-pixelled (512 by 448 shown
// at 4:3), so the ratio is given, not derived from the image.
class Window {
 public:
  Window() = default;
  ~Window();
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool open(const char* title, int width, int height);
  // Handles pending events. False once the user has asked to quit.
  bool pump();
  void present(const ps2::Image& image, float aspect);
  void close();

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;
  int texture_width_ = 0, texture_height_ = 0;
};

}  // namespace host
