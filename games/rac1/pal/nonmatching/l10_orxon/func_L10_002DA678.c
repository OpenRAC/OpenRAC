/* NON_MATCHING func_L10_002DA678 -- src/overlays/l10_orxon/vendor_00296BD8.c
 * Best so far: BYTES 3/664 (99.5% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a01: Button proximity/contact and linked-object update. Best p7.c BYTES3/664: separate loop count plus 
 *   Remaining +188 addu uses base/result v0 vs retail index/result v1, +18c linked-object data pointer load v0 vs 
 */
extern void func_L00_00251328(void *, int, int, int);
extern int func_L00_001F2BE8(float,void *,int,void *,void *);
extern float func_00214358(void *, int, float);
extern float func_001F9B88(float);
extern int func_001F9908(int *);
extern int func_0022ED80(int, int, int);
extern int func_001F9850(int);
extern char *D_L10_00160058_a01 __asm__("D_L10_00160058") MACRO_ADDR;
extern char D_L10_00178400[];
extern char D_L10_00174340[];
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern unsigned char D_0014C150[];
// Updates a button and its linked object from nearby activating objects.
void func_L10_002DA678(char *m) {
 char *d=*(char **)(m+0x78), *link;
 int active=0,n;
 char **list;
 char *contact;
 if (*(int *)d==-1) {func_L00_00251328(m,8,8,8);return;}
 link=D_L10_00160058_a01+(*(int *)d<<8);
 n=func_L00_001F2BE8(1.0f,m+0x10,0,m,0);
 if(n!=0) {
  if(n>0) {
   int count;
   list=(char **)D_L10_00178400;
   contact=D_L10_00174340;
   count=n;
   do {
    float z=func_00214358(*list+0x10,0,0.5f);
    if(*(char **)(contact+0x18)==m) {
     if(func_001F9B88(z-*(float *)(*list+0x18))<0.1f) {
      if(*(short *)(*list+0xA6)!=0x3EF) active=1;
     }
    }
    --count; ++list;
   }while(count);
  }
 }
 func_001F9908((int *)(d+0x10));
 if(D_0014C150[(unsigned char)m[0xB0]+(D_0015EE84_m<<4)]==0xFF)active=1;
 if(!active) {
  if(*(int *)(d+0xC)==0)goto inactive;
  *(int *)(d+0xC)=0;
 } else {
  if(*(int *)(d+4)!=-1) *(int *)(*(char **)((*(int *)(d+4)<<8)+D_L10_00160058_a01+0x78)+0xC)=1;
  else *(int *)(d+0xC)=0;
 }
 if((unsigned char)link[0x20]>=1 && (unsigned char)link[0x20]<=2) {
  link[0x20]=3;
  if(*(int *)(d+0x10)==0) {
   func_0022ED80(0,0,(int)link);func_0022ED80(0,0,(int)m);
   *(int *)(d+0x10)=func_001F9850(0x1E);
  }
 }
 func_L00_00251328(m,0x7F,0x7F,0x7F);return;
inactive:
 if((unsigned char)link[0x20]==4) {
  link[0x20]=1;
  if(*(int *)(d+0x10)==0) {
   func_0022ED80(1,0,(int)link);func_0022ED80(0,0,(int)m);
   *(int *)(d+0x10)=func_001F9850(0x1E);
  }
 }
 func_L00_00251328(m,8,8,8);
}
