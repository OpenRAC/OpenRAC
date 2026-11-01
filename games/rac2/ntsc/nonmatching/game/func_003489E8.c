/* GPT-6.1 Sol high: USA v1.01, game -O2 -G8 -ffunction-sections.
 * 1236/1240 bytes, 0/28 places. Rechecked the sol-low retained draft here.
 * Initial frame/saves match; branch pointer allocation, pe4 spill placement,
 * initial anchor-load scheduling and final selection loop still differ.
 * An inline helper assigning pe4 around the first position call is identical.
 * A typed enabled[7] member restores the explicit +0x418 base but yields
 * 1244 bytes and still hoists -1. Allocator: pe4 pseudo 149 has 3 references,
 * live length 160, crosses 12 calls and spills; p130's join pseudo gets s0,
 * while retail keeps pe4 in s0 through its position call and spills it for
 * the final call. Need a measured lifetime/ownership change, not fake uses.
 * No linked validation for this near miss. Required declarations follow.
 */
#include "include_asm.h"
extern void func_00335E68(void *,int);
extern void func_003363C0(void *,float);
extern float *func_00335E10(void *);
extern int *func_00335E20(void *);
extern void func_00335F88(void *,float,float,float,float);
extern void func_002E5E58(int,int,int);
extern int func_002A9588(int,int,int,int,int);
extern float D_L00_001AE5EC __attribute__((sda));
extern float D_L00_001AE760, D_L00_001AE764, D_L00_001AE768, D_L00_001AE76C;
extern float D_L00_001AE770, D_L00_001AE774, D_L00_001AE778, D_L00_001AE77C;
extern float D_L00_001AE780, D_L00_001AE784;
typedef struct {float x,y;} MenuPositionPair;
int func_003489E8(char *o,int buttons)
{
 char *pe4; MenuPositionPair *p; int i,step;
 {char *p98=o+0x98,*p4c=o+0x4c,*p130=o+0x130,*p17c=o+0x17c,*p1c8=o+0x1c8;
 func_00335E68(o,*(int *)(o+0x44c)==0);
 func_00335E68(p98,*(int *)(o+0x44c)==0);
 func_00335E68(p4c,*(int *)(o+0x44c)==0);
 func_00335E68(o+0xe4,*(int *)(o+0x44c)==0);
 func_00335E68(p130,*(int *)(o+0x44c)==0);
 func_00335E68(p17c,*(int *)(o+0x44c)==1);
 func_00335E68(p1c8,*(int *)(o+0x44c)==1);
 func_003363C0(p17c,D_L00_001AE5EC);
 func_003363C0(p1c8,D_L00_001AE5EC);
 }
 if(*(int *)(o+0x44c)==0 && *(int *)(o+0x448)!=0) {
  func_003363C0(o,1.0f);func_003363C0(o+0x98,1.0f);func_003363C0(o+0x4c,1.0f);func_003363C0(o+0x130,1.0f);
  (*(float **)(o+0x434))[1]=170.0f;
 } else {
  func_003363C0(o,0.0f);func_003363C0(o+0x98,0.0f);func_003363C0(o+0x4c,0.0f);func_003363C0(o+0x130,0.0f);
  (*(float **)(o+0x434))[1]=190.0f;
 }
 {char *p98=o+0x98,*p4c=o+0x4c,*p130=o+0x130;
 p=(MenuPositionPair *)func_00335E10(o);pe4=o+0xe4;{float x=D_L00_001AE760,y=D_L00_001AE764;p->x=x+(*(float **)(o+0x434))[0];p->y=y+(*(float **)(o+0x434))[1];}
 p=(MenuPositionPair *)func_00335E10(p130);{float x=D_L00_001AE768,y=D_L00_001AE76C;p->x=x+(*(float **)(o+0x434))[0];p->y=y+(*(float **)(o+0x434))[1];}
 p=(MenuPositionPair *)func_00335E10(p4c);{float x=D_L00_001AE770,y=D_L00_001AE774;p->x=x+(*(float **)(o+0x434))[0];p->y=y+(*(float **)(o+0x434))[1];}
 p=(MenuPositionPair *)func_00335E10(p98);{float x=D_L00_001AE778,y=D_L00_001AE77C;p->x=x+(*(float **)(o+0x434))[0];p->y=y+(*(float **)(o+0x434))[1];}
 p=(MenuPositionPair *)func_00335E10(pe4);{float x=D_L00_001AE780,y=D_L00_001AE784;p->x=x+(*(float **)(o+0x434))[0];p->y=y+(*(float **)(o+0x434))[1];}
 }
 for(i=0;i<*(int *)(o+0x444);i++) {
  char *child=o+0x218+i*72;
  p=(MenuPositionPair *)func_00335E10(child);
  p->x=(*(float **)(o+0x414))[i*3]+(*(float **)(o+0x434))[0];
  p->y=(*(float **)(o+0x414))[i*3+1]+(*(float **)(o+0x434))[1];
  func_00335F88(child,(*(float **)(o+0x414))[i*3+2],0.0f,0.0f,0.0f);
  if(*(int *)(o+0x418+i*4)) *func_00335E20(child)=0x80f0f0f0;
  else *func_00335E20(child)=0x80808080;
 }
 step=0;
 if(buttons&0x1000) {func_002E5E58(3,0,0);step=-1;}
 else if(buttons&0x4000) {func_002E5E58(3,0,0);step=1;}
 if(step) {
  int old=*(int *)(o+0x43c);
  func_002A9588(0,0,1,0,1);
  do {
   int last=*(int *)(o+0x444)-1;
   int next=*(int *)(o+0x43c)+step;
   if(next>last)next=0;
   next=next>=0?next:last;
   *(int *)(o+0x43c)=next;
   if(((int *)(o+0x418))[next])break;
  }while(old!=*(int *)(o+0x43c));
 }
 func_003363C0(pe4,(float)*(int *)(o+0x43c));
 *func_00335E20(o+0x218+*(int *)(o+0x43c)*72)=0x70fffeed;
 return *(int *)(o+0x43c);
}
