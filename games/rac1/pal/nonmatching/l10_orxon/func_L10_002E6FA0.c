/* NON_MATCHING func_L10_002E6FA0 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: SIZE ours 736 / retail 732, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a03: emits randomized facing-direction particles. Best p7.c BYTES52/732: prologue/loop-entry setup (+4.
 *   Explicit source position, guarded loop, shared scale call after selecting bounds, and actual pointer-return sp
 */
#include "common.h"
extern int func_002140B0(int);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_002140F8(float, float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9C30(void *, void *, float);
extern int func_001FA8A8(int, int, float);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00274788(void *, void *, int, float, float, float, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L10_00161F84 SDATA(D_L10_00161F84);
extern float D_L10_00161F7C SDATA(D_L10_00161F7C);
extern float D_L10_00161F78 SDATA(D_L10_00161F78);
extern int D_L10_00161F70 SDATA(D_L10_00161F70);
extern int D_L10_00161F74 SDATA(D_L10_00161F74);
extern float D_L10_00161F80 SDATA(D_L10_00161F80);
/* Emits randomized particles in the object's facing direction. */
void func_L10_002E6FA0(char *m) {
 float pos[4],vel[4],off[4];
 char *d=*(char **)(m+0x78);
 char *sourcepos;
 int i;
 ((unsigned char *)m)[0x30]=0x80;
 if(*(int *)(d+0xC)>0) {
 sourcepos=m+0x10;
 for(i=0;i<*(int *)(d+0xC);i++) {
  int small=func_002140B0(100)<D_L10_00161F84;
  float a,b,c,scale;
  int col,fade,life;
  qcopy(pos,sourcepos);
  off[0]=func_001F9F90(func_001FA748(*(float *)(m+0x48),1.5707964f))*func_002140F8(-*(float *)(d+0x10),*(float *)(d+0x10));
  off[1]=func_001F9FA8(func_001FA748(*(float *)(m+0x48),1.5707964f))*func_002140F8(-*(float *)(d+0x10),*(float *)(d+0x10));
  off[2]=0;
  off[2]=func_002140F8(-*(float *)(d+0x14),*(float *)(d+0x14));
  func_001F9BD8(pos,pos,off);
  a=func_002140F8(D_L10_00161F7C,D_L10_00161F78);
  b=func_002140F8(D_L10_00161F7C,D_L10_00161F78);
  c=func_002140F8(D_L10_00161F7C,D_L10_00161F78);
  if(a<b) a=b;
  if(a<c) a=c;
  func_00215C00(vel,a*D_0015EE6C,*(float *)(m+0x48),-*(float *)(m+0x44));
  if(small) {
   func_001F9C30(vel,vel,func_002140F8(0.5f,1.5f));
   scale=func_002140F8(0.24f,0.36f);
  } else scale=func_002140F8(0.8f,1.2f);
  scale=*(float *)(d+4)*scale;
  col=func_001FA8A8(D_L10_00161F70,D_L10_00161F74,func_002140F8(0.5f,1.0f));
  fade=func_001FA8A8(D_L10_00161F74,D_L10_00161F74 & 0xFF000000,func_002140F8(0.25f,0.5f));
  life=func_001FA898_r(func_001F9878(*(float *)(d+8)*60.0f));
  func_L00_00274788(pos,vel,life,scale,*(float *)d,D_L10_00161F80*D_0015EE70,col,fade,small);
 }
 }
}
