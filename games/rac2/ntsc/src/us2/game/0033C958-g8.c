#include "include_asm.h"
typedef int s32_F09f721e0;
typedef float f32_F09f721e0;
extern s32_F09f721e0 func_00336E40 (s32_F09f721e0, s32_F09f721e0);
extern s32_F09f721e0 func_00336DA8 (s32_F09f721e0, s32_F09f721e0);
extern void func_0033ABD8 (s32_F09f721e0, s32_F09f721e0, const void *);
extern s32_F09f721e0 func_00288FB8 (s32_F09f721e0);
extern void func_0033B050 (s32_F09f721e0, s32_F09f721e0, s32_F09f721e0, s32_F09f721e0);
extern void func_0033B030 (s32_F09f721e0, f32_F09f721e0, f32_F09f721e0, f32_F09f721e0, f32_F09f721e0, f32_F09f721e0, f32_F09f721e0);
extern void func_0033B0A8 (s32_F09f721e0, f32_F09f721e0);
extern void func_0033BE98 (s32_F09f721e0, s32_F09f721e0);
extern s32_F09f721e0 D_L04_001AE040 __attribute__ ((sda));

void func_US2_0033CDB8 (s32_F09f721e0 self, s32_F09f721e0 arg)
{
  s32_F09f721e0 q;
  s32_F09f721e0 p;
  s32_F09f721e0 a, b;
  *(s32_F09f721e0 *) self = arg;
  if (arg != 0)
    {
      p = func_00336DA8 (16, func_00336E40 (arg, arg));
      *(s32_F09f721e0 *) (self + 732) = p;
      ((s32_F09f721e0 *) p)[1] = 0;
      ((s32_F09f721e0 *) p)[2] = 0;
      ((s32_F09f721e0 *) p)[3] = 0;
      ((s32_F09f721e0 *) p)[0] = 0;
    }
  q = self + 8;
  ((f32_F09f721e0 *) *(s32_F09f721e0 *) (self + 732))[0] = 255.0f;
  ((f32_F09f721e0 *) *(s32_F09f721e0 *) (self + 732))[1] = 195.0f;
  func_0033ABD8 (q, arg, &D_L04_001AE040);
  a = func_00288FB8 (12411);
  b = func_00288FB8 (11236);
  func_0033B050 (q, a, b, func_00288FB8 (11237));
  func_0033B030 (q, 0.0f, -143.0f, 0.0f, 114.0f, 0.0f, 125.0f);
  func_0033B0A8 (q, 0.65f);
  *(s32_F09f721e0 *) (self + 728) = 0;
  *(s32_F09f721e0 *) (q + 716) = 0;
  func_0033BE98 (self, 0);
}

typedef unsigned char u8_Ff4eb83c9;
typedef int s32_Ff4eb83c9;
typedef float f32_Ff4eb83c9;
extern f32_Ff4eb83c9 D_L04_001AE070[2] __attribute__((sda));
extern void *func_00336E40_3(void *);
extern void *func_00336DA8_3(s32_Ff4eb83c9, void *);
extern void func_0033ABD8_3(u8_Ff4eb83c9 *, void *, f32_Ff4eb83c9 *);
extern void *func_00288FB8_2(s32_Ff4eb83c9);
extern void func_0033B050_2(u8_Ff4eb83c9 *, void *, void *, void *);
extern void func_0033B030_3(u8_Ff4eb83c9 *, f32_Ff4eb83c9, f32_Ff4eb83c9, f32_Ff4eb83c9, f32_Ff4eb83c9, f32_Ff4eb83c9, f32_Ff4eb83c9);
extern void func_0033B0A8_2(u8_Ff4eb83c9 *, f32_Ff4eb83c9);
extern void func_0033C590(u8_Ff4eb83c9 *, s32_Ff4eb83c9);

void func_US2_0033D4B0(u8_Ff4eb83c9 *self, void *arg) {
    u8_Ff4eb83c9 *sub;
    void *first, *second, *third;
    *(void **)self = arg;
    if (arg != 0) {
        void *t = func_00336E40_3(arg);
        void *p = func_00336DA8_3(16, t);
        *(void **)(self + 732) = p;
        ((s32_Ff4eb83c9 *)p)[1] = 0;
        ((s32_Ff4eb83c9 *)p)[2] = 0;
        ((s32_Ff4eb83c9 *)p)[3] = 0;
        ((s32_Ff4eb83c9 *)p)[0] = 0;
    }
    sub = self + 8;
    ((f32_Ff4eb83c9 *)*(void **)(self + 732))[0] = 255.0f;
    ((f32_Ff4eb83c9 *)*(void **)(self + 732))[1] = 195.0f;
    func_0033ABD8_3(sub, arg, D_L04_001AE070);
    first = func_00288FB8_2(11310);
    second = func_00288FB8_2(11236);
    third = func_00288FB8_2(11237);
    func_0033B050_2(sub, first, second, third);
    func_0033B030_3(sub, 0.0f, -143.0f, 0.0f, 114.0f, 0.0f, 140.0f);
    func_0033B0A8_2(sub, 0.65f);
    *(s32_Ff4eb83c9 *)(self + 728) = 0;
    func_0033C590(self, 0);
}

typedef unsigned char u8_Fe9770b1e;
typedef int s32_Fe9770b1e;
typedef float f32_Fe9770b1e;
extern f32_Fe9770b1e D_L04_001AE0B8[2] __attribute__((sda));
extern void *func_00336E40_1(void *);
extern void *func_00336DA8_1(s32_Fe9770b1e, void *);
extern void func_0033ABD8_1(u8_Fe9770b1e *, void *, f32_Fe9770b1e *);
extern void *func_00288FB8_1(s32_Fe9770b1e);
extern void func_0033B050_1(u8_Fe9770b1e *, void *, void *, void *);
extern void func_0033B030_1(u8_Fe9770b1e *, f32_Fe9770b1e, f32_Fe9770b1e, f32_Fe9770b1e, f32_Fe9770b1e, f32_Fe9770b1e, f32_Fe9770b1e);
extern void func_0033B0A8_1(u8_Fe9770b1e *, f32_Fe9770b1e);
extern void func_L00_00432448(u8_Fe9770b1e *, s32_Fe9770b1e);

void func_US2_0033DA38(u8_Fe9770b1e *self, void *arg) {
    u8_Fe9770b1e *sub;
    void *first, *second, *third;
    *(void **)self = arg;
    if (arg != 0) {
        void *t = func_00336E40_1(arg);
        void *p = func_00336DA8_1(16, t);
        *(void **)(self + 732) = p;
        ((s32_Fe9770b1e *)p)[1] = 0;
        ((s32_Fe9770b1e *)p)[2] = 0;
        ((s32_Fe9770b1e *)p)[3] = 0;
        ((s32_Fe9770b1e *)p)[0] = 0;
    }
    sub = self + 8;
    ((f32_Fe9770b1e *)*(void **)(self + 732))[0] = 255.0f;
    ((f32_Fe9770b1e *)*(void **)(self + 732))[1] = 195.0f;
    func_0033ABD8_1(sub, arg, D_L04_001AE0B8);
    first = func_00288FB8_1(11305);
    second = func_00288FB8_1(11236);
    third = func_00288FB8_1(11237);
    func_0033B050_1(sub, first, second, third);
    func_0033B030_1(sub, 0.0f, -143.0f, 0.0f, 114.0f, 0.0f, 140.0f);
    func_0033B0A8_1(sub, 0.2f);
    *(s32_Fe9770b1e *)(self + 728) = 0;
    func_L00_00432448(self, 0);
}
