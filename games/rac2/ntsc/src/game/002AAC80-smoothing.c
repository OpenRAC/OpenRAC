#include "include_asm.h"

extern float func_00282C48(float);
extern float func_00282C10(float);
extern void func_002AA2E8(float *, float, float);

void func_002AAC80(float *p, float target, float accel, float brake, float limit)
{
    float stop, total, speed, desired, product;
    if (target == 0.0f && *p == 0.0f) return;
    product = *p * target;
    if (product >= 0.0f && target != 0.0f) {
        speed = *p;
        stop = (speed * speed / brake) * 0.5f;
        if (func_00282C48(target) < stop) {
            total = func_00282C48(target) + func_00282C48(*p);
            if (stop < total) func_002AA2E8(p, 0.0f, brake);
            else func_002AA2E8(p, 0.0f, brake * 1.1f);
        } else {
            speed = brake * 2.0f;
            desired = func_00282C10(speed * target);
            if (limit < desired) desired = limit;
            if (target < 0.0f) func_002AA2E8(p, -desired, accel);
            else func_002AA2E8(p, desired, accel);
        }
        total = func_00282C48(target);
        if (!(func_00282C48(*p) < total)) *p = target;
    } else {
        speed = *p;
        if (speed >= 0.0f) speed -= brake;
        else speed += brake;
        *p = speed;
        if (target < 0.0f && speed < target) { *p = target; return; }
        if (target > 0.0f && *p > target) *p = target;
    }
}

