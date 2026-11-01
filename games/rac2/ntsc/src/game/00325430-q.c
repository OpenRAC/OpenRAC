#include "include_asm.h"

typedef int TI128 __attribute__((mode(TI)));
typedef unsigned int u32;
typedef struct {
    TI128 position[4];
    u32 color[4];
    float uv[4][2];
    unsigned long primitive, texture, mode, registers;
} EffectPacket;
typedef struct {
    EffectPacket packet;
    float a[4];
    float b[4];
    float c[4];
    TI128 dead;
    TI128 axis;
} Frame;

extern unsigned long func_0027C878(int);
extern void ref_00325430_00(void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_002830C0(void *, void *, void *);
extern void func_00280B90(void *, void *, int);
extern char ref_00325430_01[];

/* Fills an effect packet (texture 14, uv square, corners from the state block's two reference points) and draws it. */
void func_00325430(char *obj)
{
    Frame f;
    char *st = *(char **)(obj + 0x68);
    char *p0;
    char *p1;

    f.packet.texture = func_0027C878(14);
    p1 = st + 0x310;
    p0 = st + 0x300;
    f.axis = 0;
    f.packet.mode = 0x0000ff9000000260UL;
    f.packet.registers = 0x0000008000000048UL;
    f.packet.primitive = 0;
    f.packet.color[1] = 0x1F7F7F7F;
    f.packet.color[0] = 0x1F7F7F7F;
    f.packet.color[3] = 0x1F7F7F7F;
    ((float *)&f.axis)[3] = 1.0f;
    f.packet.color[2] = 0x1F7F7F7F;
    ((float *)&f.axis)[2] = 0.05000000074505806f;
    ref_00325430_00(&f.packet);
    f.packet.uv[0][0] = 0;
    f.packet.uv[0][1] = 0;
    f.packet.uv[1][0] = 0;
    f.packet.uv[1][1] = 1.0f;
    f.packet.uv[2][0] = 1.0f;
    f.packet.uv[2][1] = 0;
    f.packet.uv[3][0] = 1.0f;
    f.packet.uv[3][1] = 1.0f;
    func_00282CF0(f.a, p1, p0);
    func_00282F20(f.a, f.a, 1.0f);
    func_00282CF0(f.b, ref_00325430_01, p0);
    func_00282DD8(f.c, f.b, f.a);
    func_00282F20(f.c, f.c, 1.0f);
    func_00282DD8(f.b, f.a, f.c);
    f.dead = *(TI128 *)p0;
    f.packet.position[0] = f.axis;
    func_00282D30(&f.packet.position[1], &f.packet.position[0], -1.0f);
    func_002830C0(&f.packet.position[0], &f.packet.position[0], f.a);
    func_002830C0(&f.packet.position[1], &f.packet.position[1], f.a);
    f.dead = *(TI128 *)p1;
    f.packet.position[2] = f.axis;
    func_00282D30(&f.packet.position[3], &f.packet.position[2], -1.0f);
    func_002830C0(&f.packet.position[2], &f.packet.position[2], f.a);
    func_002830C0(&f.packet.position[3], &f.packet.position[3], f.a);
    func_00280B90(&f.packet, 0, 0);
}
