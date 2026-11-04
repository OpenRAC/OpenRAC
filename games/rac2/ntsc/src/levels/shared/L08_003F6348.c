/* Aim-helper step: the shared func_002AB8C0 tail takes self + 0x50 on both paths. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;
typedef int s32;
typedef float f32;
typedef int TI128 __attribute__((mode(TI)));

extern char *func_002A9640(char *, int, int);
extern float func_002A96A0(char *, void *);
extern void func_L08_0032A4D0(char *, char *, char *, char *);
extern void func_L08_0032C0D8(char *, char *, int, int, int, float);
extern void func_002A7470(char *, int, int, int);
extern void func_002A6C58(char *, int, int);
extern void func_002AB800(char *, char *);
extern void func_002AB8C0(char *, char *);

void func_L08_003F6348(char *self, char *a1)
{
    char *s0 = a1;
    char *s2;
    struct { TI128 in; float out[4]; } l;
    float v;
    float f;

    if (*(u8 *)(self + 0x20) == 4)
        return;
    s2 = func_002A9640(self, 0xC30000, 0);
    f = func_002A96A0(self, s2);
    if (0.0f < f) {
        v = *(float *)(s0 + 0x20) - *(float *)(s2 + 0x2C);
        *(float *)(s0 + 0x20) = v;
        if (0.0f < v) {
            *(float *)(s0 + 0x78) = 0.13333334028720856f;
            *(float *)(s0 + 0x7C) = 0.11666667461395264f;
            l.in = *(TI128 *)(s2 + 0x10);
            func_L08_0032A4D0((char *)&l.in, (char *)l.out, s0 + 0x78, s0 + 0x7C);
            func_L08_0032C0D8(self, s0 + 0x60, 3, 1, 0, l.out[0]);
            if (1.0f <= f) {
                *(u8 *)(s0 + 0x57) = 0x78;
                *(u8 *)(self + 0x20) = 3;
                func_002A7470(self, 2, 0, 0);
            } else {
                *(u8 *)(s0 + 0x57) = 0x78;
            }
        } else {
            *(u16 *)(self + 0x34) &= 0xEFFF;
            *(u8 *)(s0 + 0x57) = 0xFA;
            *(u8 *)(self + 0x20) = 4;
            *(float *)(s0 + 0x78) = 0.18333333730697632f;
            *(float *)(s0 + 0x7C) = 0.11666667461395264f;
            l.in = *(TI128 *)(s2 + 0x10);
            func_L08_0032A4D0((char *)&l.in, (char *)(l.out + 1), s0 + 0x78, s0 + 0x7C);
            func_L08_0032C0D8(self, s0 + 0x60, 3, 1, 0, l.out[1]);
            *(float *)(s0 + 0xB0) = 5.0f;
            *(float *)(s0 + 0xB4) = 10.0f;
            if (*(u8 *)(self + 0x43) != 3)
                func_002A7470(self, 3, 0, 0);
            func_002A6C58(self, 0, 3);
        }
        func_002AB800(self, s0 + 0x50);
    }
    *(u8 *)(self + 0xA8) = 0xFF;
    func_002AB8C0(self, s0 + 0x50);
}

