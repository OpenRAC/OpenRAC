typedef int Quadword __attribute__((mode(TI)));
typedef struct { float position[4]; float original[4]; } EmitterOrigin;
extern float D_L02_001B0440[4] __attribute__((sda));
extern float D_L02_001B0438 __attribute__((sda));
extern float D_L02_001B0434 __attribute__((sda));
extern void func_0029FCC0(char *, int, char *);
extern void func_0029FC20(char *, int, char *);
extern void func_002830C0(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_L00_00325AE8(void *, int, float, float);

/* Emit at joint four after transforming the local offset into world space. */
void func_L02_003DAF90(char *object)
{
    EmitterOrigin origin;
    float matrix[16];
    float offset[4];
    char *state = *(char **)(*(char **)(object + 0xB8) + 0x68);

    func_0029FCC0(object, 4, (char *)origin.position);
    *(Quadword *)origin.original = *(Quadword *)origin.position;
    func_0029FC20(object, 4, (char *)matrix);
    func_002830C0(offset, D_L02_001B0440, matrix);
    func_00282CC0(origin.position, origin.position, offset);
    func_L00_00325AE8(origin.position, *(int *)(state + 0x250),
            D_L02_001B0438, D_L02_001B0434);
}
