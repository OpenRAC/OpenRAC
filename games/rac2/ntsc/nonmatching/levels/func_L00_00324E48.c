/* func_L00_00324E48: M3 2026-10-26: 196/196, 5 lines (schedule only). The callee func_L00_00324D18 takes (p, p, f, f, f, f, f, p): that fixed the parameter swap (14 -> 5). Left: the $f16 copy and the second call a1 slot. In the permuter queue.
 */
typedef int s32;
typedef float f32;
typedef unsigned char u8;

typedef struct { u8 pad0[0x80]; float v80[4]; u8 pad90[0x2290-0x90]; u8 *obj; } Hero_324E48;
extern Hero_324E48 D_00189E20;

extern void func_00282D30(void *, void *, float);

extern void func_00282CC0(void *, void *, void *);

extern void func_L00_00324D18(void *,void *,f32,f32,f32,f32,f32,void *);

void func_L00_00324E48(void *a0,f32 f1,f32 f2,f32 f3,f32 f4,f32 f5,void *a1) {
 Hero_324E48 *b=&D_00189E20;
 float buf[4];
 func_00282D30(buf,(void *)(b->obj+0xE0),0.75f);
 func_00282CC0(buf,b->v80,buf);
 func_L00_00324D18(buf,a0,f1,f2,f3,f4,f5,a1);
}
