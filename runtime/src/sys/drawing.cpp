// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "drawing.h"

#include <utility>

namespace sys {

Drawing::~Drawing() {
  if (thread_.joinable()) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      quit_ = true;
    }
    work_.notify_all();
    thread_.join();
  }
}

void Drawing::start() {
  if (!thread_.joinable()) {
    thread_ = std::thread([this] { loop(); });
  }
}

void Drawing::gather(Kind kind, const u8* data, std::size_t bytes) {
  if (open_used_ && open_.kind != kind) {
    send();
  }
  if (!open_used_) {
    open_.kind = kind;
    open_used_ = true;
    if (open_.data.capacity() == 0) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!spare_.empty()) {
        open_.data = std::move(spare_.back());
        spare_.pop_back();
      }
    }
  }
  open_.data.insert(open_.data.end(), data, data + bytes);
}

void Drawing::send() {
  if (!open_used_) {
    return;
  }
  open_used_ = false;
  Command command = std::move(open_);
  open_ = Command{};
  give(std::move(command));
}

void Drawing::privileged(u32 address, u64 value) {
  send();
  Command command;
  command.kind = kPrivileged;
  command.address = address;
  command.value = value;
  give(std::move(command));
}

void Drawing::vblank(bool odd_field) {
  send();
  Command command;
  command.kind = kVblank;
  command.value = odd_field ? 1 : 0;
  give(std::move(command));
}

void Drawing::present() {
  send();
  if (thread_.joinable()) {
    std::unique_lock<std::mutex> lock(mutex_);
    done_.wait(lock, [this] { return presents_waiting_ < 2; });
    presents_waiting_++;
  }
  Command command;
  command.kind = kPresent;
  give(std::move(command));
}

void Drawing::give(Command&& command) {
  if (!thread_.joinable()) {
    run(command);
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(command));
  }
  work_.notify_one();
}

void Drawing::sync() {
  send();
  if (!thread_.joinable()) {
    return;
  }
  std::unique_lock<std::mutex> lock(mutex_);
  done_.wait(lock, [this] { return queue_.empty() && !busy_; });
}

bool Drawing::picture(ps2::Image& out) {
  std::lock_guard<std::mutex> lock(picture_mutex_);
  if (picture_shown_) {
    out = picture_;
  }
  return picture_shown_;
}

void Drawing::run(Command& command) {
  switch (command.kind) {
    case kVif:
      graphics_.vif.write(command.data.data(), command.data.size());
      break;
    case kGif:
      graphics_.gif.write(3, command.data.data(), command.data.size() / 16);
      break;
    case kPrivileged:
      graphics_.gs.write_privileged(command.address, command.value);
      break;
    case kVblank:
      graphics_.gs.vblank(command.value != 0);
      break;
    case kPresent: {
      ps2::Image image;
      {
        // The buffer is used again; only its contents are replaced.
        std::lock_guard<std::mutex> lock(picture_mutex_);
        image = std::move(spare_picture_);
      }
      bool shown = graphics_.gs.display(image);
      std::lock_guard<std::mutex> lock(picture_mutex_);
      if (shown) {
        spare_picture_ = std::move(picture_);
        picture_ = std::move(image);
      } else {
        spare_picture_ = std::move(image);
      }
      picture_shown_ = shown;
      break;
    }
  }
}

void Drawing::loop() {
  std::unique_lock<std::mutex> lock(mutex_);
  for (;;) {
    work_.wait(lock, [this] { return quit_ || !queue_.empty(); });
    if (queue_.empty()) {
      return;  // asked to stop, and nothing is left
    }
    Command command = std::move(queue_.front());
    queue_.pop_front();
    busy_ = true;
    lock.unlock();

    run(command);

    lock.lock();
    busy_ = false;
    if (command.kind == kPresent) {
      presents_waiting_--;
    }
    if (command.data.capacity() && spare_.size() < 8) {
      command.data.clear();
      spare_.push_back(std::move(command.data));
    }
    done_.notify_all();
  }
}

}  // namespace sys
