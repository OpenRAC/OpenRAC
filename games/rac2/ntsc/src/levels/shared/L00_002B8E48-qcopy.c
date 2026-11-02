typedef struct { float x, y, z, w; } Vec4_L00_002B8E48;

typedef int V4_L00_002B8E48 __attribute__((mode(TI)));

extern float func_00282C10(float);

extern float func_00283248(float, float);

extern void func_00283410(void *, void *);

extern void func_00283698(void *, void *);

extern void func_002830C0(void *, void *, void *);

extern void *g_pHeroMoby;

void func_L00_002B8E48(Vec4_L00_002B8E48 *in, float *out)
{
    Vec4_L00_002B8E48 first[4];
    Vec4_L00_002B8E48 second[4];
    Vec4_L00_002B8E48 vector;
    Vec4_L00_002B8E48 *vp = &vector;

    *(volatile V4_L00_002B8E48 *)vp = *(V4_L00_002B8E48 *)in;
    func_00283410(first, (char *)g_pHeroMoby + 0xF0);
    func_00283698(second, first);
    func_002830C0(vp, vp, second);
    out[0] = func_00283248(func_00282C10(vector.x * vector.x + vector.z * vector.z), vector.y);
    out[1] = -func_00283248(vector.z, vector.x);
}
