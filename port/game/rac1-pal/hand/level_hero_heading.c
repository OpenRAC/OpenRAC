/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Native PAL hero heading/velocity adjustment (00214520), recovered from
 * nonmatching/shared/func_L00_00214520.c and the full 0x2A0 retail body.
 * Not a PS2 match. Scratch w is initialized because the vector helpers
 * copy it; it has no effect on the result, whose w comes from velocity. */
typedef struct { float v[4]; } HeroHeadingVector;
typedef struct {
 char pad0[224];
 float velocity[4];
 char padf0[64];
 float previous[4];
 char pad140[40];
 float height;
 char pad16c[20];
 float angle;
 char pad184[12];
 float speed;
 float heightDelta;
 char pad198[191];
 unsigned char boost;
 char pad258[178];
 short airborne;
 char pad30c[408];
 short modeFlag;
 char pad4a6[1146];
 HeroHeadingVector projection;
 char pad930[5972];
 int state;
 char pad2088[4];
 int movement;
 char pad2090[35];
 unsigned char mode;
} HeroHeading;
extern HeroHeading D_0013F450;
extern float D_0015EE6C MACRO_ADDR;
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001F9CE8(void *);
extern void func_001F9BF0(void *,void *,void *);
extern void func_L00_001FF500(float *,float *,float);
extern void func_001F9BD8(void *,void *,void *);
extern float func_L00_00213A08(HeroHeadingVector *);
extern float func_L00_00234250(float *);
extern float func_00214D28(float *,float,float);
extern void func_L00_00234420(float *,float *,float);
extern void func_L00_00233F88(void *,void *,float);
/* Adjusts hero movement toward heading or damps it according to movement mode. */
void func_L00_00214520(float amount) {
 float v[4] = {0, 0, 0, 0}; HeroHeadingVector copy; float damping;
 int mode=D_0013F450.mode;
 switch(mode) {
 case 0: {
  float angle=D_0013F450.angle;
  float speed=D_0013F450.speed;
  float dot,len,other,limit;
  if(D_0013F450.movement==4) {
   if(D_0013F450.airborne==0 && D_0013F450.modeFlag!=0 && D_0013F450.boost!=0)
    speed=D_0015EE6C*2.1f;
  }
  if(D_0013F450.airborne!=0 && D_0013F450.state==14) {
   if(D_0015EE6C<speed) speed=D_0015EE6C;
  }
  v[0]=func_001F9F90(angle)*speed;
  v[1]=func_001F9FA8(angle)*speed;
  dot=-(D_0013F450.velocity[0]*v[0]+D_0013F450.velocity[1]*v[1]);
  v[2]=D_0013F450.velocity[2];
  len=func_001F9CE8(v);
  other=func_001F9CE8(D_0013F450.velocity);
  if(len==0.0f || other==0.0f) dot=0.0f;
  else {dot/=len;dot/=other;}
  speed=dot+1.0f;
  speed*=1.5f;
  limit=(D_0013F450.state==0x81)?1.7f:3.0f;
  if(limit<speed) speed=limit;
  if(speed<1.0f) speed=1.0f;
  { float *vel=D_0013F450.velocity;
  func_001F9BF0(v,v,vel);
  amount*=speed;
  if(amount<func_001F9CE8(v)) func_L00_001FF500(v,v,amount);
  func_001F9BD8(vel,vel,v);
  for (int i = 0; i < 4; ++i) D_0013F450.previous[i] = vel[i];
  copy=D_0013F450.projection;
  dot=func_L00_00213A08(&copy);
  D_0013F450.heightDelta=D_0013F450.height-dot;
  }
 break;
 }
 case 1:
 case 2: {
  float *vel=D_0013F450.velocity;
  damping=func_L00_00234250(vel);
  func_00214D28(&damping,D_0013F450.speed,amount);
  func_L00_00234420(vel,vel,0.0f);
  func_L00_00233F88(vel,vel,damping);
 break;
 }
 }
}
