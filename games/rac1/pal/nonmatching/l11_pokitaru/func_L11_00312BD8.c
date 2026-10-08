/* NON_MATCHING func_L11_00312BD8 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: BYTES 25/564 (95.6% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Projects radar icon position and fades alpha near the rim; p0/p1/p2 compile identically at BYTES 25/564.
 *   Stopped at three equivalent vertical-offset/table setup forms: global load uses v1 instead of a1 and icon-inde
 *   Needs natural scheduling change in final icon block; body otherwise matches.
 */
extern float func_001FA790(float, float);
extern void func_001FA218(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001FA888(int);
extern int func_001F4868(int);
extern void func_001F5800(int,int,int,int,int,int,int,int,long,long);
extern unsigned char D_0015EEB4_m[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern int D_0013E604 MACRO_ADDR;
typedef struct { short tex,u,v,w,h,unknown,x,y; } L11RadarIcon;
extern L11RadarIcon D_L11_001F1318[];
/* Draws a radar icon after projection and fades its alpha at the rim. */
void func_L11_00312BD8(char *rot,char *from,char *moby,int icon,long color) {
 float turn[4],mat[16],delta[4],pos[4];
 float len,side;
 int x,y;
 L11RadarIcon *t;
 qzero(turn);
 turn[2]=*(float *)(rot+8);
 if (*(float *)(rot+4)>1.5707964f || *(float *)(rot+4)<-1.5707964f) turn[2]=-turn[2];
 turn[2]=func_001FA790(1.5707964f,turn[2]);
 func_001FA218(mat,turn);
 func_001F9BF0(delta,moby+0x10,from);
 delta[2]=0.0f;
 func_001F9EE8(pos,delta,mat);
 func_001F9C30(pos,pos,0.142857149f);
 len=func_001F9CB8(pos);
 side=1.0f;
 if(D_0015EEB4_m[0]) side=-1.0f;
 x=func_001FA898_r(side*pos[0]);
 y=func_001FA898_r(-pos[1]);
 if(len<46.0f) {
  if(len>38.0f) {
   float fade;
   len=46.0f-len;
   fade=func_001FA888(((unsigned long)color>>24)&255)*len;
   color&=0xFFFFFF;
   color=(func_001FA898_r(fade*0.125f)<<24)|color;
  }
  { int vertical=D_0013E604;
  y-=0x50; y+=vertical;
  t=&D_L11_001F1318[icon];
  x+=0x1B0; }
  func_001F5800(x-t->x,y-t->y,t->w,t->h,t->u,t->v,t->w,t->h,color,func_001F4868(t->tex));
 }
}
