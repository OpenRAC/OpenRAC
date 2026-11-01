/* GPT-6.1 Sol high, v0785, USA v1.01.
 * Best: 1252/1252 bytes, 0/28 places with
 * -O2 -G8 -ffunction-sections -g -fno-gcse.
 * Forward-indexed loops recover the active-loop allocation and exact FP math;
 * -g supplies the retail division padding. The retained profile avoids PRE
 * hoisting the quad UV bases across earlier loops, but spills the transform
 * pointer too early, rematerializes it in inactive paths and changes setup.
 * Remaining: initial mode/active scheduling, transform lifetime/spill point,
 * UV pointer increment order, inactive-loop base setup, quad-loop table base
 * hoist and allocation. Default GCSE gives 1308 bytes: two quad UV pointers
 * are hoisted into all earlier branches and spilled across 12 calls; verified
 * with regalloc.py (diagnostic object byte-identical to its baseline).
 * Unsplit GCSE edges did not change that result. Separate stack buffers,
 * direct globals, table sda and disabling the first scheduler did not close it.
 * The matching-behavior reference is RAC1 space.c's
 * render_environment_mapped_object (adapted there from Lombyte, GPL-3.0).
 * Required declarations follow. No linked build run for this near miss.
 */
#include "common.h"
extern float D_002625B0[][4], D_002620A0[][4], D_00214A30[][4];
extern float D_00214CE0[][2], D_00214E38[][2];
extern short D_00262350[][8];
extern int D_001A8BB0;
extern float D_001B5200[];
extern int D_001ACBB0 __attribute__((sda));
extern int D_001ABF00 __attribute__((sda));
extern int D_001ABF04 __attribute__((sda));
extern int D_001ABF08 __attribute__((sda));
extern short D_00152C20 NOT_SDA;
extern unsigned long long func_0027C878(int);
extern void func_0029FC20(void *, int, void *);
extern float func_00282C48(float);
extern void func_00282948(int *);
extern float func_00283CE0(int);
extern void func_002830C0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern float func_00282DB0(void *, void *);
extern void func_00282D30(void *, void *, float);
extern float func_00282C10(float);
extern void func_00280B90(void *, int, int);
void func_002ECB70(void *object)
{
 struct {
  float vertex[4][4];
  unsigned int color[4];
  float uv[4][2];
  unsigned long long packet[4];
 } quad;
 float matrix[16], normal[4], reflected[4], ray[4];
 float *transform;
 int active;
 unsigned int color;
 float fraction;
 int i, j;
 char *o = object;
 float (*points)[4] = D_002620A0;
 float (*normals)[4] = D_002625B0;
 active = 0;
 quad.packet[1] = func_0027C878(21);
 quad.packet[0] = 0;
 color = D_001A8BB0 == 2 ? 0x28a5a281 : 0x3ca5a281;
 quad.packet[2] = 0xff9000000260ULL;
 quad.packet[3] = 0x8000000044ULL;
 quad.color[0] = quad.color[1] = quad.color[2] = quad.color[3] = color;
 transform = matrix;
 func_0029FC20(object, 0, transform);
 if (D_001A8BB0 == 0 || D_001A8BB0 == 2) {
  if (func_00282C48(D_001B5200[0x50] - *(float *)(o + 0x10)) < 16.0f &&
      func_00282C48(D_001B5200[0x51] - *(float *)(o + 0x14)) < 16.0f) active = 1;
 } else active = 1;
 if (D_001A8BB0 == 6 && D_00152C20 == 2) active = 0;
 if (active || D_001ABF00 == 1) {
  D_001ABF04 = 1;
  func_00282948(&D_001ABF08);
  fraction = func_00283CE0(D_001ABF08) / func_00283CE0(60);
  {
  float (*n)[4] = normals;
  float (*p)[4] = points;
  float (*dest)[4] = D_00214A30;
  float (*uv)[2] = D_00214CE0;
  float (*old)[2] = D_00214E38;
  for (i = 0; i < 43; i++) {
   float divisor, x, y;
   func_002830C0(dest[i], p[i], transform);
   func_00282CF0(ray, dest[i], &D_001B5200[0x50]);
   func_00282F20(ray, ray, 1.0f);
   func_002830C0(normal, n[i], transform);
   func_00282F20(normal, normal, 0.1f);
   func_00282D30(reflected, normal, 2.0f * func_00282DB0(normal, ray));
   func_00282CF0(reflected, ray, reflected);
   func_00282F20(reflected, reflected, 1.0f);
   reflected[2] += 1.0f;
   divisor = func_00282C10(reflected[2] * 2.0f) * 2.0f;
   if (D_001ABF00 == 1 || D_001ABF08 == 0) {
    uv[i][0] = reflected[0] / divisor + 0.5f;
    uv[i][1] = reflected[1] / divisor + 0.5f;
   } else {
    x = reflected[0] / divisor + 0.5f;
    uv[i][0] = x + (old[i][0] - x) * fraction;
    x = reflected[1] / divisor + 0.5f;
    uv[i][1] = x + (old[i][1] - x) * fraction;
   }
  }
  }
  if (D_001ABF00 == 1) D_001ABF00 = 2;
 } else if (D_001ABF04 == 1) {
  D_001ABF04 = 0;
  {
   float (*dest)[4] = D_00214A30;
   float (*uv)[2] = D_00214CE0;
   float (*old)[2] = D_00214E38;
   float (*p)[4] = points;
   for (i = 0; i < 43; i++) {
    float x = uv[i][0];
    float y = uv[i][1];
    old[i][0] = x;
    old[i][1] = y;
    func_002830C0(dest[i], p[i], matrix);
   }
  }
  D_001ABF08 = 60;
 } else {
  float (*dest)[4] = D_00214A30;
  float (*p)[4] = points;
  for (i = 0; i < 43; i++) func_002830C0(dest[i], p[i], matrix);
  D_001ABF08 = 60;
 }
 {
 float *quad_u = &quad.uv[0][0];
 float *quad_v = &quad.uv[0][1];
 for (i = 0; i < 38; i++) {
  for (j = 0; j < 4; j++) {
   int index = D_00262350[i][j * 2];
   qcopy_nc(quad.vertex[j], D_00214A30[index]);
   {
    float x = D_00214CE0[index][0];
    float y = D_00214CE0[index][1];
    quad_u[j*2] = x;
    quad_v[j*2] = y;
   }
  }
  func_00280B90(&quad, 0, 0);
 }
}
}
