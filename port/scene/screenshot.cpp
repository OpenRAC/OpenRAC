// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "scene/screenshot.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stb_image_write.h>
#include <thread>

#include "common/log.h"

namespace openrac::scene {

namespace {

// One thread, so a run's shots are written one at a time, in order.
struct Writer {
    std::mutex lock;
    std::condition_variable wake;
    std::deque<std::function<void()>> jobs;
    bool closing = false;
    std::thread thread{[this] { run(); }};

    ~Writer() {
        {
            const std::lock_guard held(lock);
            closing = true;
        }
        wake.notify_one();
        thread.join();
    }

    void run() {
        std::unique_lock held(lock);

        for (;;) {
            wake.wait(held, [this] { return closing || !jobs.empty(); });

            if (jobs.empty()) {
                return;
            }

            const std::function<void()> job = std::move(jobs.front());
            jobs.pop_front();
            held.unlock();
            job();
            held.lock();
        }
    }
};

}  // namespace

bool write_png(
    const std::string& path, int width, int height, const std::vector<std::uint8_t>& rgba
) {
    return stbi_write_png(path.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
}

void write_png_later(
    std::string path, int width, int height, std::vector<std::uint8_t> rgba, std::string written
) {
    static Writer writer;
    {
        const std::lock_guard held(writer.lock);
        writer.jobs.emplace_back([=, rgba = std::move(rgba)] {
            if (write_png(path, width, height, rgba)) {
                log::info("{}", written);
            } else {
                log::warn("cannot write {}", path);
            }
        });
    }
    writer.wake.notify_one();
}

}  // namespace openrac::scene
