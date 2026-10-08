/* NON_MATCHING func_L13_002EA840 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 632 / retail 628, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern float func_001F9D10(void *,void *);
extern char *D_L13_001B0AB0[];
extern float D_0015EE60 MACRO_ADDR;
extern char D_0013F4D0[];
typedef int L13Quad __attribute__((mode(TI)));
/* Chooses a path direction and eases movement speed toward or away from the player. */
int func_L13_002EA840(char *moby,char *data,int index,int arg) {
 float next[4],previous[4];
 char *path=D_L13_001B0AB0[*(int *)(data+0xB0+index*4)];
 int result=0;
 if(path) {
  int point=*(short *)(data+0x9C)+1;
  int direction;
  float nearNext,nearPrevious;
  if(point>=*(int *)path)point=0;
  qcopy(next,path+point*16+16);
  nearNext=func_001F9D10(next,D_0013F4D0);
  point=*(short *)(data+0x9C)-1;
  if(point<0)point=*(int *)path-1;
  qcopy(previous,path+point*16+16);
  nearPrevious=func_001F9D10(previous,D_0013F4D0);
  if(nearNext<nearPrevious)direction=-1;
  else if(nearPrevious<nearNext)direction=1;
  else {direction=-1;if(*(signed char *)(data+0x10C)==1)direction=1;}
  if(direction!=*(signed char *)(data+0x10C)) {
   float speed=*(float *)(data+0xAC)*(1.0f+D_0015EE60*(-0.100000024f));
   *(float *)(data+0xAC)=speed;
   if(speed<D_0015EE60*0.15f) {
    short old=*(short *)(data+0x9C);
    *(signed char *)(data+0x10C)=direction;
    *(short *)(data+0x9C)=*(short *)(data+0x9E);
    *(short *)(data+0x9E)=old;
   }
  }else {
   float distance=func_001F9D10(moby+0x10,D_0013F4D0);
   float scale=1.0f;
   float speed=*(float *)(data+0xAC);
   if((unsigned char)moby[0xBC]==4)scale=3.0f;
   speed+=(scale*30.0f/distance-speed)*(D_0015EE60*0.12f);
   *(float *)(data+0xAC)=speed;
   if(speed<0.0f)*(float *)(data+0xAC)=0.0f;
  }
  result=func_L13_002EA598(moby,data,index,arg);
 }
 if(*(float *)(path+(*(short *)(data+0x9C)<<4)+0x1C)>0.0f)result|=2;
 return result;
}
