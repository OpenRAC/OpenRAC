/*
 * Tier: Claude Opus 5.5 (xhigh effort), task v2021, 2026-11-01.
 * Profile: -O2 -G8 -ffunction-sections. Needs a unit of its own with a -g8.c
 * name; it was compiled as src/levels/06_notak/L06_0033BDC0-spawner-g8.c.
 * All the declarations it needs are below.
 *
 * Best check.py: the form below is EXACT BYTES. 1776/1776 bytes, 1 of 1 USA
 * v1.01 places, 1 of 1 USA v2.00 places; PAL: in neither catalogue.
 * linkcheck level 06: loaded bytes equal. Its strict step fails on
 * func_L00_003A0C00 (D_001A8A44 needs two values), and fails the same way with
 * this function back in assembly.
 *
 * Not landed: SpawnerRegion's `index == -1` test is known false at its only
 * call, because the caller has just tested `index != -1`, and it emits no
 * instruction. WORKER2.md forbids such tests when they only change code shape,
 * so this is reported as FAILED. The condition is a real value and the arms
 * differ, as the vanishing-test idiom asks, but it needs a maintainers' ruling.
 *
 * What the test does: expand_end_loop moves the region from the i < 4 test to
 * the break (the last exit within 30 insns) to the end of the loop.
 * duplicate_loop_exit_test then copies it before the loop, unless it holds a
 * label or a call, or more than 30 insns. Retail has no copy. The helper's label
 * survives jump1, which refuses the copy, and cse1 then folds the test.
 *
 * Without the test: 1868 bytes, 0 of 1 places, with the region duplicated.
 * Three plain inline accessors (entry, rotate, region), with up to four named
 * locals in them, all gave those same 1868 bytes. Their raw insn counts were not
 * measured. By the source, 29..31 raw insns in the region should refuse the copy
 * with no label, so a plain form may still exist.
 *
 * Other choices the bytes need:
 *  - Nine really-used locals declared before `state` put its pseudo number in
 *    94..127, which gives retail's PRE spill order.
 *  - func_L02_0032B468 returns a pointer, as its matched definition does. The
 *    resulting call_value gives sched2 retail's order in case 2.
 *  - D_L06_001A8F20 is MACRO_ADDR, which keeps its lui and addiu adjacent.
 */

#include "common.h"
typedef int VectorBits __attribute__((mode(TI)));
typedef float Vector[4] __attribute__((aligned(16)));
extern void func_L06_003F3690(char *, char *);
extern int func_00282978(void *);
extern int func_002A77E0(int);
extern int func_002A7820(int, int);
extern void func_L04_003429F8(void *, void *, void *, int, void *, int, float);
extern int func_L06_003F3760(void *, void *);
extern char *D_L06_001FE500[];
extern char *D_L06_001B290C __attribute__((sda));
extern int D_L06_001B2348 __attribute__((sda));
extern Vector D_00189EA0;
extern Vector D_L06_001B9840;
extern char D_L06_001A8F20[] MACRO_ADDR;
extern float func_00282E48(void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282FB8(void *, void *, float);
extern float func_00282DF0(void *);
extern void func_00282F20(void *, void *, float);
extern void func_L02_0031E9E8(void *, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_002A7878(float, float);
extern float func_002A89A8(void *, int, float);
extern void func_0029FCC0(void *, int, void *);
extern void func_002ACB48(float *, float);
extern float func_L00_0031CDD0(void *, void *, int, float, float);
extern char *func_L06_003F3EF0(void *, void *, void *, int, int);
extern int func_002E5CF8(int, int, void *);
extern void func_002ABBB8(void *, void *, void *, float, float, int, int, int, float, float, float, float,
                         int, float, int, int, int, int, int);
extern void func_002E5DA0(int, int, void *, int);
extern void *func_L02_0032B468(void *, int, void *, void *, int, int, float, void *, void *, void *);
extern void func_0029F338(void *);
extern int func_002A8600(void *, int, int, int);
extern void func_002A93B0(void *);

typedef struct { char pad[0xA0]; char *children[20]; } SpawnerSlots;

static inline char *SpawnerRegion(int index)
{
    if (index == -1)
        return 0;
    return D_L06_001B290C + (index << 7);
}

/* Select a spawn point, launch a child and retire the spawner when destroyed. */
void func_L06_003F3800(char *moby)
{
    Vector point, delta, origin, step;
    char *region;
    void *anchor;
    int hasAnchor;
    char *path_points;
    int path_count;
    char *path;
    char *target;
    int slot;
    int first;
    char *state = *(char **)(moby + 0x68);

    func_L06_003F3690(moby, state);
    switch (*(unsigned char *)(moby + 0x20)) {
    case 0: {
        char *child;
        if (D_L06_001B2348) {
            child = 0;
            while (!func_002A8600(&child, *(int *)(state + 0x84), 0, 0))
                func_0029F338(child);
            *(unsigned char *)(moby + 0x20) = 1;
        }
        break;
    }
    case 1: {
        func_L06_003F3690(moby, state);
        if (!func_00282978(state + 0x7E))
            break;
        if (!func_002A77E0(7)) {
            anchor = 0;
            hasAnchor = 0;
            path_points = 0;
            path_count = 0;
            if (*(int *)(state + 0x74) != -1) {
                anchor = state + 0x74;
                hasAnchor = 1;
            }
            if (*(int *)(state + 0x78) != -1) {
                path = D_L06_001FE500[*(int *)(state + 0x78)];
                path_points = path + 0x10;
                path_count = *(int *)path;
            }
            func_L04_003429F8(moby, state + 0xF0, anchor, hasAnchor,
                             path_points, path_count,
                             *(float *)(state + 0x70));
        } else {
            target = *(char **)(state + 0x130);
            if (target && *(unsigned char *)(target + 0x20) != 0xFE &&
                *(unsigned char *)(target + 0x20) != 0xFD) {
                qcopy((state + 0xF0), (target + 0x10));
            } else {
                *(int *)(state + 0x130) = 0;
                *(int *)(state + 0x134) = 2;
            }
        }
        if (*(int *)(state + 0x134) == 2)
            break;
        {
        slot = func_L06_003F3760(moby, state);
        if (*(unsigned char *)(state + 0x81) >= *(unsigned char *)(state + 0x80) || slot == -1)
            break;
        region = 0;
        if (*(unsigned char *)(state + 0x82)) {
            int i;
            float nearest = 100000.0f;
            for (i = 0; i < 4; i++) {
                int index = ((int *)(state + 0x90))[i];
                if (index != -1) {
                    char *candidate = D_L06_001B290C + index * 0x80;
                    float current = func_00282E48(D_00189EA0, candidate + 0x30);
                    if (current < nearest) {
                        region = candidate;
                        nearest = current;
                    }
                }
            }
            if (!region) {
                func_00282CF0(point, state + 0xF0, moby + 0x10);
                func_00282D30(point, point, 0.5f);
                if (*(float *)(state + 0x70) > 3.0f)
                    func_00282FB8(point, point, 3.0f);
                if (func_00282DF0(point) < 3.0f)
                    func_00282F20(point, point, 3.0f);
                func_00282CC0(point, point, moby + 0x10);
            } else {
                qcopy(point, region + 0x30);
            }
        } else {
            int i;
            first = func_002A7820(0, 3);
            for (i = 0; i < 4; i++) {
                int k = (i + first) % 4;
                int index = *(int *)(state + (k << 2) + 0x90);
                if (index != -1) {
                    region = SpawnerRegion(index);
                    break;
                }
            }
            if (!region) {
                float radius = *(float *)(state + 0x70);
                if (radius < 3.0f)
                    radius = 3.0f;
                func_L02_0031E9E8(point, radius);
                func_00282CC0(point, point, moby + 0x10);
            } else {
                qcopy(point, region + 0x30);
            }
        }
        if (region) {
            func_00282D30(delta, region, func_002A7878(-1.0f, 1.0f));
            func_00282CC0(point, point, delta);
            func_00282D30(delta, region + 0x10, func_002A7878(-1.0f, 1.0f));
            func_00282CC0(point, point, delta);
        }
        {
            float height = func_002A89A8(point, 0, 0.5f);
            if (height != 0.0f)
                point[2] = height;
        }
        func_00282CF0(delta, point, moby + 0x10);
        {
        float distance = func_00282DF0(delta) * 0.01000000071f;
        func_00282F20(delta, delta, distance);
        func_0029FCC0(moby, 0, origin);
        func_00282F20(step, delta, 0.05f);
        func_002ACB48(step, 0.025f);
        func_00282CC0(origin, origin, step);
        delta[2] = func_L00_0031CDD0(origin, point, 0, distance, -0.002722222358f);
        }
        ((SpawnerSlots *)state)->children[slot] = func_L06_003F3EF0(moby, origin, delta,
                                           *(unsigned char *)(state + 0x83), (unsigned char)slot);
        func_002E5CF8(0, 0, moby);
        *(short *)(state + 0x7E) = *(short *)(state + 0x7C);
        (*(unsigned char *)(state + 0x81))++;
        }
        break;
    }
    case 2:
        func_002ABBB8(moby, state + 0x60, 0, 0.0f, 0.0f, 30, 8, 20, 8.0f, 4.0f, 18.0f, 1.0f, 1, 15.0f, 1, 1, -1, 0,
                      0x810001);
        func_002E5DA0(1, 0, moby, 0x75D);
        func_L02_0032B468(moby, 0x6C5, moby + 0x10, moby + 0xF0, 0, 0, 0.0f,
                          D_L06_001A8F20, D_L06_001A8F20, D_L06_001A8F20);
        func_L02_0032B468(moby, 0x6C6, moby + 0x10, moby + 0xF0, 0, 0, 0.0f,
                          D_L06_001A8F20, D_L06_001A8F20, D_L06_001A8F20);
        func_L02_0032B468(moby, 0x6C7, moby + 0x10, moby + 0xF0, 0, 0, 0.0f,
                          D_L06_001A8F20, D_L06_001A8F20, D_L06_001A8F20);
        func_0029F338(moby);
        return;
    }
    if (*(unsigned char *)(moby + 0x31) && func_00282E48(moby + 0x10, D_L06_001B9840) < 24.0f) {
        func_002A93B0(moby);
        *(unsigned char *)(moby + 0x6F) = 0x16;
    }
}
