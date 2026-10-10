/* NON_MATCHING func_L00_002A11E8 -- src/overlays/shared/vendor_0029FD68.c
 * Best so far: BYTES 45/852 (94.7% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   VendorDrawHologramCone: scrolls a texture offset (D_L00_00161244 += 0.01, wrap at 1.0), then for 4 quads build
 *   Best candidate p5.c: 46 of 852 bytes differ, all in the prologue: retail emits the four `out[k] = &v[k]` store
 *   Unblock: a wording of the out[] pointer-array setup that the prepass scheduler keeps in pair order (initialize
 *   Round fz6/x02: p7 (out[0..3] order) 46 bytes, p8 (out stores before hdr) 104, p9 (1,0,3,2) 45. Still only the 
 */
#include "common.h"
extern int func_001F4868(int);
extern void func_001F9EC0(void *, void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
struct Quad { int a, b, c, d; };
extern struct Quad D_L00_001CA780[];
extern float D_L00_001CA680[][4];
extern float D_L00_001CA710[][2];
extern int D_L00_001CA758[];
extern int D_L00_001CA7C0[];
extern short D_L00_00161244;

struct HoloPacket {
    float pos[4][4];
    int col[4];
    float uv[4][2];
    long hdr[4];
};

/* Draws the four quads of the hologram cone, scrolling the texture coordinates. */
void func_L00_002A11E8(void) {
    struct HoloPacket s;
    float v[4][4];
    float *out[4];
    int i;
    *(float *)&D_L00_00161244 = *(float *)&D_L00_00161244 + 0.01f;
    if (*(float *)&D_L00_00161244 > 1.0f) *(float *)&D_L00_00161244 = *(float *)&D_L00_00161244 - 1.0f;
    s.hdr[2] = 1;
    s.hdr[3] = 0x4000000044L;
    s.hdr[0] = 0;
    s.hdr[1] = func_001F4868(0x18);
    out[1] = v[1];
    out[0] = v[0];
    out[3] = v[3];
    out[2] = v[2];
    for (i = 0; i < 4; i++) {
        func_001F9EC0(out[0], D_L00_001CA680[D_L00_001CA780[i].a], (char *)D_L00_001CA7C0[7] + 0xC0);
        func_001F9EC0(out[1], D_L00_001CA680[*(int *)((char *)D_L00_001CA780 + i * 16 + 4)], (char *)D_L00_001CA7C0[7] + 0xC0);
        func_001F9EC0(out[2], D_L00_001CA680[*(int *)((char *)D_L00_001CA780 + i * 16 + 8)], (char *)D_L00_001CA7C0[7] + 0xC0);
        func_001F9EC0(out[3], D_L00_001CA680[D_L00_001CA780[i].d], (char *)D_L00_001CA7C0[7] + 0xC0);
        s.pos[0][0] = *(float *)((char *)D_L00_001CA7C0[7] + 0x10) + v[0][0];
        s.pos[0][1] = *(float *)((char *)D_L00_001CA7C0[7] + 0x14) + v[0][1];
        s.pos[0][2] = *(float *)((char *)D_L00_001CA7C0[7] + 0x18) + v[0][2];
        s.pos[1][0] = *(float *)((char *)D_L00_001CA7C0[7] + 0x10) + v[1][0];
        s.pos[1][1] = *(float *)((char *)D_L00_001CA7C0[7] + 0x14) + v[1][1];
        s.pos[1][2] = *(float *)((char *)D_L00_001CA7C0[7] + 0x18) + v[1][2];
        s.pos[2][0] = *(float *)((char *)D_L00_001CA7C0[7] + 0x10) + v[2][0];
        s.pos[2][1] = *(float *)((char *)D_L00_001CA7C0[7] + 0x14) + v[2][1];
        s.pos[2][2] = *(float *)((char *)D_L00_001CA7C0[7] + 0x18) + v[2][2];
        s.pos[3][0] = *(float *)((char *)D_L00_001CA7C0[7] + 0x10) + v[3][0];
        s.pos[3][1] = *(float *)((char *)D_L00_001CA7C0[7] + 0x14) + v[3][1];
        s.pos[3][2] = *(float *)((char *)D_L00_001CA7C0[7] + 0x18) + v[3][2];
        s.col[0] = D_L00_001CA758[D_L00_001CA780[i].a];
        s.col[1] = D_L00_001CA758[*(int *)((char *)D_L00_001CA780 + i * 16 + 4)];
        s.col[2] = D_L00_001CA758[*(int *)((char *)D_L00_001CA780 + i * 16 + 8)];
        s.col[3] = D_L00_001CA758[D_L00_001CA780[i].d];
        s.uv[0][0] = D_L00_001CA710[D_L00_001CA780[i].a][0];
        s.uv[0][1] = D_L00_001CA710[D_L00_001CA780[i].a][1] + *(float *)&D_L00_00161244;
        s.uv[1][0] = D_L00_001CA710[*(int *)((char *)D_L00_001CA780 + i * 16 + 4)][0];
        s.uv[1][1] = D_L00_001CA710[*(int *)((char *)D_L00_001CA780 + i * 16 + 4)][1] + *(float *)&D_L00_00161244;
        s.uv[2][0] = D_L00_001CA710[*(int *)((char *)D_L00_001CA780 + i * 16 + 8)][0];
        s.uv[2][1] = D_L00_001CA710[*(int *)((char *)D_L00_001CA780 + i * 16 + 8)][1] + *(float *)&D_L00_00161244;
        s.uv[3][0] = D_L00_001CA710[D_L00_001CA780[i].d][0];
        s.uv[3][1] = D_L00_001CA710[D_L00_001CA780[i].d][1] + *(float *)&D_L00_00161244;
        func_L00_001FD1D8(&s, 0, 1);
    }
}
