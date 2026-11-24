/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * screen_cover.h's native command adapter. Rectangle arithmetic is independent of game addresses.
 * The 24-pixel letterbox limit and black/full-alpha bars follow draw_letterbox_bars in the pinned
 * RAC1 PAL src/game/render/draw.c (seen in game code). No matched C or packet body is copied here.
 */
#include "screen_cover.h"

#include <stddef.h>

#include "openrac/frame.h"

/* Fill records currently use the recorder's default 2D coordinate space (frame_record.h). */
static const float cover_width = 512.0f;
static const float cover_height = 448.0f;

/* The bar's maximum is 24 game pixels (seen in game code: draw_letterbox_bars). */
enum {
    LETTERBOX_HEIGHT = 24
};

/* An intersection in half-open game pixels, before conversion to recorder coordinates. */
typedef struct cover_bounds {
    int x0;
    int y0;
    int x1;
    int y1;
} cover_bounds;

/* Intersects the requested rectangle with the view and its explicit native clip.
 * Returns -1 for an invalid view, 0 for an empty intersection, 1 otherwise. */
static int clipped_bounds(const rac1_cover_view* view, const int* rect, cover_bounds* out) {
    /* An invalid view must fall back rather than divide by a missing dimension. */
    if (view == NULL || view->width <= 0 || view->height <= 0) {
        return -1;
    }

    *out = (cover_bounds){0, 0, view->width, view->height};

    /* Both optional rectangles constrain the same half-open native view. */
    for (int n = 0; n < 2; ++n) {
        const int* limit = n == 0 ? rect : view->clip;

        /* An absent rectangle places no further constraint on the view. */
        if (limit == NULL) {
            continue;
        }

        out->x0 = out->x0 > limit[0] ? out->x0 : limit[0];
        out->y0 = out->y0 > limit[1] ? out->y0 : limit[1];
        out->x1 = out->x1 < limit[2] ? out->x1 : limit[2];
        out->y1 = out->y1 < limit[3] ? out->y1 : limit[3];
    }

    return out->x0 < out->x1 && out->y0 < out->y1;
}

/* Returns whether the clipped region is the entire view, with no lost edge pixels. */
static int whole_view(const rac1_cover_view* view, const cover_bounds* bounds) {
    return bounds->x0 == 0 && bounds->y0 == 0 && bounds->x1 == view->width
           && bounds->y1 == view->height;
}

int rac1_cover_colour(int red, int green, int blue, int alpha, uint32_t* rgba) {
    /* Out-of-byte values can spill into adjacent retail fields and require the original path. */
    if (rgba == NULL || (unsigned)red > UINT8_MAX || (unsigned)green > UINT8_MAX
        || (unsigned)blue > UINT8_MAX || (unsigned)alpha > UINT8_MAX) {
        return 0;
    }

    /* Unsigned shifts keep all eight alpha bits, including over-full game values. */
    *rgba = (uint32_t)red | (uint32_t)green << 8 | (uint32_t)blue << 16 | (uint32_t)alpha << 24;
    return 1;
}

/* A clipped cover's rectangle in Fill's screen space and its placement flags. */
typedef struct cover_rect {
    float normalized[4];
    const float* rect; /* NULL for the whole target, else normalized */
    int placement;     /* OPENRAC_PICTURE_WHOLE when it extends past the 4:3 picture */
} cover_rect;

/* Clips and normalizes a cover. Returns -1 for an unsupported blend or invalid view (the caller's
 * original path), 0 for an empty intersection (nothing to draw), 1 with `out` filled. */
static int prepare_cover(const rac1_cover_view* view, const int* rect, int blend, cover_rect* out) {
    /* Other blends belong to the still-residual screen effects, not this cover contract. */
    if (blend != OPENRAC_BLEND_OPAQUE && blend != OPENRAC_BLEND_MIX) {
        return -1;
    }

    cover_bounds bounds;
    const int clipped = clipped_bounds(view, rect, &bounds);

    /* Empty coverage is handled without drawing; an invalid view must take the original path. */
    if (clipped <= 0) {
        return clipped;
    }

    /* Normalize into Fill's existing screen space, including PAL's 416-line video mode. */
    out->normalized[0] = (float)bounds.x0 * cover_width / (float)view->width;
    out->normalized[1] = (float)bounds.y0 * cover_height / (float)view->height;
    out->normalized[2] = (float)bounds.x1 * cover_width / (float)view->width;
    out->normalized[3] = (float)bounds.y1 * cover_height / (float)view->height;

    /* NULL means the whole target, so a picture-only fill must keep its explicit rectangle. */
    const int whole = view->whole_target && whole_view(view, &bounds);
    out->rect = whole ? NULL : out->normalized;
    out->placement = view->whole_target && !whole ? OPENRAC_PICTURE_WHOLE : 0;
    return 1;
}

int rac1_cover_fill(const rac1_cover_view* view, const int* rect, uint32_t rgba, int blend) {
    cover_rect cover;
    const int prepared = prepare_cover(view, rect, blend, &cover);

    /* Empty coverage is handled; an unsupported blend or view keeps the original path. */
    if (prepared <= 0) {
        return prepared == 0;
    }

    openrac_frame_anchor(view->anchor);
    openrac_frame_fill(cover.rect, rgba, blend | cover.placement);
    return 1;
}

int rac1_cover_fill_depth(
    const rac1_cover_view* view,
    const int* rect,
    uint32_t rgba,
    int blend,
    uint32_t depth,
    int depth_bits,
    int depth_test,
    unsigned writes
) {
    cover_rect cover;
    const int prepared = prepare_cover(view, rect, blend, &cover);

    /* Empty coverage is handled; an unsupported blend or view keeps the original path. */
    if (prepared <= 0) {
        return prepared == 0;
    }

    /* Without a window (or with a value the recorder refuses) nothing is drawn: say so. */
    openrac_frame_anchor(view->anchor);
    const int placed = blend | cover.placement;
    return openrac_frame_fill_depth(cover.rect, rgba, placed, depth, depth_bits, depth_test, writes)
           != 0;
}

/* The bars are unblended, full-alpha black (seen in game code: draw_letterbox_bars). */
static const uint32_t letterbox_black = 0x80000000u;

/* One bar at its own depth: every channel and the depth, as the bar's packet writes them under
 * the game's open masks. */
static int letterbox_bar_depth(
    const rac1_cover_view* view, const int* rect, uint32_t depth, int depth_bits, int depth_test
) {
    const unsigned writes = OPENRAC_WRITE_COLOUR | OPENRAC_WRITE_DEPTH;
    return rac1_cover_fill_depth(
        view, rect, letterbox_black, OPENRAC_BLEND_OPAQUE, depth, depth_bits, depth_test, writes
    );
}

/* The two bars' rows for an already updated height. Returns -1 for an out-of-contract height or
 * invalid view (the retail path), 0 for a retracted bar (no geometry), 1 with both rows filled. */
static int letterbox_rows(const rac1_cover_view* view, int height, int* top, int* bottom) {
    cover_bounds bounds;

    /* Out-of-contract counters must retain the retail path rather than being silently clamped. */
    if (height < 0 || height > LETTERBOX_HEIGHT || clipped_bounds(view, NULL, &bounds) < 0) {
        return -1;
    }

    /* A fully retracted bar emits no geometry (seen in game code: draw_letterbox_bars). */
    if (height == 0) {
        return 0;
    }

    /* Clip before subtraction so malformed tiny native views cannot overflow integer arithmetic. */
    const int rows = height < view->height ? height : view->height;
    top[0] = 0;
    top[1] = 0;
    top[2] = view->width;
    top[3] = rows;
    bottom[0] = 0;
    bottom[1] = view->height - rows;
    bottom[2] = view->width;
    bottom[3] = view->height;
    return 1;
}

int rac1_cover_letterbox(const rac1_cover_view* view, int height) {
    int top[4];
    int bottom[4];
    const int rows = letterbox_rows(view, height, top, bottom);

    /* A retracted bar is handled without geometry; an invalid one keeps the retail path. */
    if (rows <= 0) {
        return rows == 0;
    }

    return rac1_cover_fill(view, top, letterbox_black, OPENRAC_BLEND_OPAQUE)
           && rac1_cover_fill(view, bottom, letterbox_black, OPENRAC_BLEND_OPAQUE);
}

int rac1_cover_letterbox_depth(
    const rac1_cover_view* view, int height, uint32_t depth, int depth_bits, int depth_test
) {
    int top[4];
    int bottom[4];
    const int rows = letterbox_rows(view, height, top, bottom);

    /* A retracted bar is handled without geometry; an invalid one keeps the retail path. */
    if (rows <= 0) {
        return rows == 0;
    }

    return letterbox_bar_depth(view, top, depth, depth_bits, depth_test)
           && letterbox_bar_depth(view, bottom, depth, depth_bits, depth_test);
}

int rac1_cover_clear_depth(const rac1_cover_view* view) {
    cover_bounds bounds;
    const int clipped = clipped_bounds(view, NULL, &bounds);

    /* Empty clips need no clear; invalid dimensions must retain their original behavior. */
    if (clipped <= 0) {
        return clipped == 0;
    }

    /* Clear has whole-target placement only; a partial clear cannot be widened to fit its API. */
    if (!view->whole_target || !whole_view(view, &bounds)) {
        return 0;
    }

    openrac_frame_anchor(view->anchor);
    openrac_frame_clear(0, OPENRAC_CLEAR_DEPTH);
    return 1;
}
