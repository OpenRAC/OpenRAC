// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "renderer/ui/overlay_images.h"

#include <map>
#include <mutex>

namespace openrac::renderer::ui::overlay_images {

namespace {

constexpr std::size_t kChannels = 4;

struct Entry {
    const void* owner = nullptr;
    std::shared_ptr<const Picture> picture;
};

struct Store {
    std::mutex lock;
    std::map<int, Entry> pictures;
    int next = 1;
    std::uint64_t changes = 0;
};

Store& store() {
    static Store s;
    return s;
}

}  // namespace

int create(
    const void* owner,
    int width,
    int height,
    std::span<const std::uint8_t> rgba,
    bool smooth,
    Refusal* why
) {
    auto refuse = [&](Refusal r) {
        if (why != nullptr) {
            *why = r;
        }

        return 0;
    };

    if (owner == nullptr || width < 1 || height < 1 || width > kSideMax || height > kSideMax) {
        return refuse(Refusal::Bad);
    }

    const std::size_t bytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;

    if (rgba.size() < bytes) {
        return refuse(Refusal::Bad);
    }

    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    std::size_t kept = 0;
    std::size_t kept_bytes = 0;

    for (const auto& [id, entry] : s.pictures) {
        if (entry.owner == owner) {
            ++kept;
            kept_bytes += entry.picture->rgba.size();
        }
    }

    if (kept >= kPerOwnerMax || kept_bytes + bytes > kBytesPerOwnerMax) {
        return refuse(Refusal::Full);
    }

    auto picture = std::make_shared<Picture>();
    picture->width = width;
    picture->height = height;
    picture->smooth = smooth;
    picture->rgba.assign(rgba.begin(), rgba.begin() + static_cast<std::ptrdiff_t>(bytes));
    const int id = s.next++;
    s.pictures[id] = Entry{owner, std::move(picture)};
    ++s.changes;

    if (why != nullptr) {
        *why = Refusal::None;
    }

    return id;
}

bool destroy(const void* owner, int id) {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    const auto it = s.pictures.find(id);

    if (owner == nullptr || it == s.pictures.end() || it->second.owner != owner) {
        return false;
    }

    s.pictures.erase(it);
    ++s.changes;
    return true;
}

void drop(const void* owner) {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    const std::size_t before = s.pictures.size();
    std::erase_if(s.pictures, [&](const auto& item) { return item.second.owner == owner; });

    if (s.pictures.size() != before) {
        ++s.changes;
    }
}

std::shared_ptr<const Picture> find(int id) {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    const auto it = s.pictures.find(id);
    return it != s.pictures.end() ? it->second.picture : nullptr;
}

bool owns(const void* owner, int id) {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    const auto it = s.pictures.find(id);
    return owner != nullptr && it != s.pictures.end() && it->second.owner == owner;
}

std::size_t count() {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    return s.pictures.size();
}

std::uint64_t changes() {
    Store& s = store();
    const std::lock_guard<std::mutex> lock(s.lock);
    return s.changes;
}

}  // namespace openrac::renderer::ui::overlay_images
