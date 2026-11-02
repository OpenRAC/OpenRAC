typedef unsigned int Quadword __attribute__((mode(TI)));
typedef union { Quadword whole; float f[4]; } Vector;
extern float func_0029F530(char *);
extern float func_00283B98(float, float), func_00283BE0(float, float);
extern float func_00283180(float), func_00283198(float);
extern void func_00282CC0(void *, void *, void *), func_00282C88(void *);
extern void func_00282D08(void *, void *, void *, float);
extern void func_00335370(void *), func_003353C8(void *);
extern char *D_L01_001B264C __attribute__((sda));
extern struct { unsigned char prefix[0x80]; Vector position; unsigned char unused90[8]; float heading; } D_00189E20;

/* Blend the configured cameras, or orbit the player when either endpoint is absent. */
void func_L01_00416E78(char *object)
{
    int *state = *(int **)(object + 0x68);
    Vector position, rotation, first, second;
    float weight, heading, value;
    if (*(unsigned char *)(object + 0x42) == 1)
        value = func_0029F530(object);
    else
        value = 170.0f;
    weight = value * 0.0058823530562222003937f;
    if (state[1] == -1 || state[2] == -1) {
        heading = func_00283B98(weight * 0.69813179969787597656f - 0.34906589984893798828f, D_00189E20.heading);
        position.f[0] = func_00283180(heading) * 5.0f;
        position.f[1] = func_00283198(heading) * 5.0f;
        position.f[2] = 0;
        func_00282CC0(&position, &position, &D_00189E20.position);
        position.f[2] += 1.0f;
        func_00282C88(&rotation);
        rotation.f[2] = func_00283B98(heading, 3.141590118408203125f);
        rotation.f[1] = weight * -0.17000000178813934326f;
    } else {
        func_00282D08(&position, D_L01_001B264C + state[1] * 0x80 + 0x30, D_L01_001B264C + state[2] * 0x80 + 0x30, weight);
        first.whole = ((Vector *)(D_L01_001B264C + state[1] * 0x80 + 0x70))->whole;
        second.whole = ((Vector *)(D_L01_001B264C + state[2] * 0x80 + 0x70))->whole;
        rotation.f[2] = func_00283B98(func_00283BE0(second.f[2], first.f[2]) * weight, first.f[2]);
        rotation.f[1] = func_00283B98(func_00283BE0(second.f[1], first.f[1]) * weight, first.f[1]);
        rotation.f[0] = func_00283B98(func_00283BE0(second.f[0], first.f[0]) * weight, first.f[0]);
    }
    func_00335370(&position);
    func_003353C8(&rotation);
}
