/*
 * Retained handwritten near miss; historical evidence only.
 * No linked-image equality or complete retail semantics is established.
 * Compiler: game EE-GCC 2.9-ee-991111b (project profile).
 * Profile: -O2 -G32 -Wa,-G32 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
 * Reported final C / retail: 1260 / 1252 bytes; size equality is not a match.
 * us: 0/1 placements; 1260/1252 bytes.
 * us2: 0/1 placements; 1260/1252 bytes.
 * pal: 0/1 placements; 1260/1252 bytes.
 * Residual: 1260 bytes versus1252; correct1D0 frame, interleaved-table base choices and allocation differ.
 * Reopening condition: Recover the actual interleaved UV table declarations and independently qualified GP/absolute addressing mechanism. Do not shrink32-byte tables to fitG8.
 * Where listed, tools/check.py measurements predate retention; no compilation was rerun for this copy.
 */

/* Historical manual summary: Earlier mapping described interleaved UV data and local lifetimes. The current draft retains full table extents; table-base choice and allocation remain unresolved. Historical helper identities and constant interpretations are superseded. The original map is retained privately; no extracted instruction, opcode, address or generated table is reproduced here. */

/* Manual reconstruction retained after measured nonmatching checks.
   Profile: -O2 -G32 -Wa,-G32 -ffunction-sections -frac2-qcopy -frac2-qcopy-barrier.
   1260 bytes versus1252; correct1D0 frame, interleaved-table base choices and allocation differ.
   Reopen: Recover the actual interleaved UV table declarations and independently qualified GP/absolute addressing mechanism. Do not shrink32-byte tables to fitG8.
   This is an unqualified draft, not an exact match or native behavior proof.
*/
typedef unsigned int u32;
typedef unsigned long long u64;
typedef int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vec;
typedef struct { float x, y; } Pair;
typedef struct { Vec position[4]; u32 color[4]; Pair uv[4]; u64 descriptor[4]; } TrailPacket;
extern int D_L20_001AEC6C __attribute__((sda)), D_L20_001AECA0 __attribute__((sda));
extern u32 D_L20_001AEC70[2] __attribute__((sda)), D_L20_001AECA8[2] __attribute__((sda));
extern Pair D_L20_001AEC80[4] __attribute__((sda)), D_L20_001AECB8[4] __attribute__((sda));
extern float D_L20_001AEC78 __attribute__((sda)), D_L20_001AEC7C __attribute__((sda)), D_L20_001AECB4 __attribute__((sda));
extern u32 D_L20_001AECD8 __attribute__((sda)), D_L20_001AECDC, D_L20_001AECE0, D_L20_001AECE4;
extern float D_L20_001AECE8 __attribute__((sda)), D_L20_001AECEC;
extern float D_L20_001B9FC0[4];
extern void func_00283658(void *,void *);
extern u64 func_0027C878(int);
extern u32 func_00283D38(u32,u32,float);
extern void func_00282C90(void *);
extern void func_002833F0(void *,void *);
extern void func_L01_002E1510(void *,int,void *,int,void *);
extern void func_00282CF0(void *,void *,void *);
extern void func_002AFDD8(char *,char *,char *,char *);
extern void func_00282F20(void *,void *,float);
extern void func_00280B90(void *,void *,int);
extern float func_002A7878(float,float);
extern float func_002A7940(void);
extern void func_00281EB8(void *,int,u32,int,float,float,float);

void func_L20_0039BEC8(char *object)
{
    TrailPacket packet;
    float matrix[16], rotated[16];
    Vec direction, normal, tangent;
    float *vars = *(float **)(object + 0x68);
    int i;
    u32 color;
    func_00283658(matrix, object + 0xc0);
    *(Quad *)(matrix + 12) = *(Quad *)(object + 0x10);
    packet.descriptor[1] = func_0027C878(D_L20_001AEC6C);
    packet.descriptor[0] = 0;
    packet.descriptor[2] = 0xff9000000260ULL;
    packet.descriptor[3] = 0x8000000048ULL;
    for (i = 0; i < 4; i++) {
        color = D_L20_001AEC70[(i & 1) ? 0 : 1];
        packet.uv[i].x = D_L20_001AEC80[i].x;
        packet.uv[i].y = D_L20_001AEC80[i].y;
        packet.color[i] = color;
        packet.color[i] = func_00283D38(color & 0xffffff, color, vars[0]);
        func_00282C90(&packet.position[i]);
        packet.position[i].f[0] = D_L20_001AEC7C;
        if ((i & 1) == 0) packet.position[i].f[2] = D_L20_001AEC78;
        else packet.position[i].f[2] = 0.0f;
    }
    *(Quad *)(rotated + 12) = 0;
    rotated[14] = 0.19634954631328583f;
    func_002833F0(rotated, rotated + 12);
    func_L01_002E1510(&packet, 1, rotated, 32, matrix);
    func_00282CF0(&direction, D_L20_001B9FC0, object + 0x10);
    func_002AFDD8(object, (char *)&direction, (char *)&direction, 0);
    normal.q = 0;
    normal.f[0] = direction.f[1];
    normal.f[1] = -direction.f[0];
    normal.f[3] = 1.0f;
    tangent.q = normal.q;
    direction.f[2] = 0.0f;
    packet.descriptor[1] = func_0027C878(D_L20_001AECA0);
    for (i = 0; i < 4; i++) {
        float width;
        packet.uv[i].x = D_L20_001AECB8[i].x + vars[3];
        packet.uv[i].y = D_L20_001AECB8[i].y;
        color = D_L20_001AECA8[(i & 1) ? 0 : 1];
        packet.color[i] = color;
        packet.color[i] = func_00283D38(color & 0xffffff, color, vars[0]);
        width = i > 1 ? D_L20_001AECB4 : D_L20_001AEC7C;
        func_00282F20(&packet.position[i], &tangent, width);
        if ((i & 1) == 0) packet.position[i].f[2] = D_L20_001AEC78;
        else packet.position[i].f[2] = 0.0f;
    }
    func_00280B90(&packet, matrix, 0);
    for (i = 0; i < 4; i++) {
        float width = i > 1 ? D_L20_001AECB4 : D_L20_001AEC7C;
        func_00282F20(&packet.position[i], &tangent, -width);
        if ((i & 1) == 0) packet.position[i].f[2] = D_L20_001AEC78;
        else packet.position[i].f[2] = 0.0f;
    }
    func_00280B90(&packet, matrix, 0);
    normal.q = *(Quad *)(object + 0x10);
    normal.f[2] += 0.333f;
    color = func_00283D38(D_L20_001AECD8, D_L20_001AECDC, func_002A7878(0.0f,1.0f));
    color = func_00283D38(color & 0xffffff, color, vars[0]);
    func_00281EB8(&normal, 11, color, 1, D_L20_001AECE8, 0.0f, func_002A7940());
    color = func_00283D38(D_L20_001AECE0, D_L20_001AECE4, func_002A7878(0.0f,1.0f));
    color = func_00283D38(color & 0xffffff, color, vars[0]);
    func_00281EB8(&normal, 11, color, 1, D_L20_001AECEC, 0.0f, func_002A7940());
    func_00281EB8(object + 0x10, 49, color, 1, D_L20_001AECEC, 0.0f, func_002A7940());
}
