#include "common.h"
extern void func_00282CF0(void *,void *,void *);
extern float func_00282DF0(void *);
extern void func_002ACF68(void *,void *,void *,int,float);
extern float func_00283CE0(int);
extern void func_00282DD8(void *,void *,void *);
extern float func_00282DB0(void *,void *);
extern float func_002831B0(float);
extern float func_00282C48(float);
extern void func_00282F20(void *,void *,float);
extern float func_002703B0(char *,float,float,float,float,float);
extern float func_00283198(float),func_00283180(float);
extern void func_00282D30(void *,void *,float);
extern void func_002ACDE8(void *,void *,void *);
float func_L00_0041DEF0(char *object,void *target,void *up)
{
    float difference[4],axis[4],fallback[4];
    char *state=*(char **)(object+0x68);
    float distance,ratio,dot,length,angle,step,fraction=0.0f;
    func_00282CF0(difference,target,object+0x10);
    distance=func_00282DF0(difference);
    if(distance==0.0f) goto done;
    if(distance<1.5f) { func_002ACF68(state,state,difference,0,0.5f); goto done; }
    *(short *)(state+0x12)=*(unsigned short *)(state+0x12)+1;
    if(*(short *)(state+0x12)>30) *(short *)(state+0x12)=30;
    ratio=func_00283CE0(*(short *)(state+0x12))/func_00283CE0(30);
    func_00282DD8(axis,difference,state);
    dot=func_00282DB0(state,difference);
    length=func_00282DF0(state)*func_00282DF0(difference);
    if(length==0.0f) goto done;
    angle=1.57079637f-func_002831B0(dot/length);
    if(func_00282C48(angle)>2.879793644f) {
        func_00282DD8(fallback,state,up); func_00282DD8(axis,fallback,state);
    }
    func_00282F20(axis,axis,1.0f);
    step=func_002703B0(state+0x20,0.0f,angle*0.5f,ratio*0.025f,0.3f,0.0f);
    fraction=*(float *)(state+0x20)*57.29577255f;
    if(fraction>1.0f) fraction=1.0f;
    func_00282D30(axis,axis,func_00283198(step)); axis[3]=func_00283180(step);
    func_002ACDE8(state,state,axis);
done:
    return fraction;
}
