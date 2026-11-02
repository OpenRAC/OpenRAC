/* A unit of its own for -frac2-keep-block-order (config/cflags.txt): retail keeps the
   frame-15 joint call behind the frame-5 one, reached by a jump. */

extern int func_L01_00321178(char *, float *);
extern float func_0029F530(char *);
extern int func_00283CF0(float);
extern float func_00283CE0(int);
extern float D_L11_002B6C10[];
extern void func_00282D30(char *, char *, float);
extern void func_00282CC0(char *, char *, char *);
extern float func_00283248(float, float);
extern int func_002B5278(char *, float);
extern void func_0029FCC0(char *, int, float *);
extern void func_002A7A00(float *, float, float);
extern float func_002A78D8(float, float);
extern float func_002A7878(float, float);
extern char *func_002BDCE8(float *, float, float, float, int, float *, float, int, int);

/* Steps along the vector at +0xC0, weighted by the walk table at the animation frame, and
   turns toward the target; on frames 5 and 15 it scatters ten dust particles at a joint
   (3, then 2). A state change hands over to the shared handler instead. */
void func_L11_00456070(char *obj, float *target)
{
    float offset[4], origin[4], half[4], velocity[4], pos[4];
    float frame, t, scale;
    int index, i, type, next;
    short lifetime;
    char *particle, *data;

    if (*(unsigned char *)(obj + 0x42) != *(unsigned char *)(obj + 0x43)) {
        func_L01_00321178(obj, target);
        return;
    }
    frame = func_0029F530(obj);
    index = func_00283CF0(frame);
    next = index + 1;
    t = func_00283CE0(index);
    func_00282D30((char *)offset, obj + 0xC0, D_L11_002B6C10[index] + (D_L11_002B6C10[next] - D_L11_002B6C10[index]) * (frame - t));
    func_00282CC0(obj + 0x10, (char *)offset, obj + 0x10);
    func_002B5278(obj, func_00283248(target[0] - *(float *)(obj + 0x10), target[1] - *(float *)(obj + 0x14)));
    if (index == 5 || index == 15) {
        if (index == 5) {
            func_0029FCC0(obj, 3, origin);
        } else {
            func_0029FCC0(obj, 2, origin);
        }
        func_00282D30((char *)half, obj + 0xC0, 0.5f);
        func_00282CC0((char *)origin, (char *)origin, (char *)half);
        origin[2] -= 0.2f;
        lifetime = 60;
        for (i = 9; i >= 0; i--) {
            func_002A7A00(velocity, 0.005000000353902578f, 0.050000004f);
            type = func_00283CF0(func_002A78D8(1.0f, 3.0f));
            scale = func_002A7878(0.3f, 1.0f) * 210000.0f;
            pos[0] = origin[0] + func_002A78D8(0, 0.3f);
            pos[1] = origin[1] + func_002A78D8(0, 0.3f);
            pos[2] = origin[2] + func_002A78D8(0, 0.3f);
            particle = func_002BDCE8(pos, 0, 1.0f, 1.01f, type, velocity, scale, 0x406080, 0);
            if (particle != 0) {
                *(short *)(particle + 0xA) = lifetime;
                particle[3] = 0x44;
                data = particle + 0x20;
                particle[9] = func_00283CF0(4.0f) + 0x40;
                *(float *)(data + 0x1C) = func_00283CE0(5);
                *(int *)(data + 4) = 3;
                data[0xA] = 0x20;
                data[0xB] = particle[0xA];
            }
        }
    }
}
