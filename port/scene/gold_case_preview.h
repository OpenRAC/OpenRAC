#pragma once
#include <vector>

#include "pickup_preview.h"

namespace openrac::scene::gold_case_preview {
using Adjustment = pickup_preview::Adjustment;

struct Case {
    std::uint32_t address;
    int source, display;
    renderer::Vec3 position;
    renderer::Mat4 anchor;
};

inline std::vector<Case> cases;

inline bool source(int id) {
    return id >= 0x5B0 && id < 0x5BA;
}

inline Adjustment defaults(int id = 0) {
    switch (id) {
#include "gold_case_defaults.inc"
        default:
            Adjustment a;
            a.model.spin = false;
            return a;
    }
}

inline Adjustment get(int id) {
    return defaults(id);
}
}  // namespace openrac::scene::gold_case_preview
