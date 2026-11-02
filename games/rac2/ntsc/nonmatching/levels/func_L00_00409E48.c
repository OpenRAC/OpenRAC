/* Tier: sol-high, task v1197 (29 October 2026).
 * Retained structured draft: 1648/1628 bytes, 0/27 USA v1.01 places.
 * Profile: -O2 -G8 -ffunction-sections; all declarations are below.
 * The earlier raw-pointer draft recompiles to 1628/1628 bytes, 0/27 places,
 * first instruction difference +0x18. This draft recovers the complete prologue,
 * m in s2, state in s4, initial stores/counter branch, resident player base,
 * unsigned hit byte, target-relative scalar loads and displaced final TI copy.
 * Remaining: modulo/tuning-load schedule, camera/position and orientation-pointer
 * allocation, repeated 1.0f materialization instead of shared f20, unmerged special
 * target cross-product call, final forward-pointer move and byte/word store order.
 * Moving the special cross product out of its target branches gives 1616 bytes;
 * retaining its up pointer gives 1648. An explicit camera-up pointer, independent
 * non-clobbering first-three matrix copies and -frac2-gcse-unsplit-edges each tie
 * the retained 1648-byte output. -fno-gcse gives 1624 with the wrong 0xe0 frame.
 * All-TI copies under -frac2-qcopy -frac2-qcopy-barrier give 1624 bytes with worse
 * saved GPRs. No volatile casts, pinned registers or source barriers were used.
 * Next useful experiment: recover the original orientation pointer/normalization
 * constant lifetimes. Do not repeat the tied forms. Symbol bindings remain
 * unverified because this is not an instruction match; no link check was run.
 */
#include "common.h"
typedef int Quadword __attribute__((mode(TI)));
typedef float Vector[4] __attribute__((aligned(16)));
extern char *func_0029EFD0(int);
extern void func_00282D30(void *,void *,float);
extern void func_00282CC0(void *,void *,void *);
extern void func_00282CF0(void *,void *,void *);
extern void func_00282F20(void *,void *,float);
extern void func_00282DD8(void *,void *,void *);
extern int func_002761F8(void *,void *,int,void *,int);
extern float func_002A7878(float,float),func_002A78D8(float,float),func_002A7940(void);
extern char *func_002AB1F0(void *);
extern void func_L00_0040B2C0(void *,void *);
extern void func_002A0F48(void *);
extern void func_002ACA50(void *,void *,int);
extern int func_002FED90(void *,int,float,float,float);
extern int D_L00_001B0FD8 __attribute__((sda));
extern unsigned char D_00139583;
typedef struct { char pad[0x50]; float value; char tail[0x8c]; } ScaleRecord;
extern ScaleRecord D_L00_00262570[];
typedef struct ProjectileState {
    float last_target[3]; void *owner; void *target;
    float height, speed, yaw, spin_speed;
    short life; unsigned char homing, age;
    float spin; int index; Vector forward, color; int sound, handle;
} ProjectileState;
typedef struct ProjectileObject {
    char pad00[0x10]; Vector position; signed char status;
    char pad21[0x0b]; float radius;
    unsigned char alpha; signed char enabled; short color; unsigned short flags;
    char pad36[0x32]; ProjectileState *state;
    char pad6c[0x50]; unsigned char hit; char padbd[3];
    Vector forward, right, up;
} ProjectileObject;
typedef struct PlayerState {
    char pad0000[0x750]; Vector up, forward;
    char pad0770[0x1b20]; char *object; int mode;
    char pad2298[0x254]; short orientation;
} PlayerState;
extern PlayerState D_00189E20;
extern float D_L00_001B8FF0[12],D_L00_001B8DC0[4],D_L00_001BF220[4];
extern char D_L00_00255BA0[],D_L00_001B1FB0[];
char *func_L00_00409E48(char *owner,void *position,char *target,void *color,float height)
{
    Vector aim,offset,tmp1,tmp2,tmp3,tmp4;
    ProjectileObject *m=(ProjectileObject *)func_0029EFD0(0xed7);
    ProjectileState *s;
    char *hit;
    float scale;
    void *forward;
    if(m) {
        m->alpha=255;m->enabled=1;m->status=0;
        m->color=255;m->flags|=0x100;
        s=m->state;
        s->sound=-1;
        if((unsigned)D_L00_001B0FD8>32)D_L00_001B0FD8=0;
        s->index=D_L00_001B0FD8;
        D_L00_001B0FD8=(D_L00_001B0FD8+1)%32;
        scale=D_L00_00262570[D_00139583].value*(1.0f/60.0f);
        qcopy(m->position,position);
        if(target) {
            func_00282D30(offset,target+0xe0,height);
            func_00282CC0(aim,target+0x10,offset);
        } else qzero(aim);
        if(D_00189E20.mode==29) {
            float *camera_forward=D_L00_001B8FF0;
            float *camera_right;
            float *shot_right;
            float *shot_up;
            qcopy(m->forward,camera_forward);
            camera_right=camera_forward+4;
            shot_right=m->right;
            qcopy(shot_right,camera_right);
            shot_up=m->up;
            qcopy(shot_up,camera_forward+8);
            qcopy(m->position,D_L00_001B8DC0);
            func_00282D30(offset,m->forward,-0.2f);
            func_00282CC0(m->position,m->position,offset);
            func_00282D30(tmp1,camera_right,-0.25f);
            func_00282CC0(m->position,m->position,tmp1);
            func_00282D30(tmp2,camera_forward,0.0f);
            func_00282CC0(m->position,m->position,tmp2);
            if(target) {
                func_00282CF0(m->forward,aim,m->position);
                func_00282F20(m->forward,m->forward,1.0f);
                func_00282DD8(shot_right,m->forward,D_00189E20.object+0xe0);
            } else {
                func_00282D30(tmp3,camera_forward,30.0f);
                func_00282CC0(tmp3,D_L00_001B8DC0,tmp3);
                if(func_002761F8(D_L00_001B8DC0,tmp3,0,D_00189E20.object,0))qcopy(tmp3,D_L00_001BF220);
                func_00282CF0(tmp4,tmp3,m->position);
                func_00282F20(m->forward,tmp4,1.0f);
                func_00282DD8(shot_right,m->forward,D_00189E20.object+0xe0);
            }
            func_00282F20(shot_right,shot_right,1.0f);
            func_00282DD8(shot_up,shot_right,m->forward);
        } else if(D_00189E20.orientation==1) {
            func_00282F20(m->forward,owner+0xd0,-1.0f);
            func_00282D30(m->right,owner+0xc0,-1.0f);
            qcopy(m->up,owner+0xe0);
        } else {
            if(target) {
                float *shot_forward=m->forward;
                float *shot_right;
                func_00282CF0(shot_forward,aim,m->position);
                shot_right=m->right;
                func_00282F20(shot_forward,shot_forward,1.0f);
                func_00282DD8(shot_right,shot_forward,D_00189E20.object+0xe0);
                func_00282F20(shot_right,shot_right,1.0f);
                func_00282DD8(m->up,shot_right,shot_forward);
            } else {
                float *shot_forward=m->forward;
                float *shot_right;
                qcopy(shot_forward,D_00189E20.forward);
                shot_right=m->right;
                func_00282DD8(shot_right,shot_forward,D_00189E20.up);
                func_00282F20(shot_right,shot_right,1.0f);
                func_00282DD8(m->up,shot_right,shot_forward);
            }
        }
        forward=m->forward;
        m->forward[3]=m->right[3]=m->up[3]=0;
        s->owner=owner;
        s->yaw=func_002A7878(4.0f,10.0f);
        if(target)s->yaw=0;
        s->spin_speed=func_002A78D8(6.0f,10.0f);
        s->speed=scale;
        s->spin=func_002A7940();
        s->homing=1;s->target=0;s->age=1;
        qcopy(s->forward,forward);qcopy(s->color,color);
        if(target) {
            hit=func_002AB1F0(target);
            if(hit) {
                *(unsigned short *)(hit+0x1e)|=0x80;
                if(*(unsigned char *)(hit+0xb))s->homing=0;
            }
        }
        s->life=300;
        if(target) {
            float *target_position=(float *)(target+0x10);
            s->last_target[0]=*(float *)(target+0x10);
            s->last_target[1]=target_position[1];
            s->last_target[2]=target_position[2]+height;
            s->height=height;
        }
        *(Quadword *)offset=*(Quadword *)(owner+0x10);offset[2]=((float *)position)[2];
        if(func_002761F8(offset,position,0,D_00189E20.object,0)) {
            m->hit=1;qcopy(m->position,D_L00_001BF220);
        }
        func_L00_0040B2C0(m,D_L00_00255BA0+s->index*64);
        func_002A0F48(m);
        func_002ACA50(m,D_L00_001B1FB0,31);
        s->handle=func_002FED90(m,0,1.0f,1.0f,1.0f);
        m->radius*=2.5f;
    }
    return (char *)m;
}
