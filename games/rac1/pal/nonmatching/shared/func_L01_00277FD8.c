/* NON_MATCHING func_L01_00277FD8 -- src/overlays/shared/mobyutil_0026E8E0.c
 * Best so far: SIZE ours 628 / retail 632, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 */
extern void func_001F9BF0(void *,void *,void *);
extern void func_L00_001FF4B0(void *,void *,float);
extern void func_001F9BD8(void *,void *,void *);
extern int func_L00_002629E0(int,void *,void *);
extern int func_L01_00277C30(int,int,int);
extern float func_001F9D48(void *,void *);
extern char *D_L01_001B0C30[];
/* Tests sight corridors and picks the nearest reachable path point if obstructed. */
int func_L01_00277FD8(int *ids,int count,int index,void *from,void *to,void *out,float width) {
 float delta[4],left[4],right[4];
 int blocked=0,i=0;
 { int *entry=ids;
 for(;i<count;i++,entry++) {
  if(width!=0.0f) {
   float x;
   func_001F9BF0(delta,from,to);
   func_L00_001FF4B0(delta,delta,width);
   x=delta[0]; delta[3]=x; delta[0]=delta[1]; delta[1]=-x;
   func_001F9BF0(left,from,delta);
   func_001F9BD8(right,from,delta);
   if(func_L00_002629E0(*entry,left,to))blocked=1;
   else if(func_L00_002629E0(*entry,right,to))blocked=1;
  } else if(func_L00_002629E0(*entry,from,to))blocked=1;
  if(blocked)break;
 }
 }
 if(blocked) {
  char *path=D_L01_001B0C30[index];
  int a=func_L01_00277A38(width,ids,count,index,from);
  int b=func_L01_00277A38(width,ids,count,index,to);
  int mask=func_L01_00277C30(index,a,b);
  if(mask) {
   int best=-1;
   float distance=10000.0f;
   char *point=path+16;
   for(i=0;i<*(int *)path;i++,point+=16) {
    if((mask>>i)&1) {
     float d=func_001F9D48(point,to);
     if(d<distance){distance=d;best=i;}
    }
   }
   if(best>=0){qcopy(out,path+16+best*16);return 1;}
  }
  return 0;
 }
 qcopy(out,to);
 return 1;
}
