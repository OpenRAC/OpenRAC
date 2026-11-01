// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reject every C++ heap allocation, including the first clock/owner access and snapshot sort.
#include <cstddef>
#include <new>

#include "common/breadcrumbs.h"
#include "tests/check.h"

// Any attempted scalar or array allocation fails this process through the noexcept API.
void* operator new(std::size_t) {
    throw std::bad_alloc();
}

void* operator new[](std::size_t) {
    throw std::bad_alloc();
}

// Over-aligned allocations must not bypass the test's rejection either.
void* operator new(std::size_t, std::align_val_t) {
    throw std::bad_alloc();
}

void* operator new[](std::size_t, std::align_val_t) {
    throw std::bad_alloc();
}

// No allocation can succeed in this program, so there is no allocated object to release.
void operator delete(void*) noexcept {}

void operator delete[](void*) noexcept {}

void operator delete(void*, std::size_t) noexcept {}

void operator delete[](void*, std::size_t) noexcept {}

void operator delete(void*, std::align_val_t) noexcept {}

void operator delete[](void*, std::align_val_t) noexcept {}

void operator delete(void*, std::size_t, std::align_val_t) noexcept {}

void operator delete[](void*, std::size_t, std::align_val_t) noexcept {}

// Exercises first use and repeated wraparound with no allocator available.
int main() {
    using namespace openrac::crash;
    Breadcrumbs::Snapshot entries{};
    CHECK(breadcrumbs().snapshot(entries) == 0);
    // First TLS access, nested contexts and level metadata must also avoid the allocator.
    {
        const BreadcrumbScope main(BreadcrumbThread::Main, 1);
        breadcrumb("world depth prepass");
        {
            const BreadcrumbScope render(BreadcrumbThread::Render, 2);
            breadcrumb("shadow maps");
        }
        breadcrumb("restored main pass");
    }
    level_breadcrumb("level data read", 1);
    for (std::size_t i = 0; i < Breadcrumbs::kPerThread * 4; ++i) {
        breadcrumb(BreadcrumbThread::Main, "main event", static_cast<std::int64_t>(i));
        breadcrumb(BreadcrumbThread::Render, "render event", static_cast<std::int64_t>(i));
        CHECK(breadcrumbs().snapshot(entries) != 0);
    }
    CHECK(breadcrumbs().snapshot(entries) == entries.size());
    return openrac::test::result();
}
