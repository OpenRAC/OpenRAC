#include "common.h"
typedef struct {float v[4];} Vec;
typedef struct {Vec basis[3]; Vec pos;} Camera;
typedef struct {float yaw,pitch,dist; int frames_left; float inv_frames; int frames; char pad[8]; Vec fwd,up,rot,pos,saved_rot;} CamBlendOrbit;
extern unsigned char D_001B5490[];
extern char D_00189E20[];
extern Vec D_00189EA0[], D_001B5340[], D_001B5570[], D_0018A0E0[];
extern float func_002A7C00(float,float,float);
extern void func_00282DD8(void*,void*,void*);
extern void func_00270B58(float*,void*,void*,void*,void*,void*);
extern void func_00282F20(void*,void*,float);
extern float func_00283BE0(float,float), func_00283B98(float,float);
extern void func_002ACEC0(void*,void*,void*,float);
extern void func_00282D30(void*,void*,float), func_00282CF0(void*,void*,void*);
extern float func_00282DB0(void*,void*), func_00282DF0(void*), func_002831B0(float), func_00282C48(float);
extern void func_002AB668(void*,void*,float), func_002ACDE8(void*,void*,void*);
extern int func_00282948(void*);
extern void func_00283958(void*,void*), func_002AB600(void*,void*), func_00282CC0(void*,void*,void*);
/* Adapted from RAC1 advance_alternate_camera_transition, GPL-3.0. */
int func_002712D8(Camera *cam, CamBlendOrbit *o) {
    Vec ang;
    Vec fwd;
    Vec side;
    Vec up;
    Vec target;
    Vec pos;
    Vec diff;
    Vec proj;
    Vec m[3];
    Vec r0;
    Vec r1;
    Vec r2;
    Vec q;
    float step;
    float dyaw;
    float dpitch;
    float yaw;
    float turn;
    float sign;
    float s2;
    float d;
    char *g;

    if (o->frames_left <= 0) {
        return 1;
    }
    step = 1.0f / func_002A7C00(1.0f, (float)o->frames, (float)o->frames_left * o->inv_frames);
    if (D_001B5490[2] == 2) {
        qcopy(&target, D_00189EA0);
        qcopy(&fwd, &o->fwd);
        qcopy(&up, &o->up);
        func_00282DD8(&side, &fwd, &up);
        func_00270B58(ang.v, &cam->pos, &target, &fwd, &side, &up);
    } else {
        qcopy(&target, &cam->pos);
        g = D_00189E20;
        func_00282F20(&fwd, *(char **)(g + 0x2290) + 0xC0, 1.0f);
        func_00282F20(&up, *(char **)(g + 0x2290) + 0xE0, 1.0f);
        ang.v[2] = 0.0f;
        ang.v[1] = 0.0f;
        ang.v[0] = 3.1415927f;
    }
    dyaw = func_00283BE0(ang.v[0], o->yaw);
    o->yaw = func_00283B98(o->yaw, dyaw * step);
    dpitch = func_00283BE0(ang.v[1], o->pitch);
    o->pitch = func_00283B98(o->pitch, dpitch * step);
    o->dist = o->dist + (ang.v[2] - o->dist) * step;
    func_00282F20(&pos, &fwd, o->dist);
    func_002ACEC0(&pos, &pos, &up, o->yaw);
    func_00282DD8(&side, &pos, &up);
    func_00282F20(&side, &side, 1.0f);
    func_002ACEC0(&pos, &pos, &side, o->pitch);
    func_00282CC0(&o->pos, &target, &pos);
    qcopy(D_001B5340, &o->pos);
    func_00283958(&o->rot, m);
    d = func_00282DB0(&m[2], cam);
    func_00282D30(&proj, &m[2], d);
    func_00282CF0(&diff, cam, &proj);
    yaw = 1.57079637f - func_002831B0(func_00282DB0(&m[0], &diff) / func_00282DF0(&diff));
    sign = -1.0f;
    if (func_00282DB0(&diff, &m[1]) >= 0.0f) {
        sign = 1.0f;
    }
    yaw = yaw * sign;
    if (func_00282C48(dyaw) > 1.5707965f
        && ((dyaw >= 0.0f && sign < 0.0f) || (dyaw < 0.0f && sign >= 0.0f))) {
        if (yaw < 0.0f) {
            yaw += 6.283186f;
        } else {
            yaw -= 6.283186f;
        }
    }
    turn = yaw * step;
    if (func_00282C48(turn) < 1e-5f) {
        qcopy(&r0, &m[0]);
        qcopy(&r1, &m[1]);
    } else {
        func_002AB668(&r2, &m[2], turn);
        func_002ACDE8(&r0, &m[0], &r2);
        func_002ACDE8(&r1, &m[1], &r2);
    }
    if (func_00282C48(yaw) < 1e-5f) {
        qcopy(&r2, &m[0]);
    } else {
        func_002AB668(&q, &m[2], yaw);
        func_002ACDE8(&r2, &m[0], &q);
    }
    yaw = 1.57079637f - func_002831B0(func_00282DB0(&r2, cam));
    d = func_00282DB0(&r2, &cam->basis[2]);
    s2 = -1.0f;
    if (d >= 0.0f) {
        s2 = 1.0f;
    }
    yaw *= s2;
    func_002ACEC0(&r0, &r0, &r1, yaw * step);
    func_00282F20(&D_001B5570[0], &r0, 1.0f);
    func_00282DD8(&D_001B5570[1], &D_001B5570[0], D_0018A0E0);
    func_00282F20(&D_001B5570[1], &D_001B5570[1], -1.0f);
    func_00282DD8(&D_001B5570[2], &D_001B5570[1], &D_001B5570[0]);
    func_002AB600(&o->saved_rot, D_001B5570);
    func_002AB600(&o->rot, D_001B5570);
    func_00282948(&o->frames_left);
    return 0;
}
