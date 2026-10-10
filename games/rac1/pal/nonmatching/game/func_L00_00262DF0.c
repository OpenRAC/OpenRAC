extern char *D_L00_001B0830[];
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9C78(void *, void *);
extern float func_001F9CB8(void *);
extern float func_001F9B88(float);
extern void func_L00_001FF4B0(void *, void *, float);

typedef u32 vec128_262DF0 __attribute__((mode(TI), aligned(16)));

/* Snaps a point onto path idx: finds the nearest segment within dist of *in, moves the point onto
   the path (keeping its own z) and stores it in *out. Returns 1 when a point was found. */
int func_L00_00262DF0(int idx, void *in, void *out, float dist) {
    vec128_262DF0 query, a, o, d, dn, cr, sc;
    char *path;
    char *pb;
    int count;
    float len_d;
    float dot;
    int found;
    int i;

    query = *(vec128_262DF0 *)in;
    found = 0;
    count = *(int *)D_L00_001B0830[idx];
    for (i = 0; i < count - 1; i++) {
        path = D_L00_001B0830[idx];
        pb = path + (i << 4);
        if (*(float *)(pb + 0x1C) == 0.0f && *(float *)(pb + 0x2C) == 0.0f) {
            continue;
        }
        func_001F9BF0(&a, &query, pb + 0x10);
        *(int *)((char *)&a + 8) = 0;
        func_001F9BF0(&d, pb + 0x20, pb + 0x10);
        *(int *)((char *)&d + 8) = 0;
        func_L00_001FF4B0(&dn, &d, 1.0f);
        func_001F9CA0(&cr, &a, &dn);
        if (dist < func_001F9B88(*(float *)((char *)&cr + 8))) {
            continue;
        }
        len_d = func_001F9CB8(&d);
        dot = func_001F9C78(&a, &dn);
        if (len_d < dot || dot < 0.0f) {
            if (func_001F9CB8(&a) < dist) {
                found = 1;
                func_L00_001FF4B0(&o, &a, dist);
                func_001F9BD8(&query, &o, pb + 0x10);
                *(float *)((char *)&query + 8) = *(float *)((char *)in + 8);
            }
        } else {
            found = 1;
            func_L00_001FF4B0(&sc, &dn, dot);
            func_001F9BF0(&o, &a, &sc);
            func_L00_001FF4B0(&o, &o, dist);
            func_001F9BD8(&o, &o, &sc);
            func_001F9BD8(&query, &o, pb + 0x10);
            *(float *)((char *)&query + 8) = *(float *)((char *)in + 8);
        }
    }
    if (found) {
        *(vec128_262DF0 *)out = query;
    }
    return found;
}
