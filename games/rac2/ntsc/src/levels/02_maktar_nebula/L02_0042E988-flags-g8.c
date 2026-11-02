
typedef unsigned char u8;
typedef int Quadword __attribute__((mode(TI)));
extern u8 *D_L02_001B359C;
extern int D_L02_001B30C8;
extern int D_L02_001B2AAC __attribute__((sda));
extern float D_L02_001B2AA0 __attribute__((sda));
extern float D_L02_001B2AA4 __attribute__((sda));
extern float D_L02_001B2AA8 __attribute__((sda));
extern float D_L02_001B2AB0 __attribute__((sda));
extern int func_L02_00400B00(u8 *);
extern int func_00283DB8(float *, float);
extern void func_L02_0042ED48(u8 *);
extern int func_002B3868(u8 *);
extern int func_002A77E0(int);
extern void func_002A7470(u8 *, int, int, int);
extern void func_002A7750(u8 *, int, int);
extern float func_00283248(float, float);
extern float func_00283C80(float, float);
extern float func_00283B98(float, float);

/* Hand-adapted from func_L11_00474920. */
extern float ref_L02_0042E988_00 __attribute__((sda));
extern float ref_L02_0042E988_01 __attribute__((sda));
extern float ref_L02_0042E988_02 __attribute__((sda));
extern int ref_L02_0042E988_03 __attribute__((sda));
extern float ref_L02_0042E988_04 __attribute__((sda));
void func_L02_0042E988(u8 *object) {
    float bounds[4];
    float center[4];
    int *state = *(int **)(object + 0x68);
    u8 *target;
    float level, angle, heading, delta;
    int animation;
    if (!func_L02_00400B00(D_L02_001B359C + *state * 0x100)) return;
    *(Quadword *)center = 0;
    center[0] = *(float *)(object + 0x10);
    center[1] = *(float *)(object + 0x14);
    center[2] = *(float *)(object + 0x18);
    center[3] = 5.0f;
    *(Quadword *)bounds = *(Quadword *)center;
    if (func_00283DB8(bounds, 255.0f) <= 0) {
        *(unsigned short *)(object + 0x32) = 0;
        return;
    }
    *(unsigned short *)(object + 0x32) = 255;
    func_L02_0042ED48(object);
    target = (u8 *)(*state * 0x100 + (int)D_L02_001B359C);
    level = *(float *)(*(u8 **)(target + 0x68) + 0x6C);
    switch (object[0x20]) {
    case 1:
        if (func_002B3868(object) && object[0x43] != func_002A77E0(2)) {
            animation = func_002A77E0(2);
            func_002A7470(object, animation, 0, 10);
        }
        if (level > ref_L02_0042E988_00 && !func_002A77E0(60)) {
            func_002A7750(object, 2, -1);
        } else if (level < ref_L02_0042E988_01 && !func_002A77E0(60)) {
            func_002A7750(object, 3, -1);
        }
        break;
    case 2:
        if (func_002B3868(object)) {
            if (object[0x43] != func_002A77E0(2) + 2) {
                animation = func_002A77E0(2);
                func_002A7470(object, animation + 2, 0, 10);
            }
            return;
        }
        if (level < ref_L02_0042E988_02 && !func_002A77E0(60)) func_002A7750(object, 1, -1);
        break;
    case 3:
        angle = ((float)(D_L02_001B30C8 % ref_L02_0042E988_03) * -6.2831855f) / (float)ref_L02_0042E988_03 + 3.1415927f;
        {
            center[1] = 600.0f;
            center[0] = 500.0f;
            center[2] = 500.0f;
            center[3] = 1.0f;
            delta = *(float *)(object + 0x10) - center[0];
            heading = func_00283248(delta, *(float *)(object + 0x14) - center[1]);
        }
        if (func_00283C80(heading, angle) < ref_L02_0042E988_04 ||
            func_00283C80(heading, func_00283B98(angle, 3.1415927f)) < ref_L02_0042E988_04) {
            if (object[0x43] != 4) func_002A7470(object, 4, 0, 10);
        } else if ((object[0x60] & 2) && object[0x43] != func_002A77E0(2)) {
            animation = func_002A77E0(2);
            func_002A7470(object, animation, 0, 10);
        }
        if (level > ref_L02_0042E988_02 && !func_002A77E0(60)) func_002A7750(object, 1, -1);
        break;
    }
}
