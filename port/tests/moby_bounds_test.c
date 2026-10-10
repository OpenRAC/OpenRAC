/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/game_moby_bounds.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)
#if __SIZEOF_POINTER__ == 4
_Static_assert(offsetof(BoundsModel, sequences) == 0x48, "EE sequences");
_Static_assert(offsetof(BoundsMoby, model) == 0x24, "EE model");
_Static_assert(offsetof(BoundsMoby, scale) == 0x2C, "EE scale");
_Static_assert(offsetof(BoundsMoby, flags) == 0x34, "EE flags");
_Static_assert(offsetof(BoundsMoby, sequence) == 0x52, "EE sequence");
_Static_assert(offsetof(BoundsMoby, blend) == 0x54, "EE blend");
_Static_assert(offsetof(BoundsMoby, cached_sequence) == 0x71, "EE cache");
_Static_assert(offsetof(BoundsMoby, in_grid) == 0x94, "EE grid membership");
_Static_assert(offsetof(BoundsMoby, grid) == 0xA0, "EE packed grid");
_Static_assert(offsetof(BoundsMoby, revision) == 0xA8, "EE revision");
_Static_assert(offsetof(BoundsMoby, basis) == 0xC0, "EE basis");
_Static_assert(offsetof(BoundsMoby, cached_sphere) == 0xF0, "EE sphere cache");
#endif
float bounds_snapshots[8][4];
static BoundsMoby m;
static BoundsModel model;
static float first[4] = {1, 2, 3, 4}, second[4] = {5, 10, 15, 20};
static int calls;
static unsigned int cells;
void bounds_grid(void* p, int packed) { CHECK(p == &m); ++calls; cells = (unsigned int)packed; }
static void reset(void) {
    memset(&m, 0, sizeof(m)); memset(&model, 0, sizeof(model));
    calls = 0; cells = 0;
    m.model = &model; model.sequences[0] = first; model.sequences[1] = second;
    m.scale = 2; m.cached_sequence = 255;
    m.basis[0][0] = m.basis[1][1] = m.basis[2][2] = 1;
    m.basis[1][3] = 77;
    m.position[0] = 16; m.position[1] = 32; m.position[2] = 48;
}
int main(void) {
    BoundsMoby saved;
    reset(); m.state = -1; saved = m; func_0020EEE8(&m); CHECK(memcmp(&m, &saved, sizeof(m)) == 0);
    reset(); func_0020EEE8(&m);
    CHECK(m.sphere[0] == 16386 && m.sphere[1] == 32772 && m.sphere[2] == 49158 && m.sphere[3] == 8);
    CHECK(m.cached_sequence == 0 && m.cached_sphere[2] == 3 && m.revision == 1 && calls == 0);
    m.model = NULL; m.flags = 0x8000; func_0020EEE8(&m);
    CHECK(m.basis[1][1] == -1 && m.basis[1][3] == 77 && m.sphere[1] == 32764 && m.revision == 2);
    reset(); m.sequence = 1; m.previous = 0; m.blend = 0.25f; func_0020EEE8(&m);
    CHECK(m.sphere[0] == 16392 && m.sphere[1] == 32784 && m.sphere[2] == 49176 && m.sphere[3] == 32);
    CHECK(m.cached_sequence == 255 && m.cached_sphere[0] == 0);
    reset(); memcpy(bounds_snapshots[3], second, sizeof(second));
    m.sequence = 255; m.snapshot = 3; m.blend = 0.5f; func_0020EEE8(&m);
    CHECK(m.sphere[0] == 16390 && m.sphere[3] == 24 && m.cached_sequence == 255);
    reset(); m.in_grid = 1; m.grid = 0xDEADBEEF; m.revision = 65535; func_0020EEE8(&m);
    CHECK(calls == 1 && cells == 0x02010100u && m.revision == 0);
    m.grid = cells; func_0020EEE8(&m); CHECK(calls == 1);
    m.grid = 0xDEADBEEF; m.position[0] = -16; func_0020EEE8(&m); CHECK(calls == 1);
    reset(); m.in_grid = 1; m.position[0] = 1024; func_0020EEE8(&m); CHECK(calls == 1);
    m.position[0] = 1040; func_0020EEE8(&m); CHECK(calls == 1);
    /* Upper-cell bytes are not clamped by retail's lower-cell-only mask. */
    reset(); m.in_grid = 1; m.scale = 1; m.cached_sequence = 0;
    m.cached_sphere[3] = 1050000; m.position[0] = m.position[1] = 1050000.0f / 1024;
    func_0020EEE8(&m); CHECK(calls == 1 && (cells & 0x80000000u));
    m.grid = cells; func_0020EEE8(&m); CHECK(calls == 2);
    CHECK(bounds_integer(2147483648.0f) == 2147483647);
    CHECK(bounds_integer(-2147483648.0f) == (-2147483647 - 1));
    CHECK(bounds_integer(-3.9f) == -3);
    puts("moby bounds: OK"); return 0;
}
