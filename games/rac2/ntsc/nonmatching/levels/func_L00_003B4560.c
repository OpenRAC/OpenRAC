/* GPT-6.1 Sol high worker v1426, 2026-10-29.
 * Best check.py result: 2324 compiled bytes / 2340 retail bytes; 0/27 places.
 * Profile: default game -O2 -G8 -ffunction-sections in a standalone -g8.c
 * unit. All required declarations are below; common.h supplies qcopy.
 *
 * Compared with v1185 (2236 bytes), multidimensional float mesh and UV
 * arrays, indexed inputs in all eight loops, and position stores before UV
 * evaluation restore retail's increment/slti control in the last six loops.
 * Their lengths and main instruction schedules now agree structurally.
 * The setup through qcopy and the 0xC50 frame also agree structurally.
 * Remaining differences: allocation keeps the position base in fp instead
 * of the UV base; first-two-loop output address formation, common color
 * store/counter increment, absolute-load temporaries and FP scheduling differ.
 * Later-loop saved registers are permuted. This is not a linked match.
 *
 * Measured alternatives: direct output field/array stores produce 2516 bytes
 * and extra pointer spills; -mno-split-addresses gives 2220 bytes and changes
 * loop induction; -fno-gcse gives 2204 bytes; a mixed struct-array draft with
 * -fno-rerun-loop-opt gives 2360 bytes but leaves UV indexing unreduced.
 * -g did not change an earlier 2236-byte draft. On the retained source,
 * -frac2-gcse-unsplit-edges plus -frac2-keep-block-order, a separate index for
 * the last six loops, and early continue after the alpha store each tie at
 * the same 2324 bytes. Incomplete sda views at -G8 incorrectly turn the
 * retail absolute byte/word loads into GP accesses; those were not retained.
 * Stop: tie after 14 function checks and one allocation diagnostic.
 * Next: establish the original pointer/copy idiom or a corresponding RAC1
 * body before more allocation trials; no counterpart was identified here.
 *
 * Retail draws two planar meshes (148 and 60 vertices) in four combinations
 * of coordinate signs, scrolling UVs; only the first pair updates alpha.
 */
#include "common.h"
typedef unsigned int MeshQuad __attribute__((mode(TI)));
typedef union { MeshQuad q; float v[4]; } MeshVector;
extern float D_L00_001AF9DC __attribute__((sda)), D_L00_001AF9E0 __attribute__((sda)), D_L00_001AF9E4 __attribute__((sda));
extern int D_L00_001AF9D8 __attribute__((sda)), D_L00_001AF998 __attribute__((sda)), D_L00_001AF9D0 __attribute__((sda));
extern u32 D_L00_001AF9E8 __attribute__((sda));
extern int D_L00_001AF99C NOT_SDA, D_L00_001AF9D4 NOT_SDA;
extern signed char D_L00_001AF9EF NOT_SDA;
extern MeshVector D_L00_001B8DC0 NOT_SDA, D_0018A0E0 NOT_SDA;
extern float D_L00_00294CA0[][3], D_L00_00295390[][3];
extern float D_L00_00295660[][2], D_L00_00295B00[][2];
extern u32 D_L00_00295CE0[], D_L00_00295F30[];
extern u64 func_0027C878(int);
extern void func_002FC560(int,u64), func_00280328(void);
extern void func_00282CF0(void *,void *,void *), func_00282F20(void *,void *,float), func_00282DD8(void *,void *,void *), func_002830C0(void *,void *,void *);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern void func_00281810(int,void *,int,void *,int);
void func_L00_003B4560(unsigned char *object, float opacity)
{
    MeshVector transform[4];
    float positions[148][3];
    float uv[148][2];
    MeshVector temp;
    int i, alpha;
    float u,v;
    float *p;
    unsigned char *origin;
    D_L00_001AF9DC += D_L00_001AF9E0;
    if (D_L00_001AF9DC >= 7.0f) D_L00_001AF9DC -= 7.0f;
    { u64 texture = func_0027C878(D_L00_001AF9D8);
      origin = object + 0x10;
      func_002FC560(6,texture); }
    func_002FC560(0x42,0x8000000048ULL);
    func_002FC560(8,0);
    func_002FC560(0x14,0xFF9000000260ULL);
    func_00280328();
    func_00282CF0(&transform[0],&D_L00_001B8DC0,origin);
    func_00282F20(&transform[0],&transform[0],1.0f);
    func_00282DD8(&transform[1],&transform[0],&D_0018A0E0);
    func_00282F20(&transform[1],&transform[1],-1.0f);
    func_00282DD8(&transform[2],&transform[1],&transform[0]);
    qcopy(&transform[3], origin);
    for (i=0;i<148;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = D_L00_00294CA0[i][1] * D_L00_001AF9E4;
        temp.v[2] = D_L00_00294CA0[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295660[i][1] + D_L00_001AF9DC;
        v = D_L00_00295660[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
        if (D_L00_00295CE0[i] & 0xFF000000) {
            alpha=func_00283CF0(func_00283CE0(D_L00_001AF9EF)*opacity);
            if (alpha < 1) alpha=1;
            D_L00_00295CE0[i] = (alpha << 24) | D_L00_001AF9E8;
        } else D_L00_00295CE0[i] = D_L00_001AF9E8;
    }
    func_00281810(D_L00_001AF998,positions,D_L00_001AF9D0,uv,1);
    for (i=0;i<60;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = D_L00_00295390[i][1] * D_L00_001AF9E4;
        temp.v[2] = D_L00_00295390[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295B00[i][1] + D_L00_001AF9DC;
        v = D_L00_00295B00[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
        if (D_L00_00295F30[i] & 0xFF000000) {
            alpha=func_00283CF0(func_00283CE0(D_L00_001AF9EF)*opacity);
            if (alpha < 1) alpha=1;
            D_L00_00295F30[i] = (alpha << 24) | D_L00_001AF9E8;
        } else D_L00_00295F30[i] = D_L00_001AF9E8;
    }
    func_00281810(D_L00_001AF99C,positions,D_L00_001AF9D4,uv,1);
    for (i=0;i<148;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = -D_L00_00294CA0[i][1] * D_L00_001AF9E4;
        temp.v[2] = D_L00_00294CA0[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295660[i][1] + D_L00_001AF9DC;
        v = D_L00_00295660[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF998,positions,D_L00_001AF9D0,uv,1);
    for (i=0;i<60;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = -D_L00_00295390[i][1] * D_L00_001AF9E4;
        temp.v[2] = D_L00_00295390[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295B00[i][1] + D_L00_001AF9DC;
        v = D_L00_00295B00[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF99C,positions,D_L00_001AF9D4,uv,1);
    for (i=0;i<148;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = D_L00_00294CA0[i][1] * D_L00_001AF9E4;
        temp.v[2] = -D_L00_00294CA0[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295660[i][1] + D_L00_001AF9DC;
        v = D_L00_00295660[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF998,positions,D_L00_001AF9D0,uv,1);
    for (i=0;i<60;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = D_L00_00295390[i][1] * D_L00_001AF9E4;
        temp.v[2] = -D_L00_00295390[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295B00[i][1] + D_L00_001AF9DC;
        v = D_L00_00295B00[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF99C,positions,D_L00_001AF9D4,uv,1);
    for (i=0;i<148;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = -D_L00_00294CA0[i][1] * D_L00_001AF9E4;
        temp.v[2] = -D_L00_00294CA0[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295660[i][1] + D_L00_001AF9DC;
        v = D_L00_00295660[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF998,positions,D_L00_001AF9D0,uv,1);
    for (i=0;i<60;++i) {
        temp.v[0] = 0.0f;
        temp.v[1] = -D_L00_00295390[i][1] * D_L00_001AF9E4;
        temp.v[2] = -D_L00_00295390[i][0] * D_L00_001AF9E4;
        temp.v[3] = 1.0f;
        func_002830C0(&temp,&temp,transform);
        p = positions[i];
        p[0] = temp.v[0];
        p[1] = temp.v[1];
        p[2] = temp.v[2];
        u = D_L00_00295B00[i][1] + D_L00_001AF9DC;
        v = D_L00_00295B00[i][0];
        uv[i][0] = u;
        uv[i][1] = v;
    }
    func_00281810(D_L00_001AF99C,positions,D_L00_001AF9D4,uv,1);
}
