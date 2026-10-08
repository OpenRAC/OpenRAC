/* NON_MATCHING func_L13_002B8FC0 -- src/overlays/shared/vendor_002B8FC0.c
 * Best so far: SIZE ours 652 / retail 636, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_001FA218(float *,float *);
extern float func_001FA888(int);
extern void func_001F9EE8(void *,void *,void *);
extern void func_001F9C30(void *,void *,float);
extern int func_001FA898(float);
extern char *D_L13_00161240 MACRO_ADDR;
extern char D_L13_00160960[];
extern char D_L13_00160970[];
extern int D_0013E600[];
typedef int V128Packet __attribute__((mode(TI)));
// Builds a transformed screen-space vertex packet from signed coordinate pairs.
void func_L13_002B8FC0(short *points,int count,int value,long color,float x,float y,float scale,float rotation) {
 float mat[16],angles[4],pos[4],input[4]; char *old,*vertices,*out; int left=count;
 int words=(count+1)/2+3;
 *(int *)D_L13_00161240=words|0x10000000;
 *(int *)(D_L13_00161240+4)=0;*(int *)(D_L13_00161240+8)=0;*(int *)(D_L13_00161240+12)=words|0x50000000;
 old=D_L13_00161240;D_L13_00161240=old+16;qcopy(D_L13_00161240,D_L13_00160960);*(short *)(old+16)=-32767;
 old=D_L13_00161240;D_L13_00161240=old+16;*(long *)(old+16)=0x144;*(long *)(D_L13_00161240+8)=color;
 old=D_L13_00161240;D_L13_00161240=old+16;qcopy(D_L13_00161240,D_L13_00160970);*(short *)(old+16)=count-32768;
 *(V128Packet *)angles=0;angles[2]=rotation;
 vertices=D_L13_00161240+16;D_L13_00161240=vertices;func_001FA218(mat,angles);
 out=vertices;
 if(count>0) {
  long high=(long)value<<32;
  do {int ix,iy;
   *(V128Packet *)input=0;
   input[0]=func_001FA888(points[0]);input[1]=func_001FA888(points[1]);points+=2;left--;
   *(V128Packet *)pos=*(V128Packet *)input;
   func_001F9EE8(pos,pos,mat);func_001F9C30(pos,pos,scale);
   pos[0]+=x;pos[1]+=y;
   ix=func_001FA898(pos[0]*16.0f);iy=func_001FA898(pos[1]*16.0f);
   ix+=D_0013E600[4];iy+=D_0013E600[5];ix-=8;iy-=8;
   *(long *)out=(long)ix|((long)iy<<16)|high;out+=8;
  } while(left);
 }
 D_L13_00161240+=(count+1)/2*16;
}
