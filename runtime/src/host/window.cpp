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
  if (!SDL_Init(SDL_INIT_VIDEO)) {
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
    }
  }
  return running;
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

void Window::close() {
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
