/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L00_003E5978 (GS-quad sphere draw, 64 faces x 4 corners). Not matching: compiles to the
 * retail size (604 bytes) with the same instruction sequence in the loop; the remaining
 * difference is about 30 instructions of register/stack-slot assignment: retail spills
 * object+0xC0, &uv[0][1], &uv[0][0] and object+0x10 to 0xDC/0xD8/0xD4/0xD0($sp) in that order
 * (mine gets 0xD4/0xD0/0xDC/0xD8), keeps s0=texel index, s1=vertex, s2=face, s3=u, s4=v,
 * s5=j, s6=direction vector, s7=i+1, fp=&D_L00_00297420; and the GS ALPHA word is built as
 * ((a|b<<2)|c<<4)|d<<6)|e<<32 with the loads in b,a,c,d,e order. Tried: p loaded first,
 * named locals for object+0x10/0xC0 (worse: extra fp copy), swapping the u/v statements
 * (decides which of D_L00_00297420/D_L00_00297424 is hoisted into fp; retail hoists 0297420
 * when v is computed first and stored second), declaration order of u/v. Must live in a -g8
 * file (the D_L00_001B2B50..6C words are read $gp-relative). */
typedef struct { float vertices[4][4]; unsigned int colors[4]; float uv[4][2]; unsigned long long registers[4]; } __attribute__((aligned(16))) BeamQuad_L00_003E5978;

typedef struct {
    short corner;
    short texel;
} Face_L00_003E5978;

typedef struct {
    float u;
    float v;
} Texel_L00_003E5978;

extern unsigned long long func_0027C878(int);
extern void func_00282C88(void *);
extern void func_002AFFD8(void *, void *, int);
extern void func_00282D30(void *, void *, float);
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern void func_00280B90(void *, int, int);
extern int D_L00_001B2B50_2 __attribute__((sda));
extern int D_L00_001B2B54_2 __attribute__((sda));
extern int D_L00_001B2B58 __attribute__((sda));
extern int D_L00_001B2B5C __attribute__((sda));
extern int D_L00_001B2B60_1 __attribute__((sda));
extern int D_L00_001B2B64 __attribute__((sda));
extern int D_L00_001B2B68 __attribute__((sda));
extern float D_L00_001B2B6C __attribute__((sda));
extern float D_L00_00296B20[][4];
extern Face_L00_003E5978 D_L00_00297020[][4];
extern float D_L00_00297420[][2];
extern float D_L00_00297424[][2];

void func_L00_003E5978(char *object)
{
    BeamQuad_L00_003E5978 quad;
    float m[4][4] __attribute__((aligned(16)));
    char *p;
    int i, j, k;
    float fu, fv;
    float *vertex;
    float *u, *v;
    Face_L00_003E5978 *face;

    p = *(char **)(object + 0x68);
    quad.registers[1] = func_0027C878(D_L00_001B2B64);
    quad.registers[2] = 0xFF9000000260ULL;
    quad.registers[3] = (unsigned long long)D_L00_001B2B50_2 | ((unsigned long long)D_L00_001B2B54_2 << 2) | ((unsigned long long)D_L00_001B2B58 << 4) | ((unsigned long long)D_L00_001B2B5C << 6) | ((unsigned long long)D_L00_001B2B60_1 << 32);
    quad.registers[0] = 0;
    func_00282C88(m[0]);
    func_00282C88(m[1]);
    func_00282C88(m[2]);
    m[0][0] = *(float *)(p + 0x10);
    m[1][1] = *(float *)(p + 0x10);
    m[2][2] = *(float *)(p + 0x10) * 0.125f;
    quad.colors[0] = quad.colors[1] = quad.colors[2] = quad.colors[3] = D_L00_001B2B68 + (*(int *)(p + 0x24) << 24);
    func_002AFFD8(object + 0x10, m[3], 1);
    for (i = 0; i < 64; i++) {
        face = D_L00_00297020[i];
        vertex = quad.vertices[0];
        v = &quad.uv[0][1];
        u = &quad.uv[0][0];
        for (j = 3; j >= 0; j--) {
            k = face->texel;
            func_00282D30(vertex, D_L00_00296B20[face->corner], D_L00_001B2B6C);
            fv = D_L00_00297424[k][0];
            fu = D_L00_00297420[k][0];
            *u = fu;
            *v = fv;
            func_00283098(vertex, vertex, m);
            func_00283098(vertex, vertex, object + 0xC0);
            func_00282CC0(vertex, vertex, object + 0x10);
            func_00282F20(m[3], m[3], 0.5f);
            func_00282CC0(vertex, vertex, m[3]);
            face++;
            u += 2;
            v += 2;
            vertex += 4;
        }
        func_00280B90(&quad, 0, 0);
    }
}
