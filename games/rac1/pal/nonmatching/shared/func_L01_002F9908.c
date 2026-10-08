/* NON_MATCHING func_L01_002F9908 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 28/484 (94.2% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a debris moby with randomized rotation and spin, scaling and lifetime.
 *   p7.c is 28/484 bytes different: stores at 0x154–0x160 and gravity setup/store scheduling and f0/f1 allocation 
 *   Stopped at budget 8; store-order and gravity expression wording could unblock the remaining scheduling/allocat
 */
extern char *func_0020D348(int);
extern void func_L00_0025E210(void *);
extern float func_002140F8(float, float);
extern int func_001160D8(void);
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00251E30(void *);
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
typedef int V128 __attribute__((mode(TI)));
// Spawns a debris moby with random rotation, angular speed, and lifetime.
char *func_L01_002F9908(void *position, void *velocity, int cls, int mode, int flag, float scale, float gravity, float spin, float life) {
 float pos[4], vel[4]; float *vp=vel,*pp=pos; char *m,*d; float speed,angular;
 *(V128 *)pp=*(V128 *)position; *(V128 *)vp=*(V128 *)velocity;
 m=func_0020D348(cls);
 if (m) {
  d=*(char **)(m+0x78); func_L00_0025E210(m);
  *(float *)(m+0x2c) *= scale;
  ((unsigned char *)m)[0x30]=255; *(short *)(m+0x32)=255; m[0x31]=1;
  *(float *)(m+0x40)=func_002140F8(-3.1415927f,3.1415927f);
  *(float *)(m+0x44)=func_002140F8(-3.1415927f,3.1415927f);
  *(float *)(m+0x48)=func_002140F8(-3.1415927f,3.1415927f);
  qcopy(m+0x10,pp); qcopy(d,vp);
  speed=func_002140F8(D_0015EE6C*1.5707964f,D_0015EE6C*6.2831855f);
  if (func_001160D8() & 1) angular=-speed*spin;
  else angular=speed*spin;
  *(int *)(d+0x28)=mode; *(float *)(d+0x14)=scale; *(int *)(d+0x10)=mode; *(float *)(d+0x18)=angular;
  *(int *)(d+0x1c)=func_001FA898_r(func_001F9878(life*60.0f));
  *(int *)(d+0x20)=flag; *(float *)(d+0x24)=gravity*9.8f*D_0015EE70;
  func_L00_00251E30(m);
 }
 return m;
}
