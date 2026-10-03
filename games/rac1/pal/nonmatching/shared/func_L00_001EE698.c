/* NON_MATCHING func_L00_001EE698 -- src/overlays/shared/effects_001EE2E0.c
 * Best so far: SIZE ours 916 / retail 928, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Point-in-collision-volume test (type at obj+0x10: 3 box, 5 sphere, 6 and 7 capsule-ish; switch order in retail
 *   Left: case 7 first test (retail: 'bc1t -> addiu v0,1' with fallthrough 'daddu v0,0' in the 2nd test; ours stor
 */
typedef int u128 __attribute__((mode(TI)));
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_001F9EC0(void *, void *, void *);
extern float func_001F9CE8(void *);
extern float func_001F9CB8(void *);
extern float func_001F9D10(void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern char *D_L00_001601AC MACRO_ADDR;
extern char *D_L00_001601BC MACRO_ADDR;
extern char *D_L00_001601B4 MACRO_ADDR;
extern char *D_L00_001601A4 MACRO_ADDR;

// Tests whether a point lies inside the shape of the given collision volume type.
int func_L00_001EE698(void *pos, char *obj, float r) {
    float p[4];
    float q[4];
    float d[4];
    float s[4];
    char *base;
    float *pp = p;
    int ret;
    *(u128 *)p = *(u128 *)pos;
    switch (*(int *)(obj + 0x10)) {
    case 3:
        base = D_L00_001601AC + (*(int *)(obj + 0x14) << 7);
        func_001F9BF0(d, p, base + 0x30);
        *(int *)&d[3] = 0;
        func_001F9EC0(s, d, base + 0x40);
        return s[0] >= -1.0f && s[0] <= 1.0f && s[1] >= -1.0f && s[1] <= 1.0f &&
               s[2] >= -1.0f && s[2] <= 1.0f;
    case 6:
        base = D_L00_001601BC + (*(int *)(obj + 0x14) << 7);
        ret = 0;
        func_001F9BF0(d, p, base + 0x30);
        *(int *)&d[3] = 0;
        func_001F9EC0(s, d, base + 0x40);
        if (func_001F9CE8(s) < 1.0f) {
            if (s[2] >= -1.0f && s[2] <= 1.0f) ret = 1;
        }
        return ret;
    case 5:
        base = D_L00_001601B4 + (*(int *)(obj + 0x14) << 7);
        r = func_001F9CB8(base) + r;
        if (func_001F9D10(p, base + 0x30) < r) return 1;
        return 0;
    case 7:
        base = D_L00_001601A4 + *(int *)(obj + 0x14) * 0x90;
        func_001F9BF0(d, p, base + 0x30);
        *(int *)&d[3] = 0;
        func_001F9EC0(s, d, base + 0x40);
        if (func_001F9CE8(s) < 1.0f) {
            if (s[2] >= -1.0f && s[2] <= 1.0f) return 1;
        }
        q[0] = 0;
        q[1] = 0;
        q[2] = 1.0f;
        q[3] = 1.0f;
        func_001F9EC0(s, q, base);
        func_001F9BD8(s, s, base + 0x30);
        func_001F9BF0(q, pp, s);
        if (func_001F9CB8(q) < *(float *)(base + 0x80)) return 1;
        q[3] = 1.0f;
        q[2] = -1.0f;
        *(int *)&q[0] = 0;
        *(int *)&q[1] = 0;
        func_001F9EC0(s, q, base);
        func_001F9BD8(s, s, base + 0x30);
        func_001F9BF0(q, pp, s);
        if (func_001F9CB8(q) < *(float *)(base + 0x80)) return 1;
        return 0;
    }
    return 0;
}
