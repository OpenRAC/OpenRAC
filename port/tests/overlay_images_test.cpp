// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' overlay pictures (renderer/ui/overlay_images.h, interface 1.41): copied, each its
// owner's, ids never given twice, the limits, and an owner's let-go and unload.

#include <cstdint>
#include <vector>

#include "check.h"
#include "renderer/ui/overlay_images.h"

namespace {

using namespace openrac::renderer::ui;

const int kA = 1;
const int kB = 2;

std::vector<std::uint8_t> pixels(int width, int height, std::uint8_t value) {
    return std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 4, value);
}

void a_picture_is_a_copy_and_its_owners() {
    auto rgba = pixels(2, 3, 7);
    const int id = overlay_images::create(&kA, 2, 3, rgba, true);
    CHECK(id > 0);
    rgba[0] = 99;  // the store keeps its own copy
    const auto picture = overlay_images::find(id);
    CHECK(picture != nullptr && picture->width == 2 && picture->height == 3 && picture->smooth);
    CHECK(picture->rgba.size() == 24 && picture->rgba[0] == 7);
    CHECK(overlay_images::owns(&kA, id) && !overlay_images::owns(&kB, id));
    // Another owner cannot let it go.
    CHECK(!overlay_images::destroy(&kB, id) && overlay_images::find(id) != nullptr);
    const std::uint64_t changes = overlay_images::changes();
    CHECK(overlay_images::destroy(&kA, id) && overlay_images::find(id) == nullptr);
    CHECK(overlay_images::changes() == changes + 1);
    // The picture held stays valid; the id is not given again.
    CHECK(picture->rgba[0] == 7);
    const int next = overlay_images::create(&kA, 1, 1, pixels(1, 1, 0), false);
    CHECK(next > id);
    CHECK(overlay_images::destroy(&kA, next));
}

void bad_pictures_are_refused() {
    overlay_images::Refusal why = overlay_images::Refusal::None;
    const auto one = pixels(1, 1, 0);
    CHECK(
        overlay_images::create(nullptr, 1, 1, one, true, &why) == 0
        && why == overlay_images::Refusal::Bad
    );
    CHECK(overlay_images::create(&kA, 0, 1, one, true, &why) == 0);
    CHECK(overlay_images::create(&kA, overlay_images::kSideMax + 1, 1, one, true, &why) == 0);
    // Fewer bytes than the size needs.
    CHECK(
        overlay_images::create(&kA, 2, 1, one, true, &why) == 0
        && why == overlay_images::Refusal::Bad
    );
    CHECK(!overlay_images::destroy(&kA, 12345));
}

void an_owner_has_a_limit_and_an_unload() {
    overlay_images::Refusal why = overlay_images::Refusal::None;
    const auto one = pixels(1, 1, 0);
    std::vector<int> ids;

    for (std::size_t i = 0; i < overlay_images::kPerOwnerMax; ++i) {
        ids.push_back(overlay_images::create(&kA, 1, 1, one, false));
        CHECK(ids.back() > 0);
    }

    CHECK(
        overlay_images::create(&kA, 1, 1, one, false, &why) == 0
        && why == overlay_images::Refusal::Full
    );
    // Another owner's count is its own.
    const int other = overlay_images::create(&kB, 1, 1, one, false);
    CHECK(other > 0);
    overlay_images::drop(&kA);

    for (const int id : ids) {
        CHECK(overlay_images::find(id) == nullptr);
    }

    CHECK(overlay_images::find(other) != nullptr && overlay_images::count() == 1);
    overlay_images::drop(&kB);
    CHECK(overlay_images::count() == 0);
}

void an_owner_has_a_byte_limit() {
    overlay_images::Refusal why = overlay_images::Refusal::None;
    // 4096 x 4096 x 4 is 64 MB: two fit in 128 MB, a third does not.
    const int side = overlay_images::kSideMax;
    const auto big = pixels(side, side, 1);
    const int a = overlay_images::create(&kA, side, side, big, true);
    const int b = overlay_images::create(&kA, side, side, big, true);
    CHECK(a > 0 && b > 0);
    CHECK(
        overlay_images::create(&kA, side, side, big, true, &why) == 0
        && why == overlay_images::Refusal::Full
    );
    overlay_images::drop(&kA);
}

}  // namespace

int main() {
    a_picture_is_a_copy_and_its_owners();
    bad_pictures_are_refused();
    an_owner_has_a_limit_and_an_unload();
    an_owner_has_a_byte_limit();
    return openrac::test::result();
}
