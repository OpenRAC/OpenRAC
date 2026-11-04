/* sol-high handoff, task v1171, USA v1.01, default game compiler profile.
 * check.py: 1552 bytes versus 1564, 0 of 2 places (levels 22 and 23).
 * Complete draft; state loads at retail +0x98..+0x134 still differ:
 * retail keeps signed compares in v1, separately loads the halfword in a0,
 * shifts it for the zero test, and rereads the signed state for the final test.
 * This draft CSEs those reads into a0. The case-3 distance branch also retains
 * addiu s0,sp,0x20 where retail has nop. Other normalized operations/registers
 * agree after accounting for the missing state instructions and relocations.
 * Improvements: separate path-record variables by switch case; omit case 0
 * and guard the remaining switch with state != 0; qcopy_nc into the case-1
 * point; native point-to-waypoint copies before direction stores; an explicit
 * count/header/point-array path type for the case-3 end point; spell the object
 * position directly to avoid an extra s0=s6 copy.
 * Seventeen checks were run. Full native copies shortened/merged case tails;
 * state unions, signed bitfields and a short local did not repair the loads.
 * A volatile field adds extra shifts/loads; it is not a retained solution.
 * Switching the first state dispatch preserves more rereads but puts the
 * default block first. An early position local worsens allocation.
 * Next: establish the original narrow state representation/control-flow
 * contract and the point-address lifetime across case 3. No link check run.
 * All required declarations follow. No shared headers or flags were changed.
 */
#include "common.h"

typedef struct Rec_003A2D58 {
    int count;
    unsigned char pad04[0x18];
    float angle;
} Rec_003A2D58;

extern Rec_003A2D58 *D_L22_001FE940[];

typedef int Quad_2160 __attribute__((mode(TI)));
typedef union Path_2160 { Quad_2160 q; float f[4]; int count; } Path_2160;
typedef struct PathData_2160 { int count; char pad04[12]; Path_2160 points[1]; } PathData_2160;
typedef struct State_2160 {
    char pad000[0x190]; int f190;
    char pad194[0x10C]; int paths[44];
    short state, direction, path, point;
    char pad358[8]; char waypoint[16];
    char pad370[8]; short avoiding;
    char pad37A[10]; unsigned short timer;
    char pad386[10]; char avoidance[16];
    char pad3A0[12]; char *target;
    char pad3B0[16]; char previous[16];
    char pad3D0[8]; int target_hit;
    char pad3DC[36]; char flat[16];
    char pad410[8]; int f418;
    char pad41C[32]; float angle;
} State_2160;
extern char D_00189EA0[];
extern void func_002F3AB0(void *, void *, void *);
extern float func_002F3C40(void *, void *);
extern int func_L22_003A2780(void *, void *, void *);
extern int func_L22_003A1ED8(void *);
extern void func_L22_003A2988(void *, float);
extern void func_L22_003A2A28(void *, int);
extern void func_L22_003A2E50(char *, int, int, float);
extern void func_00321298(void *, void *, void *, void *);
extern void func_0032FEB8(void *, void *);
extern float func_0032A758(void *, void *, int);
void func_L22_003A2160(char *obj) {
    char target[16], flat[16], point[16];
    float distances[2];
    State_2160 *st = *(State_2160 **)(obj + 0x68);
    int hit, active, state, next;
    short mode;
    float distance;
    qcopy_nc(target, st->target);
    qcopy_nc(flat, st->target);
    func_L22_003A2988(flat, 0.0f);
    if (st->target == D_00189EA0) hit = st->target_hit;
    else hit = func_L22_003A2780(obj, obj + 0x10, st->target);
    if (!hit && st->state == 0) st->state = 1;
    active = 1;
    mode = st->state;
    if (mode == 2) {
        if (func_L22_003A2780(obj, (obj + 0x10), st->waypoint)) {
            st->avoiding = 0; active = 0;
        }
    } else if (mode == 4) {
        if (hit) { st->avoiding = 0; active = 0; }
    } else if ((st->target == D_00189EA0 && st->target_hit) ||
               (st->state == 0 && hit) || st->state == 3) {
        st->avoiding = 0; active = 0;
    }
    if (active) {
        if (st->avoiding && st->f190 < 5) {
            if (func_002F3C40((obj + 0x10), st->avoidance) < 10.0f) st->avoiding = 0;
            qcopy(target, st->avoidance);
        } else {
            if (func_L22_003A1ED8(obj)) {
                qcopy(target, st->avoidance);
                qcopy(st->previous, (obj + 0x10));
            }
            st->timer += 1;
        }
    }
    state = st->state;
    if (state != 0) switch (state) {
    case 1: {
        Path_2160 *rec;
        func_L22_003A2A28(obj, 1);
        rec = (Path_2160 *)D_L22_001FE940[st->paths[st->path]];
        qcopy_nc(point, rec + (st->point + 1));
        st->state = 2;
        distance = func_002F3C40(rec + (st->point + 1), st->target);
        if (st->point == 0) {
            if (func_002F3C40(rec + 2, st->target) < distance) {
                st->direction = state;
                func_00321298(st->waypoint, point, rec + (st->point + 2), (obj + 0x10));
            } else {
                *(Quad_2160 *)st->waypoint = *(Quad_2160 *)point;
                st->direction = -1;
            }
        } else if (st->point == rec->count - 1) {
            if (func_002F3C40(rec + (rec->count - 1), st->target) < distance) {
                st->direction = -1;
                func_00321298(st->waypoint, point, rec + st->point, (obj + 0x10));
            } else {
                *(Quad_2160 *)st->waypoint = *(Quad_2160 *)point;
                st->direction = state;
            }
        } else {
            distances[0] = func_002F3C40(rec + st->point, st->target);
            distances[1] = func_002F3C40(rec + (st->point + 2), st->target);
            if (distances[0] < distances[1]) st->direction = -1;
            else st->direction = state;
            func_00321298(st->waypoint, point, rec + (st->direction + 1), (obj + 0x10));
        }
        func_L22_003A2988(st->waypoint, 0.0f);
        break;
    }
    case 2:
        if (func_002F3C40((obj + 0x10), st->waypoint) < 2.0f) {
            st->state = 3;
            goto finish;
        }
        qcopy(target, st->waypoint);
        break;
    case 3: {
        if (func_002F3C40(st->flat, flat) < 20.0f && (st->f418 || hit)) {
            st->state = 4;
        } else if (func_002F3C40((obj + 0x10), st->waypoint) < 2.0f) {
            next = st->point + st->direction;
            if (next < 0) {
                func_L22_003A2E50(obj, 1, 0, ((PathData_2160 *)D_L22_001FE940[st->paths[st->path]])->points[0].f[3]);
            } else {
                PathData_2160 *end = (PathData_2160 *)D_L22_001FE940[st->paths[st->path]];
                if (next >= end->count) func_L22_003A2E50(obj, 1, 0, end->points[end->count - 1].f[3]);
                else st->point = next;
            }
            qcopy(st->waypoint, (Path_2160 *)D_L22_001FE940[st->paths[st->path]] + (st->point + 1));
            func_L22_003A2988(st->waypoint, 0.0f);
            qcopy(target, st->waypoint);
        }
        break;
    }
    case 4:
        st->state = 0;
        break;
    }
finish:
    func_0032FEB8(obj, target);
    func_002F3AB0(point, target, (obj + 0x10));
    st->angle = func_0032A758(obj, point, 0);
}
