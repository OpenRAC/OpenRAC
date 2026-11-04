#include "include_asm.h"


extern float func_00283BE0(float, float);
extern float func_00283B98(float, float);
extern float func_00282C48(float);

float func_L01_00310BC0(float *speed, float angle, float target,
                      float acceleration, float damping, float limit)
{
    float difference = func_00283BE0(target, angle);
    float drive = difference * 6.366197f;
    if (drive > 1.0f)
        drive = 1.0f;
    else if (drive < -1.0f)
        drive = -1.0f;
    *speed = *speed + (acceleration * drive - damping * *speed);
    if (limit != 0.0f) {
        if (*speed > limit)
            *speed = limit;
        else if (*speed < -limit)
            *speed = -limit;
    }
    if (func_00282C48(difference) < *speed)
        *speed = func_00282C48(difference);
    else if (*speed < -func_00282C48(difference))
        *speed = -func_00282C48(difference);
    return func_00283B98(angle, *speed);
}
