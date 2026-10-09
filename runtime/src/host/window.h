// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cstddef>

#include "ps2/gs.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Gamepad;
struct SDL_AudioStream;

namespace host {

// A controller as the games read it: buttons as a bit mask in the console's
// order (select, L3, R3, start, up, right, down, left, L2, R2, L1, R1,
// triangle, circle, cross, square) and two sticks, 0x80 centred.
struct Controls {
  unsigned buttons = 0;
  unsigned char left_x = 0x80, left_y = 0x80, right_x = 0x80, right_y = 0x80;
};

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
  // What is held now, on the keyboard and on the first game controller.
  // Keys: W A S D the left stick, I J K L the right stick, the arrows the
  // direction pad, space cross, F square, E circle, R triangle, Q L1, left
  // shift R1, Z L2, C R2, return start, backspace select.
  Controls controls() const;
  // Sound to be heard: pairs of left and right samples at `rate`. What does
  // not come in time is silence; what would pile up is dropped.
  void play(const short* samples, std::size_t frames, int rate);
  void present(const ps2::Image& image, float aspect);
  void close();

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;
  SDL_Gamepad* gamepad_ = nullptr;
  SDL_AudioStream* audio_ = nullptr;
  int audio_rate_ = 0;
  int texture_width_ = 0, texture_height_ = 0;
};

}  // namespace host
