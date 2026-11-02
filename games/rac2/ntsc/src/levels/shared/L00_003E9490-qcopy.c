typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vec16_Fc936841d;

extern void func_00282CF0(char *local, char *first, char *second);
extern void func_00282D30(char *a, char *b, float value);
extern void func_00282CC0(char *target, char *first, char *local);

void func_L00_003E9490(float t, Vec16_Fc936841d *dst, Vec16_Fc936841d *src, Vec16_Fc936841d *other)
{
    Vec16_Fc936841d local;

    if (t == 0.0f) {
        *(TI128 *)dst = *(TI128 *)src;
    } else if (t == 1.0f) {
        *(TI128 *)dst = *(TI128 *)other;
    } else {
        func_00282CF0((char *)&local, (char *)other, (char *)src);
        func_00282D30((char *)&local, (char *)&local, t);
        func_00282CC0((char *)dst, (char *)src, (char *)&local);
    }
}
