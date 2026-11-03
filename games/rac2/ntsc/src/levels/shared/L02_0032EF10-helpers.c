#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { TI128 q; float f[4]; } MoveVector;
extern int func_002C94C8(int, void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002C9218(int, void *, void *, float);

void func_L02_0032EF10(MoveVector *start, MoveVector *target, MoveVector *output, int path, float radius) {
    MoveVector source, intersection, destination;
    MoveVector *hit = &intersection;
    source.q = start->q;
    if (func_002C94C8(path, &source, target, hit)) {
        func_00282CF0(hit, hit, &source);
        func_00282D30(hit, hit, 0.9900000095367432f);
        func_00282CC0(&destination, &intersection, &source);
    } else {
        qcopy(&destination, target);
    }
    if (!func_002C9218(path, &destination, output, radius))
        qcopy(output, &destination);
    output->f[2] = target->f[2];
}
