#include "include_asm.h"
#include "common.h"

/* func_L00_003DC2D0 is matched in its own unit, src/levels/shared/L00_003DC2D0.c (default flags). */

extern int D_L00_001B043C __attribute__((sda));

extern int D_L00_001B2B44 __attribute__((sda));

int func_L00_003DE260(void)
{
    char *p = (char *)D_L00_001B043C;

    if (p == 0)
        return 0;
    return (unsigned int)((unsigned char)p[0x20] - 5) < 3;
}

int func_L00_003DF108(void)
{
    if (D_L00_001B043C == 0)
        return 0;
    return D_L00_001B2B44;
}

extern void func_L00_003B8138(void);

/* Writes the object's state byte (7 when arg0 is zero, 5 otherwise), lets the shared
 * updater run, then writes the caller's value in the object's slot - 10 when it is -1. */
void func_L00_003DF6B0(int arg0, int arg1)
{
    char *p = (char *)D_L00_001B043C;
    int a = arg1;
    char *q;

    if (p == 0)
        return;
    q = *(char **)(p + 0x68);
    if (arg0 != 0)
        p[0x20] = 5;
    else
        p[0x20] = 7;
    func_L00_003B8138();
    if (a != -1)
        *(int *)(q + 0x20) = a;
    else
        *(int *)(q + 0x20) = 10;
}

extern int D_L00_001B19C8[] __attribute__((sda));

extern char *D_L00_001B1EA8[] __attribute__((sda));

extern float D_L00_001B0430 __attribute__((sda));

extern float func_002EE2B0(char *a, char *b);

extern void func_00381F80(char *a, char *b);

/* walks the object list once the shared flag is set, stopping at either special state and
   otherwise acting on the entries whose handle reports type 0x13 */
void func_L00_003DF030(char *arg)
{
    char *o;

    if ((D_L00_001B19C8[0] & 1) == 0)
        return;

    for (o = D_L00_001B1EA8[0]; o != 0; o = *(char **)(o + 40)) {
        unsigned char state = *(unsigned char *)(o + 32);

        if (state == 0xFE)
            return;
        if (state == 0xFD)
            return;
        if (o == 0 || *(int *)(o + 36) == 0 || *(short *)(*(int *)(o + 36) + 70) != 0x13)
            continue;
        if (func_002EE2B0(o + 16, arg + 16) < D_L00_001B0430)
            func_00381F80(o, arg);
    }
}

/* A moby's update distance (0x30) and draw distance (0x32), as in RAC1's moby layout. */
struct BFObject {
    char pad[0x30];
    unsigned char field_30;
    unsigned char field_31;
    unsigned short field_32;
};

extern char *ref_L00_003DBF50_00(int);
extern float ref_L00_003DBF50_01(void);
extern float ref_L00_003DBF50_02(float, float);
extern int ref_L00_003DBF50_03 SDATA(ref_L00_003DBF50_03);
extern char *ref_L00_003DBF50_04;
extern char *ref_L00_003DBF50_05;
extern char *ref_L00_003DBF50_06;
extern char *ref_L00_003DBF50_07(char *);
extern void ref_L00_003DBF50_08(void);
/* The resident block at 0x189E20 (hero position at +0x80, hero moby at +0x2290). */
extern struct BFPlayer { char pad[0xbf8]; char *target; } D_00189E20 NOT_SDA;
extern float ref_L00_003DBF50_10(char *);
extern void ref_L00_003DBF50_11(char *, char *, char *, float);
extern void ref_L00_003DBF50_12(char *, char *);
extern float ref_L00_003DBF50_13 SDATA(ref_L00_003DBF50_13);
extern float ref_L00_003DBF50_14 SDATA(ref_L00_003DBF50_14);
extern void ref_L00_003DBF50_15(void *, void *, float);
extern int ref_L00_003DBF50_16(char *, float, float, float, int);

/* Spawns moby class 0xB2E at pos for parent and sets its state up (the three shared buffers
 * on first use), hands dir to the target record at D_00189E20 + 0xBF8 when there is one,
 * and passes dir with a random spread and a random speed through ref_L00_003DBF50_15. */
char *func_L00_003DBF50(char *parent, int arg, char *pos, char *dir)
{
    float velocity[4];
    char *object = ref_L00_003DBF50_00(0xb2e);
    char *state;
    float length;

    if (object) {
        object[0x20] = 0;
        state = *(char **)(object + 0x68);
        ((struct BFObject *)object)->field_30 = 255;
        ((struct BFObject *)object)->field_32 = 255;
        *(float *)(state + 0x64) = ref_L00_003DBF50_01();
        *(float *)(state + 0x68) = ref_L00_003DBF50_02(0.3723369836807251f, 0.6283186078071594f);
        *(int *)(state + 0x74) = *(short *)(parent + 0xaa);
        *(int *)(state + 0x7c) = 0;
        qcopy(object + 0x10, pos);
        *(float *)(object + 0x2c) = ref_L00_003DBF50_02(0.01f, 0.0125f);
        *(unsigned short *)(object + 0x34) |= 0x100;
        *(int *)(object + 0x78) = 0x7f4080ff;
        *(int *)(state + 0x70) = arg;
        {
            char *parentState = *(char **)(parent + 0x68);

            if (!ref_L00_003DBF50_03) {
                char *p = *(char **)(parentState + 8);

                ref_L00_003DBF50_04 = p;
                p = ref_L00_003DBF50_07(p + 0x100);
                ref_L00_003DBF50_05 = p;
                p = ref_L00_003DBF50_07(p + 0x4b00);
                ref_L00_003DBF50_06 = p;
                ref_L00_003DBF50_07(p + 0xa0);
                ref_L00_003DBF50_08();
                ref_L00_003DBF50_03 = 1;
            }
        }
        if (D_00189E20.target) {
            length = ref_L00_003DBF50_10(dir);
            if (length == 0.0f)
                length = 1.0f;
            ref_L00_003DBF50_11(D_00189E20.target, pos, dir, length);
            qcopy(state, dir);
        } else {
            ref_L00_003DBF50_12(object, state);
        }
        qcopy(velocity, dir);
        velocity[0] += ref_L00_003DBF50_02(-0.15f, 0.15f);
        velocity[1] += ref_L00_003DBF50_02(-0.15f, 0.15f);
        length = ref_L00_003DBF50_02(ref_L00_003DBF50_13 * (1.0f / 60.0f),
                                     ref_L00_003DBF50_14 * (1.0f / 60.0f));
        ref_L00_003DBF50_15(velocity, velocity, length * 0.5f);
        *(int *)(state + 0x6c) = 96;
        *(int *)(state + 0x60) = 680;
        *(int *)(state + 0x78) = ref_L00_003DBF50_16(object, 1.0f, 1.0f, 1.0f, 0);
    }
    return object;
}
