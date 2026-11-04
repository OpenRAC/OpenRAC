#include "common.h"

extern char *D_001F1700[];
extern void func_00282CF0(float *, float *, float *);
extern void func_00282CC0(float *, float *, float *);
extern void func_00282F20(float *, float *, float);
extern void func_00282DD8(float *, float *, float *);
extern float func_00282C48(float);
extern float func_00282DF0(float *);
extern float func_00282DB0(float *, float *);
typedef struct {
    float x, y, z, active;
} PathPoint_2C9218;
typedef struct {
    int count;
    char pad04[12];
    PathPoint_2C9218 points[1];
} Path_2C9218;

int func_002C9218(int index, float *point, float *out, float radius)
{
    float delta[4], adjustment[4], position[4], edge[4], unit[4], cross[4], projection[4];
    int found = 0, i, offset, next;
    float length, dot;
    qcopy_nc(position, point);
    for (i = 0; i < *(int *)D_001F1700[index] - 1; i++) {
        offset = i * 16 + 16;
        next = i * 16 + 32;
        if (((Path_2C9218 *)D_001F1700[index])->points[i].active == 0) continue;
        func_00282CF0(delta, position, (float *)(D_001F1700[index] + offset));
        delta[2] = 0;
        func_00282CF0(edge, (float *)(D_001F1700[index] + next), (float *)(D_001F1700[index] + offset));
        edge[2] = 0;
        func_00282F20(unit, edge, 1.0f);
        func_00282DD8(cross, delta, unit);
        if (radius < func_00282C48(cross[2])) continue;
        length = func_00282DF0(edge);
        dot = func_00282DB0(delta, unit);
        if (length < dot || dot < 0) {
            if (func_00282DF0(delta) < radius) {
                func_00282F20(adjustment, delta, radius);
                found = 1;
                func_00282CC0(position, adjustment, (float *)(D_001F1700[index] + offset));
                position[2] = point[2];
            }
        } else {
            func_00282F20(projection, unit, dot);
            found = 1;
            func_00282CF0(adjustment, delta, projection);
            func_00282F20(adjustment, adjustment, radius);
            func_00282CC0(adjustment, adjustment, projection);
            func_00282CC0(position, adjustment, (float *)(D_001F1700[index] + offset));
            position[2] = point[2];
        }
    }
    if (found) qcopy(out, position);
    return found;
}
