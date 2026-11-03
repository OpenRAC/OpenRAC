/*
 * GPT-6.1 Sol, high effort, task v1315 (29 October 2026).
 * Retained near miss: 2012 compiled bytes versus 2028 retail bytes; 0/27 USA
 * v1.01 placements match. Last check: tools/check.py func_L00_003C6298 --diff
 * through /workspace/rac2-orch/v2/nx.sh. Eight complete-C compiles plus one stub.
 * Required profile: -O2 -G8 -ffunction-sections -frac2-zero-ti-por.
 *
 * The 0xC0 frame and vector/matrix scratch offsets agree. Remaining differences
 * include the initial special flag (sltiu versus retail's constant-one/movn),
 * absolute versus GP macro accesses, and scalar light-coordinate scheduling and
 * registers. Equivalent boolean forms tied; stop before further wordsmithing.
 * A float tick initialized before the upgrade-selection branches preserves the
 * retail 1.0f * 0.0166666675f multiplication; a constant expression folds it, and
 * an integer tick introduces an unwanted cvt.s.w. The retained declarations are
 * below. Data bindings and the target-selector ref have not passed a linked build.
 * The selector at 003C5B88 is a level-00-only catalogue identity, so callers in
 * all 27 levels need ref_L00_003C6298_00 rather than its canonical function name.
 * Next: investigate the entry conditional-move source shape and macro addressing
 * with allocator evidence; then refine light-coordinate stores. No full build.
 */
#include "common.h"

#define FIELD(p, type, offset) (*(type *)((char *)(p) + (offset)))
typedef unsigned int Quad __attribute__((mode(TI)));
typedef union { Quad q; float f[4]; } Vec;
typedef struct { unsigned short count, pad; unsigned int maximum, levels; } Statistics;
typedef struct { char pad[0x10]; Vec position; } Light;
typedef struct { char pad[0xE0]; } Weapon;
typedef struct {
    char pad000[0x730]; Vec matrix[3];
    char pad760[0xAD0]; int input;
    char pad1234[0x10]; int weapon;
    char pad1248[0x1048]; char *object;
    int state, field2298, mode;
    char pad22A0[0x18]; unsigned char flag;
    char pad22B9[0x233]; short aiming;
    char pad24EE[0x46]; int timer;
} Hero;
extern Hero D_00189E20;
extern Statistics D_0018C360[];
extern unsigned int D_001A7390 MACRO_ADDR;
extern int D_001A79F0 MACRO_ADDR;
extern unsigned int D_00138320[] MACRO_ADDR;
extern unsigned char D_00139578 NOT_SDA;
extern Weapon D_L00_00262570[];
extern int D_L00_001B2A0C[] MACRO_ADDR;
extern int D_L00_001B2A04[] MACRO_ADDR;
extern int D_L00_001B2A08[] MACRO_ADDR;
extern int D_L00_001B2A00 __attribute__((sda));
extern int D_L00_001B2A18 __attribute__((sda));
extern int D_L00_001B2A1C __attribute__((sda));
extern int D_L00_001AFFAC __attribute__((sda));
extern float D_L00_001AFFA8 __attribute__((sda));
extern float D_L00_001AFFC8 __attribute__((sda));
extern float D_L00_001AFFBC[] MACRO_ADDR;
extern float D_L00_001AFFB8 __attribute__((sda));
extern int D_L00_001AFFB0 __attribute__((sda));
extern int D_L00_001AFFB4[] __attribute__((sda));
extern int D_L00_001B0E30[] __attribute__((sda));
extern unsigned char D_001A7A76[] MACRO_ADDR;
extern int D_L00_001A903C[] MACRO_ADDR;
extern Light D_L00_001C6F40[];
typedef struct { char prefix[0x74]; unsigned char active; char pad75[0x13]; char *owner; } SoundView;
extern char D_00188660[];
extern Vec D_L00_001B8C80[];
extern int func_00282978(void *);
extern int func_00282948(void *);
extern void func_002A7470(void *, int, int, int);
extern void func_L00_002AF568(void);
extern void func_L00_002AF7D0(void);
extern void func_0029FCC0(void *, int, void *);
extern void func_L00_002C42E0(int, int);
extern void func_00283678(void *, void *);
extern void func_00283698(void *, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00283098(void *, void *, void *);
extern float func_00283248(float, float);
extern float func_00282E20(void *);
extern char *ref_L00_003C6298_00(void *, void *, void *, void *);
extern char *func_002AB1F0(void *);
extern float func_00283CE0(int);
extern float func_002AA140(float, float, float);
extern int func_L00_00325FC8(float, void *, int, int);
extern void func_00290010(int, int, int, float);
extern void func_002E5E58(int, int, int);
extern void func_002B0A18(int, int);
extern int func_002E5CF8(int, int, void *);
extern void func_L00_00403730(void *);
extern void func_L00_003C6B98(void *);
extern int func_00291308(float, void *, int, float);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern int func_002E59B0(void *, int);
extern void func_00291450(int);
extern void func_002E59F8(int);
extern void func_002A75E0(void *, int, int, int, int);

void func_L00_003C6298(char *owner)
{
    Vec direction;
    union {
        struct { Vec matrix[3], inverse[4]; } aim;
        struct { Vec offset, point; } light;
    } work;
    char *context = FIELD(owner, char *, 0x68);
    int special;
    int allowed;
    char *target, *definition;
    char *weapon;
    int upgraded;
    float rate, scale;
    Light *light;

    special = 1;
    if (D_00189E20.weapon != 3) special = 0;
    if (!FIELD(owner, unsigned char, 0x20)) {
        FIELD(owner, unsigned char, 0x20) = 1;
        D_L00_001B2A0C[0] = -1;
        D_L00_001B2A04[0] = -1;
        D_L00_001B2A08[0] = 0;
    }
    func_00282978(context + 0x1C);
    allowed = 1;
    if (D_00189E20.state == 29) {
        if (!func_00282948(&D_00189E20.timer)) allowed = 0;
    } else D_00189E20.timer = 40;

    if ((unsigned int)D_00189E20.mode < 2 &&
        (D_00189E20.input & D_00138320[0]) && allowed) {
        if (FIELD(owner, unsigned char, 0x43) != 2)
            func_002A7470(owner, 2, 0, 0);
        if (!D_00189E20.flag) func_L00_002AF568();
        if (!FIELD(context, short, 0x1E)) {
            FIELD(context, short, 0x1E) = 1;
            if (D_0018C360[16].count < 65535) D_0018C360[16].count++;
            if (D_0018C360[16].maximum < D_001A7390) D_0018C360[16].maximum = D_001A7390;
            D_0018C360[16].levels = (D_0018C360[16].levels | (1u << D_001A79F0)) | 0x80000000u;
        }
        if (!FIELD(context, short, 0x1C)) FIELD(context, short, 0x1C) = 15;
        func_0029FCC0(owner, 0, context);
        if (D_00189E20.state == 1) func_L00_002C42E0(29, 1);
        direction.q = 0;
        if (D_00189E20.state == 29 &&
            (FIELD(D_00189E20.object, unsigned short, 0x34) & 1)) {
            func_00283678(work.aim.matrix, D_L00_001B8C80);
            func_00283698(work.aim.inverse, work.aim.matrix);
            direction.f[2] = 1.0f;
            func_002830C0(&direction, &direction, work.aim.inverse);
        } else if (D_00189E20.aiming == 1) {
            func_00282D30(&direction, owner + 0xD0, -1.0f);
        } else {
            direction.f[0] = 1.0f;
            func_00283098(&direction, &direction, D_00189E20.matrix);
        }
        FIELD(context, float, 0x48) = func_00283248(direction.f[0], direction.f[1]);
        FIELD(context, float, 0x4C) = func_00283248(func_00282E20(&direction), direction.f[2]);
        work.aim.matrix[0].q = FIELD(context, Vec, 0).q;
        target = ref_L00_003C6298_00(owner, work.aim.matrix, context + 0x48, context + 0x4C);
        func_00282948(context + 0x44);
        if (FIELD(context, char *, 0x10)) {
            unsigned char state = FIELD(FIELD(context, char *, 0x10), unsigned char, 0x20);
            if (state == 254) goto clear_target;
            if (state == 253) {
clear_target:
                FIELD(context, char *, 0x10) = 0;
                FIELD(context, int, 0x44) = 0;
            }
        }
        if (!target) {
            if (!FIELD(context, int, 0x44)) goto no_target;
        } else {
            definition = func_002AB1F0(target);
            if (target != FIELD(context, char *, 0x10) && !FIELD(context, int, 0x44)) {
                FIELD(context, char *, 0x10) = target;
                FIELD(context, float, 0x14) = func_00283CE0(FIELD(definition, short, 4));
                FIELD(context, float, 0x18) = FIELD(definition, float, 0);
                FIELD(context, int, 0x44) = 30;
                if (FIELD(context, float, 0x14) <= 1.0f) FIELD(context, int, 0x18) = 0;
            }
        }
        {
            float ticks = 1.0f;
            weapon = (char *)&D_L00_00262570[D_00139578];
            upgraded = 0;
            if (FIELD(weapon, int, 0)) upgraded = FIELD(weapon, unsigned char, 4) == 2;
            if (upgraded) D_L00_001AFFA8 = 35.0f;
            else D_L00_001AFFA8 = 10.0f;
            scale = ticks * 0.0166666675f;
            FIELD(context, float, 0x18) -= D_L00_001AFFA8 * scale;
            if (FIELD(context, float, 0x18) < 0.0f) FIELD(context, float, 0x18) = 0.0f;
            D_L00_001AFFC8 = FIELD(context, float, 0x18) / FIELD(context, float, 0x14);
            goto have_target;
        }
no_target:
        D_L00_001AFFC8 = 1.0f;
have_target:
        rate = func_002AA140(D_L00_001AFFB8, D_L00_001AFFBC[0], D_L00_001AFFC8);
        func_00290010(1, func_L00_00325FC8(rate * 0.0002908882452175021f, context + 0x50, D_L00_001AFFB0, D_L00_001AFFB4[0]), 0, D_L00_001AFFC8);
        if (FIELD(context, char *, 0x10) && FIELD(context, float, 0x18) <= 0.0f) {
            int target_class = FIELD(FIELD(context, char *, 0x10), short, 0xAA);
            if (D_001A79F0 == 12 && target_class == 0xFB4) {
                D_L00_001B0E30[0] = D_L00_001B0E30[0] + 1;
                if (D_L00_001B0E30[0] >= 16 && !D_001A7A76[0]) {
                    D_001A7A76[0] = 1;
                    func_002E5E58(1, 0, 0);
                    func_002B0A18(0x1233, -1);
                    D_L00_001A903C[0] = 1;
                }
            }
            func_002E5CF8(3, 0, owner);
            func_L00_00403730(FIELD(context, char *, 0x10));
            D_L00_001AFFAC++;
            FIELD(context, char *, 0x10) = 0;
        }
        func_L00_003C6B98(owner);
        if (D_L00_001B2A04[0] == -1)
            D_L00_001B2A04[0] = func_00291308(7.5f, owner + 0x10, 0x207F7F, 0.0f);
        if (D_L00_001B2A04[0] >= 0) {
            float light_x, light_y, light_z;
            D_L00_001B2A08[0] = 20;
            func_00282F20(&work.light.offset, owner + 0xD0, -0.5f);
            func_00282CC0(&work.light.point, context, &work.light.offset);
            light_x = work.light.point.f[0];
            light_y = work.light.point.f[1];
            light_z = work.light.point.f[2];
            light = &D_L00_001C6F40[D_L00_001B2A04[0]];
            light->position.f[0] = light_x;
            light->position.f[3] = 7.5f;
            light->position.f[1] = light_y;
            light->position.f[2] = light_z;
        }
        if (!func_002E59B0(owner, D_L00_001B2A0C[0]))
            D_L00_001B2A1C = func_002E5CF8(1, 4, owner);
    } else {
        FIELD(context, char *, 0x10) = 0;
        FIELD(context, short, 0x1E) = 0;
        FIELD(context, int, 0x44) = 0;
        D_L00_001B2A00 = 1;
        if (D_L00_001B2A04[0] >= 0) {
            float light_x2, light_y2, light_z2;
            func_00282F20(&direction, owner + 0xD0, -0.5f);
            func_00282CC0(&work.light.offset, context, &direction);
            light_x2 = work.light.offset.f[0];
            light_y2 = work.light.offset.f[1];
            light_z2 = work.light.offset.f[2];
            light = &D_L00_001C6F40[D_L00_001B2A04[0]];
            light->position.f[1] = light_y2;
            light->position.f[2] = light_z2;
            light->position.f[3] = 7.5f;
            light->position.f[0] = light_x2;
            rate = func_00283CE0(D_L00_001B2A08[0]) * 0.05f;
            light = &D_L00_001C6F40[D_L00_001B2A04[0]];
            FIELD(light, float, 0) = rate;
            FIELD(light, float, 4) = rate;
            FIELD(light, float, 8) = rate * 0.25f;
            if (func_00282948(D_L00_001B2A08)) {
                func_00291450(D_L00_001B2A04[0]);
                D_L00_001B2A04[0] = -1;
            }
        }
        if (D_L00_001B2A04[0] >= 0) {
            if (D_L00_001B2A18) goto sound_cleanup;
            func_00291450(D_L00_001B2A04[0]);
            D_L00_001B2A04[0] = -1;
        }
sound_cleanup:
        if (D_L00_001B2A0C[0] >= 0) {
            int sound = D_L00_001B2A0C[0];
            if (sound != -1) {
                SoundView *entry = (SoundView *)(D_00188660 + sound * 0x70);
                if (entry->owner == owner && entry->active) func_002E59F8(sound);
            }
            D_L00_001B2A0C[0] = -1;
        }
        if (FIELD(owner, unsigned char, 0x60) & 2) func_002A75E0(owner, 1, 0, 1, 0);
        func_L00_002AF7D0();
    }
    if (special && D_L00_001B2A04[0] >= 0) {
        func_00291450(D_L00_001B2A04[0]);
        D_L00_001B2A04[0] = -1;
    }
}
