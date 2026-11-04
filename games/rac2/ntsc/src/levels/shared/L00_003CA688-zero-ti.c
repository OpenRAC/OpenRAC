/* Builds one 16-byte record (a default value with two angles overwritten) and walks it through
   three helpers; the owner's second block supplies the radius. Needs -frac2-zero-ti-por. */
typedef int TI128 __attribute__((mode(TI)));
typedef union {
    TI128 q;
    float f[4];
} Quad_3CA688;

extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_L00_002EC608(void *, int);
extern void func_L00_002EC970(void *, char *, int, int, float, float);

void func_L00_003CA688(char *owner)
{
    Quad_3CA688 local;
    char *inner = *(char **)(owner + 0x68);

    local.q = 0;
    local.f[0] = -0.3f;
    local.f[3] = 1.0f;
    func_00283098(&local, &local, owner + 0xC0);
    func_00282CC0(&local, &local, owner + 0x10);
    func_L00_002EC608(&local, 0x103080);
    func_L00_002EC970(&local, inner + 0x10, 0x103080, 4, *(float *)(inner + 0x64), 0.25f);
}
