extern float func_00283CE0(int);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002761F8(void *, void *, void *, void *, void *);

typedef struct {
    float x, y, z, w;
} V4;

int func_L00_0031C3D0(char *a0, char *a1, char *a2, char *a3, void *a4, void *a5, int a6)
{
    V4 local0, local1, local2, local3;
    float fraction;
    float step;
    int count = 0;

    step = 1.0f / func_00283CE0(a6);
    func_00282CF0(&local0, a0, a2);
    func_00282CF0(&local1, a1, a3);
    fraction = step;
    while (count < a6) {
        func_00282D30(&local2, &local0, fraction);
        func_00282D30(&local3, &local1, fraction);
        func_00282CC0(&local2, &local2, a2);
        func_00282CC0(&local3, &local3, a3);
        if (func_002761F8(&local2, &local3, 0, a4, a5) != 0)
            return 1;
        count++;
        fraction += step;
    }
    return 0;
}
