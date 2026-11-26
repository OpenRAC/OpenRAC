/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * RAC1 cover geometry for an explicitly owned native 2D view, not an interpreter of residual state.
 * Retail bindings must first account for their packet state, target and frame timing; these helpers
 * do not authorize replacing a packet that still has those consumers. See docs/port/RAC1_COVERS.md.
 */
#ifndef OPENRAC_RAC1_SCREEN_COVER_H
#define OPENRAC_RAC1_SCREEN_COVER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A native view in game pixels, with an optional borrowed half-open clip (x0, y0, x1, y1).
 * `clip` remains readable for the call. `anchor` names the reached list cursor, or explicitly 0
 * for a native-only recording. `whole_target` extends horizontal placement past the 4:3 picture.
 * The caller owns the native target, blend/test policy and clip; no residual state is inferred. */
typedef struct rac1_cover_view {
    int width;
    int height;
    const int* clip;
    uint32_t anchor;
    int whole_target;
} rac1_cover_view;

/* Packs byte-valued game colour channels without clamping alpha at 0x80 (the game's full alpha).
 * Returns 0 for a missing output or a channel outside 0..255, leaving the output unchanged. */
int rac1_cover_colour(int red, int green, int blue, int alpha, uint32_t* rgba);

/* Emits a clipped Fill using OPAQUE or MIX; `rect` is half-open game pixels, NULL for the view.
 * Returns 1 when handled, including an empty intersection; 0 requires the caller's original path.
 * Explicit transparent draws are retained because their alpha writes need not be invisible. */
int rac1_cover_fill(const rac1_cover_view* view, const int* rect, uint32_t rgba, int blend);

/* As rac1_cover_fill, with a depth of its own (openrac_frame_fill_depth): `depth` of a
 * `depth_bits`-bit buffer, compared by `depth_test` (OPENRAC_DEPTH_*), and the `writes`
 * (OPENRAC_WRITE_*) where it passes. Returns 1 when the window drew it or the intersection is empty;
 * 0 requires the caller's original path (invalid view or blend, no window, value out of range). */
int rac1_cover_fill_depth(
    const rac1_cover_view* view,
    const int* rect,
    uint32_t rgba,
    int blend,
    uint32_t depth,
    int depth_bits,
    int depth_test,
    unsigned writes
);

/* Emits opaque black letterbox bars for an already updated retail height in 0..24 game pixels.
 * Returns 0 for an invalid view/height; does not update the game's enable flag, height or clock. */
int rac1_cover_letterbox(const rac1_cover_view* view, int height);

/* The same bars at `depth` of a `depth_bits`-bit buffer, compared by `depth_test` and written with
 * every colour channel, as the retail bar's own packet draws them. Returns 1 when both were drawn,
 * 0 when the caller must keep its packet (invalid view or height, no window). */
int rac1_cover_letterbox_depth(
    const rac1_cover_view* view, int height, uint32_t depth, int depth_bits, int depth_test
);

/* Clears only depth of an explicitly owned whole target, leaving colour and alpha untouched.
 * Returns 1 for a full or empty clip; 0 for invalid views, partial clips or picture-only scope,
 * which the existing Clear API cannot represent. No retail clear site is implied by this helper. */
int rac1_cover_clear_depth(const rac1_cover_view* view);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_RAC1_SCREEN_COVER_H */
