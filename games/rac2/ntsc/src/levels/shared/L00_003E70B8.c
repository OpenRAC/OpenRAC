#include "include_asm.h"

typedef struct {
    char pad_000[0x140];
    float x_140;
    float y_144;
    float z_148;
} L00AimVector;

extern L00AimVector D_L00_001B8C80;

/* The two bodies below have more than one copy in the levels, and which copy this
   program calls changes from one program to the next: the retail's call is read from
   the bytes, so the name carries no address. */
extern void ref_L00_003E70B8_00(char *p);

extern void ref_L00_003E70B8_01(char *p, float scale);

extern long long func_0027C878(char *p);

extern float func_00283248(float, float);

extern float func_00282E80(char *p, float *v);

extern void func_00283410(float *matrix, float *quat);

extern void func_00282CF0(char *dst, char *src, char *arg);

extern void func_00282F20(char *dst, char *src, float scale);

extern void func_00280B90(char *pkt, float *matrix, int mode);

/* Builds a 144-byte packet, aims a quaternion at the shared 0x001B8C80 vector from pos through three float helpers, derives an offset basis by a fourth, writes the last matrix row from pos plus that offset, then dispatches the packet.
   Its own unit: the two bodies it calls are C in L00_003E3258.c, and a call inside one
   object fixes the distance between the two, which the retail does not keep in every
   level.  From here both calls are global and the linker places them per program. */
void func_L00_003E70B8(char *pos, int a1, int a2, char *a3, int t0,
                       float fA, float fB, float fC)
{
    char pkt[144];
    float m[16];
    float quat[4];
    float off[4];
    long long r;

    ref_L00_003E70B8_00(pkt);
    r = func_0027C878(a3);

    *(long long *)(pkt + 136) = ((long long)t0 << 2) | 0x8000000040LL;
    *(long long *)(pkt + 128) = 280993940374112LL;
    *(int *)(pkt + 76) = a1 + (a2 << 24);
    *(int *)(pkt + 72) = a1 + (a2 << 24);
    *(int *)(pkt + 68) = a1 + (a2 << 24);
    *(int *)(pkt + 64) = a1 + (a2 << 24);
    *(long long *)(pkt + 112) = 0LL;
    *(long long *)(pkt + 120) = r;

    quat[3] = 0.0f;
    quat[2] = func_00283248(D_L00_001B8C80.x_140 - *(float *)(pos + 0),
                            D_L00_001B8C80.y_144 - *(float *)(pos + 4));
    quat[1] = -func_00283248(func_00282E80(pos, &D_L00_001B8C80.x_140),
                             D_L00_001B8C80.z_148 - *(float *)(pos + 8));
    quat[0] = fB;

    func_00283410(m, quat);
    func_00282CF0(off, &D_L00_001B8C80.x_140, pos);
    func_00282F20(off, off, fC);

    m[12] = *(float *)(pos + 0) + off[0];
    m[13] = *(float *)(pos + 4) + off[1];
    m[14] = *(float *)(pos + 8) + off[2];
    m[15] = 1.0f;

    ref_L00_003E70B8_01(pkt, fA);
    func_00280B90(pkt, m, 0);
}
