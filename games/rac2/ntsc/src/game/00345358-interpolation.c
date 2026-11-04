#include "include_asm.h"

/* The retail clamps use min.s/max.s, requiring this unit's -ffast-math.
 * The measured key-array layout preserves indexed member addressing. */
typedef unsigned char u8;
typedef struct {char pad[0x70];float keys[4];} KeyObject;
extern void func_00349630(u8 *,int);
extern float func_0034EE78(float, float, float, float, float);
void func_00349680(char *o, int advance)
{
 int hi, lo, i;
 float t, *keys;
 if (!*(int *)(o+0x28)) return;
 lo=0;hi=0;
 if (*(int *)(o+0x1c)==1) {
  float value=*(float *)(o+0x18);
  if (advance) value+=*(float *)(o+0x24);
  *(float *)(o+0x18)=value;
  *(int *)(o+0x28)=value<=1.0f;
  value=value<=1.0f?value:1.0f;
  *(float *)(o+0x18)=value;
  for(i=0,keys=(float *)(o+0x70);i<*(int *)(o+0x84);i++) {
   int next=hi+1;
   if(next<*(int *)(o+0x84) && keys[hi]<=value) {lo=hi;hi=next;} else break;
  }
 } else {
  float value=*(float *)(o+0x18);
  if(advance) value-=*(float *)(o+0x24);
  *(float *)(o+0x18)=value;
  *(int *)(o+0x28)=value>=0.0f;
  value=value>=0.0f?value:0.0f;
  *(float *)(o+0x18)=value;
  keys=(float *)(o+0x70);
  hi=*(int *)(o+0x84)-1;lo=hi;
  { int j; for(j=hi;j>0;j--) {
   int next=lo-1;
   if(next>=0 && value<=((KeyObject *)o)->keys[lo]) {hi=lo;lo=next;} else break;
  }}
 }
 t=func_0034EE78((*(float *)(o+0x18)-((KeyObject *)o)->keys[lo])/(((KeyObject *)o)->keys[hi]-((KeyObject *)o)->keys[lo]),0.0f,*(float *)o,*(float *)(o+12),1.0f);
 if(*(int *)(o+0x2c)) {
  char *target=*(char **)(o+0x80);char *vt=*(char **)target;
  ((void (*)(void *,int,void *,void *,float))*(void **)(vt+20))(target+*(short *)(vt+16),*(int *)(o+0x2c),o+(lo*16+0x30),o+(hi*16+0x30),t);
 }
 if(!*(int *)(o+0x28)) {
  switch(*(int *)(o+0x20)) {
   case 0:break;
   case 1:func_00349630((u8 *)o,*(int *)(o+0x1c));break;
   case 2:*(int *)(o+0x1c)=*(int *)(o+0x1c)==1?-1:1;func_00349630((u8 *)o,*(int *)(o+0x1c));break;
  }
 }
}
