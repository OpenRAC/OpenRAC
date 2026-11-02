#include "include_asm.h"
extern char D_L00_001B8E20[];
extern float D_L00_001B14AC __attribute__((sda)), D_L00_001B14B0 __attribute__((sda));
extern float D_L00_001B14B8 __attribute__((sda)), D_L00_001B14BC __attribute__((sda));
extern float D_L00_001B14C0 __attribute__((sda)), D_L00_001B14C4 __attribute__((sda));
extern float func_002702C8(void *,float,float,float,float,float);
extern float func_00282C48(float);
extern void func_002ACEC0(void *,void *,void *,float);
extern float func_00282DB0(void *,void *);
extern float func_00282DF0(void *);
extern void func_00282DD8(void *,void *,void *);
extern void func_00282F20(void *,void *,float);
extern float func_00283248(float,float);
extern float func_00283B98(float,float);
extern float func_00283BE0(float,float);
#define F(p,n) (*(float *)((char *)(p)+(n)))
#define S(p,n) (*(short *)((char *)(p)+(n)))
#define P(p,n) (*(char **)((char *)(p)+(n)))
void func_L00_00425118(char *obj,float forward,float turn)
{
    float axis[4];
    char *base=P(obj,112);
    char *state=base+448;
    char *movement=base+352;
    char *camera=D_L00_001B8E20;
    float factor, angle, length, dot, amount;
    F(state,0)=func_002702C8(state+4,F(state,0),forward,D_L00_001B14AC,D_L00_001B14B0,0.0f);
    F(movement,52)=F(state,20)*F(state,0);
    if(turn<-0.3f) turn=(turn+0.3f)*1.4285714626312256f;
    else if(turn>0.3f) turn=(turn-0.3f)*1.4285714626312256f;
    else turn=0;
    if(turn==0) factor=D_L00_001B14BC;
    else if(func_00282C48(turn)<func_00282C48(F(state,8))) {
        factor = ((turn>0 && F(state,8)>0) || (turn<0 && F(state,8)<0))
            ? D_L00_001B14BC : D_L00_001B14B8;
    } else factor=D_L00_001B14B8;
    {
        float value=func_002702C8(state+12,F(state,8),turn,D_L00_001B14AC,D_L00_001B14B0,factor);
        amount=D_L00_001B14C4*value;
        F(state,8)=value;
    }
    if(forward==0) func_002ACEC0(movement,movement,camera+32,F(state,32));
    else func_002ACEC0(movement,movement,camera+32,F(movement,52));
    dot=func_00282DB0(movement,camera+32);
    length=func_00282DF0(movement);
    func_00282DD8(axis,camera+32,movement);
    func_00282F20(axis,axis,1.0f);
    angle=func_00283248(length,dot);
    if(func_00282C48(angle)<func_00282C48(amount) && factor==D_L00_001B14BC)
        F(state,8)=angle/D_L00_001B14C4;
    if(angle<amount) {
        if(amount<func_00283B98(angle,D_L00_001B14C0))
            F(movement,56)=func_00283BE0(amount,angle);
        else F(movement,56)=D_L00_001B14C0;
    } else if(amount<angle && S(P(obj,112),552)>=0) {
        if(func_00283B98(angle,-D_L00_001B14C0)<amount)
            F(movement,56)=func_00283BE0(amount,angle);
        else F(movement,56)=-D_L00_001B14C0;
    } else F(movement,56)=0;
    if(amount<0) S(P(obj,112),34)=1;
    func_002ACEC0(movement,movement,axis,F(movement,56));
}
