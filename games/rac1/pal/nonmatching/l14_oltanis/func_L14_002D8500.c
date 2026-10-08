/* NON_MATCHING func_L14_002D8500 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 660 / retail 668, checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a01: Draws two transformed quad sets. p0/p1 canonical candidates cannot compile because later source de
 *   Stop isolated existing declaration wall; lead must correct prototype before retry. Diagnostic p2 exact-symbol 
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern u64 func_001F4868(s32);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, s32);
extern char D_L14_001675C0[];
extern float D_L14_001DE840[][4];
extern float D_0013F6E0[];
extern void func_L14_002D8500_a01(void *) __asm__("func_L14_002D8500");
// Draws two shaded sets of transformed quads around stored positions.
void func_L14_002D8500_a01(void *m) {
 char packet[0x90];
 float a[4],b[4],c[4],v[4];
 char *d=*(char **)(m+0x78);
 float *dst,*src;
 int i;
 qcopy(v,d+0xE0);v[3]=1.0f;
 func_001F9BF0(a,D_L14_001675C0,v);
 func_L00_001FF4B0(a,a,1.0f);
 func_001F9CA0(b,a,D_0013F6E0);
 func_L00_001FF4B0(b,b,-1.0f);
 func_001F9CA0(c,b,a);
 *(u64 *)(packet+0x78)=func_001F4868(11);
 *(u64 *)(packet+0x88)=0x8000000048ULL;
 *(u64 *)(packet+0x80)=0xFF9000000260ULL;
 *(u64 *)(packet+0x70)=5;
 *(int *)(packet+0x40)=0x80FFFFFF;
 *(float *)(packet+0x6C)=1.0f;
 *(int *)(packet+0x4C)=0x80FFFFFF;
 *(int *)(packet+0x48)=0x80FFFFFF;
 *(int *)(packet+0x44)=0x80FFFFFF;
 *(int *)(packet+0x50)=0;*(int *)(packet+0x54)=0;*(int *)(packet+0x58)=0;
 *(float *)(packet+0x5C)=1;*(float *)(packet+0x60)=1;*(int *)(packet+0x64)=0;*(float *)(packet+0x68)=1;
 dst=(float *)packet;src=D_L14_001DE840[0];i=3;
 do {func_001F9C30(dst,src,0.4f);--i;src+=4;func_001F9EE8(dst,dst,a);dst+=4;}while(i>=0);
 func_L00_001FD1D8(packet,0,0);
 qcopy(v,d+0x200);
 func_001F9BF0(a,D_L14_001675C0,v);
 func_L00_001FF4B0(a,a,1.0f);
 func_001F9CA0(b,a,D_0013F6E0);
 func_L00_001FF4B0(b,b,-1.0f);
 func_001F9CA0(c,b,a);
 *(int *)(packet+0x40)=0x202020FF;*(int *)(packet+0x4C)=0x202020FF;*(int *)(packet+0x48)=0x202020FF;*(int *)(packet+0x44)=0x202020FF;
 dst=(float *)packet;src=D_L14_001DE840[0];i=3;
 do {func_001F9C30(dst,src,0.2f);--i;src+=4;func_001F9EE8(dst,dst,a);dst+=4;}while(i>=0);
 func_L00_001FD1D8(packet,0,0);
}
