extern void func_00282CC0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern float func_00283248(float, float);
extern float func_00283CE0(int);
extern int func_00283CF0(float);
extern float func_0029F530(void *);
extern void func_0029FCC0(void *, int, void *);
extern float func_002A7878(float, float);
extern float func_002A78D8(float, float);
extern void func_002A7A00(void *, float, float);
extern float func_002A89A8(void *, int, float);
extern int func_002B5278(void *, float);
extern char *func_002BDCE8(void *, int, float, float, void *, float, int, int, float);
extern void func_L01_00321178(void *, void *);
extern float D_L04_002AF630[];

void func_L04_003E5B10(char *o, float *dest)
{
    float v[4], pos[4], vel[4], random[4], spawn[4];
    float frame, t, color;
    int index, i, kind;
    unsigned short lifetime;
    char *particle, *data;

    if ((unsigned char)o[0x42] != (unsigned char)o[0x43]) {
        func_L01_00321178(o, dest);
        return;
    }
    frame = func_0029F530(o);
    index = func_00283CF0(frame);
    {
        int nextIndex = index + 1;
        t = func_00283CE0(index);
        frame -= t;
        func_00282D30(v, o + 0xc0,
                      D_L04_002AF630[index] + (D_L04_002AF630[nextIndex] - D_L04_002AF630[index]) * frame);
    }
    func_00282CC0(o + 0x10, v, o + 0x10);
    func_002B5278(o, func_00283248(dest[0] - *(float *)(o + 0x10), dest[1] - *(float *)(o + 0x14)));
    *(float *)(o + 0x18) = func_002A89A8(o + 0x10, 0, 0.5f);
    if (index != 5 && index != 15) {
        return;
    }
    if (index == 5) {
        func_0029FCC0(o, 3, pos);
    } else {
        func_0029FCC0(o, 2, pos);
    }
    func_00282D30(vel, o + 0xc0, 0.5f);
    func_00282CC0(pos, pos, vel);
    pos[2] -= 0.2f;
    lifetime = 60;
    for (i = 9; i >= 0; i--) {
        func_002A7A00(random, 0.005000000353902578f, 0.050000004f);
        kind = func_00283CF0(func_002A78D8(1.0f, 3.0f));
        color = func_002A7878(0.3f, 1.0f) * 210000.0f;
        spawn[0] = pos[0] + func_002A78D8(0.0f, 0.3f);
        spawn[1] = pos[1] + func_002A78D8(0.0f, 0.3f);
        spawn[2] = pos[2] + func_002A78D8(0.0f, 0.3f);
        particle = func_002BDCE8(spawn, kind, 0.0f, 1.0f, random, 1.01f, 0x406080, 0, color);
        if (particle) {
            *(unsigned short *)(particle + 0xa) = lifetime;
            data = particle + 0x20;
            particle[3] = 0x44;
            particle[9] = func_00283CF0(4.0f) + 0x40;
            *(float *)(data + 0x1c) = func_00283CE0(5);
            *(int *)(data + 4) = 3;
            data[0xa] = 0x20;
            data[0xb] = particle[0xa];
        }
    }
}
