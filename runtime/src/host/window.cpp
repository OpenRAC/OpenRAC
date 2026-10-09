// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "window.h"

#include <SDL3/SDL.h>

#include <cstdio>

namespace host {

Window::~Window() {
  close();
}

bool Window::open(const char* title, int width, int height) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
    std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return false;
  }
  if (!SDL_CreateWindowAndRenderer(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                   &window_, &renderer_)) {
    std::fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
    return false;
  }
  SDL_SetRenderVSync(renderer_, 1);
  return true;
}

bool Window::pump() {
  SDL_Event event;
  bool running = true;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      running = false;
    } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
      running = false;
    } else if (event.type == SDL_EVENT_GAMEPAD_ADDED && !gamepad_) {
      gamepad_ = SDL_OpenGamepad(event.gdevice.which);
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad_ && SDL_GetGamepadID(gamepad_) == event.gdevice.which) {
      SDL_CloseGamepad(gamepad_);
      gamepad_ = nullptr;
    }
  }
  return running;
}

Controls Window::controls() const {
  enum : unsigned {
    kSelect = 1u << 0, kL3 = 1u << 1, kR3 = 1u << 2, kStart = 1u << 3, kUp = 1u << 4, kRight = 1u << 5, kDown = 1u << 6,
    kLeft = 1u << 7, kL2 = 1u << 8, kR2 = 1u << 9, kL1 = 1u << 10, kR1 = 1u << 11, kTriangle = 1u << 12,
    kCircle = 1u << 13, kCross = 1u << 14, kSquare = 1u << 15,
  };
  Controls c;
  int left_x = 0, left_y = 0, right_x = 0, right_y = 0;  // -127..127

  const bool* keys = SDL_GetKeyboardState(nullptr);
  static constexpr struct { SDL_Scancode key; unsigned button; } key_buttons[] = {
      {SDL_SCANCODE_UP, kUp}, {SDL_SCANCODE_DOWN, kDown}, {SDL_SCANCODE_LEFT, kLeft}, {SDL_SCANCODE_RIGHT, kRight},
      {SDL_SCANCODE_SPACE, kCross}, {SDL_SCANCODE_F, kSquare}, {SDL_SCANCODE_E, kCircle}, {SDL_SCANCODE_R, kTriangle},
      {SDL_SCANCODE_Q, kL1}, {SDL_SCANCODE_LSHIFT, kR1}, {SDL_SCANCODE_Z, kL2}, {SDL_SCANCODE_C, kR2},
      {SDL_SCANCODE_RETURN, kStart}, {SDL_SCANCODE_BACKSPACE, kSelect},
  };
  for (const auto& k : key_buttons) {
    if (keys[k.key]) {
      c.buttons |= k.button;
    }
  }
  left_x += (keys[SDL_SCANCODE_D] ? 127 : 0) - (keys[SDL_SCANCODE_A] ? 127 : 0);
  left_y += (keys[SDL_SCANCODE_S] ? 127 : 0) - (keys[SDL_SCANCODE_W] ? 127 : 0);
  right_x += (keys[SDL_SCANCODE_L] ? 127 : 0) - (keys[SDL_SCANCODE_J] ? 127 : 0);
  right_y += (keys[SDL_SCANCODE_K] ? 127 : 0) - (keys[SDL_SCANCODE_I] ? 127 : 0);

  if (gamepad_) {
    static constexpr struct { SDL_GamepadButton pad; unsigned button; } pad_buttons[] = {
        {SDL_GAMEPAD_BUTTON_BACK, kSelect}, {SDL_GAMEPAD_BUTTON_LEFT_STICK, kL3}, {SDL_GAMEPAD_BUTTON_RIGHT_STICK, kR3},
        {SDL_GAMEPAD_BUTTON_START, kStart}, {SDL_GAMEPAD_BUTTON_DPAD_UP, kUp}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, kRight},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, kDown}, {SDL_GAMEPAD_BUTTON_DPAD_LEFT, kLeft},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, kL1}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, kR1},
        {SDL_GAMEPAD_BUTTON_NORTH, kTriangle}, {SDL_GAMEPAD_BUTTON_EAST, kCircle}, {SDL_GAMEPAD_BUTTON_SOUTH, kCross},
        {SDL_GAMEPAD_BUTTON_WEST, kSquare},
    };
    for (const auto& b : pad_buttons) {
      if (SDL_GetGamepadButton(gamepad_, b.pad)) {
        c.buttons |= b.button;
      }
    }
    if (SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 8000) {
      c.buttons |= kL2;
    }
    if (SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8000) {
      c.buttons |= kR2;
    }
    left_x += SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX) / 258;
    left_y += SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY) / 258;
    right_x += SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX) / 258;
    right_y += SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY) / 258;
  }

  auto stick = [](int value) {
    value = value < -127 ? -127 : value > 127 ? 127 : value;
    return static_cast<unsigned char>(0x80 + value);
  };
  c.left_x = stick(left_x);
  c.left_y = stick(left_y);
  c.right_x = stick(right_x);
  c.right_y = stick(right_y);
  return c;
}

void Window::present(const ps2::Image& image, float aspect) {
  if (!renderer_ || image.width <= 0 || image.height <= 0) {
    return;
  }
  if (!texture_ || texture_width_ != image.width || texture_height_ != image.height) {
    if (texture_) {
      SDL_DestroyTexture(texture_);
    }
    // Image stores R in the low byte, which is RGBA in memory order.
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, image.width,
                                 image.height);
    if (!texture_) {
      std::fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
      return;
    }
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_LINEAR);
    texture_width_ = image.width;
    texture_height_ = image.height;
  }
  SDL_UpdateTexture(texture_, nullptr, image.pixels.data(), image.width * 4);

  int out_w = 0, out_h = 0;
  SDL_GetRenderOutputSize(renderer_, &out_w, &out_h);
  float w = static_cast<float>(out_w), h = static_cast<float>(out_h);
  SDL_FRect dst;
  if (w / h > aspect) {
    dst.h = h;
    dst.w = h * aspect;
  } else {
    dst.w = w;
    dst.h = w / aspect;
  }
  dst.x = (w - dst.w) / 2;
  dst.y = (h - dst.h) / 2;

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  SDL_RenderTexture(renderer_, texture_, nullptr, &dst);
  SDL_RenderPresent(renderer_);
}

void Window::play(const short* samples, std::size_t frames, int rate) {
  if (!audio_ || audio_rate_ != rate) {
    if (audio_) {
      SDL_DestroyAudioStream(audio_);
    }
    SDL_AudioSpec spec{SDL_AUDIO_S16, 2, rate};
    audio_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    audio_rate_ = rate;
    if (!audio_) {
      std::fprintf(stderr, "no sound: %s\n", SDL_GetError());
      return;
    }
    SDL_ResumeAudioStreamDevice(audio_);
  }
  // More than a fifth of a second waiting means the machine ran ahead of
  // the loudspeaker: let it catch up rather than fall ever further behind.
  if (SDL_GetAudioStreamQueued(audio_) > rate * 4 / 5) {
    SDL_ClearAudioStream(audio_);
  }
  SDL_PutAudioStreamData(audio_, samples, static_cast<int>(frames * 4));
}

void Window::close() {
  if (audio_) {
    SDL_DestroyAudioStream(audio_);
    audio_ = nullptr;
  }
  if (gamepad_) {
    SDL_CloseGamepad(gamepad_);
    gamepad_ = nullptr;
  }
  if (texture_) {
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
  }
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    SDL_Quit();
  }
}

}  // namespace host
