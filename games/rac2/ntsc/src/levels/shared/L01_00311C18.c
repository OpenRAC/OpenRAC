#include "include_asm.h"
typedef int Copy128 __attribute__((mode(TI)));
typedef union { Copy128 q; float f[4]; } Vector16;

extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern float func_00282C48(float);

float func_L01_00311C18(Vector16 *out, Vector16 *start,
                      Vector16 *end, Vector16 *point)
{
    Vector16 edge, relative, other, perpendicular;
    float determinant, t, fraction;
    func_00282CF0(&edge, end, start);
    func_00282CF0(&other, start, point);
    /* This planar construction defines x/y only; retail leaves the other lanes. */
    perpendicular.f[0] = edge.f[1];
    perpendicular.f[1] = -edge.f[0];
    func_00282CC0(&perpendicular, point, &perpendicular);
    func_00282CF0(&relative, point, &perpendicular);
    determinant = edge.f[1] * relative.f[0] - edge.f[0] * relative.f[1];
    if (func_00282C48(determinant) < 0.0001f) {
        out->q = start->q;
        return 0.0f;
    }
    determinant = 1.0f / determinant;
    t = (relative.f[1] * other.f[0] - relative.f[0] * other.f[1]) * determinant;
    fraction = t < 0.0f ? 0.0f : t;
    fraction = fraction > 1.0f ? 1.0f : fraction;
    func_00282D30(out, &edge, fraction);
    func_00282CC0(out, out, start);
    return t;
}
