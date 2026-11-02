/* func_002AE838: normalises a byte-colour vector: subtracts (127, 127, 127, 0) from the
 * vector func_002830F0 builds and scales it by w * 0.0001 into the first argument.
 * func_00282D30 takes (dst, src, scale), and the scale is read before the subtraction,
 * as in retail. The (127, 127, 127, 0) vector is written through its float fields, so
 * its address is rebuilt at the call instead of being held in a saved register. */
typedef __attribute__((mode(TI))) int TI;

extern void func_002830F0(void *dst, float *src);
extern void func_00282CF0(void *dst, void *a, void *b);
extern void func_00282D30(void *dst, void *src, float scale);

void func_002AE838(void *a0, void *a1)
{
    struct { float x, y, z, w; } v;
    union { TI q; float f[4]; } w;
    float scale;

    w.q = 0;
    w.f[0] = 127.0f;
    w.f[1] = 127.0f;
    w.f[2] = 127.0f;
    func_002830F0(&v, *(float **)a1);
    scale = v.w * 0.0001f;
    func_00282CF0(&v, &v, &w);
    func_00282D30(a0, &v, scale);
}
