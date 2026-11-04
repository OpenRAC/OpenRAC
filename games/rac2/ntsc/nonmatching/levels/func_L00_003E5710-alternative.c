/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L00_003E5710: twin of func_L00_003E5978 (nonmatching/levels/func_L00_003E5978.c) with the
 * matrix diagonal 2*p[0x10], 2*p[0x10], p[0x10]*0.25. Not matching: same size and same call
 * sequence, about 30 instructions differ in register and stack-slot assignment: retail spills
 * object+0x10, object+0xC0, &uv[0][1], &uv[0][0] to 0xDC/0xD8/0xD4/0xD0($sp), keeps the loop
 * counter in $v1 with i+1 in $s7 (bnel with the next iteration's sll in the delay slot), and
 * builds the ALPHA word with loads in b,a,c,d,e order. Needs a -g8 file. Tried the same
 * variants as func_L00_003E5978 (statement order of the u/v stores, local order). */
typedef struct { float vertices[4][4]; unsigned int colors[4]; float uv[4][2]; unsigned long long registers[4]; } __attribute__((aligned(16))) BeamQuad_L00_003E5710;

typedef struct {
    short corner;
    short texel;
} Face_L00_003E5710;

typedef struct {
    float u;
    float v;
} Texel_L00_003E5710;

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
extern Face_L00_003E5710 D_L00_00297020[][4];
extern float D_L00_00297420[][2];
extern float D_L00_00297424[][2];

void func_L00_003E5710(char *object)
{
    BeamQuad_L00_003E5710 quad;
    float m[4][4] __attribute__((aligned(16)));
    char *p;
    int i, j, k;
    float fu, fv;
    float *vertex;
    float *u, *v;
    Face_L00_003E5710 *face;

    p = *(char **)(object + 0x68);
    quad.registers[1] = func_0027C878(D_L00_001B2B64);
    quad.registers[2] = 0xFF9000000260ULL;
    quad.registers[3] = (unsigned long long)D_L00_001B2B50_2 | ((unsigned long long)D_L00_001B2B54_2 << 2) | ((unsigned long long)D_L00_001B2B58 << 4) | ((unsigned long long)D_L00_001B2B5C << 6) | ((unsigned long long)D_L00_001B2B60_1 << 32);
    quad.registers[0] = 0;
    func_00282C88(m[0]);
    func_00282C88(m[1]);
    func_00282C88(m[2]);
    m[0][0] = *(float *)(p + 0x10) + *(float *)(p + 0x10);
    m[1][1] = *(float *)(p + 0x10) + *(float *)(p + 0x10);
    m[2][2] = *(float *)(p + 0x10) * 0.25f;
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
