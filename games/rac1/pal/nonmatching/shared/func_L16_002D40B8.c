/* NON_MATCHING func_L16_002D40B8 -- src/overlays/shared/vendor_002A1B58.c
 * Best so far: SIZE ours 3004 / retail 3008, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   3. SIZE 3004/3008: direct animation calls restored all transition blocks. Three small control/scheduling diffe
 *   4. BYTES199/3008: explicit arrived label fixes distance scheduling and size. Most bytes are swapped moby/data 
 *   5. COMPILE: typed-field rewrite incorrectly replaced offset prefixes 0x30*; p5 corrects these names without ch
 *   6. BYTES197/3008: typed data preserves shape; plain event global fixed its high-register allocation. Next move
 *   7. BYTES197/3008 unchanged: moving moby assignment after damage and scalar flag alias had no codegen effect. N
 *   8. BYTES197/3008 unchanged: typed moby fields leave allocation unchanged. Next independent fix uses resident M
 *   9. BYTES193/3008: resident MACRO_ADDR fixes high/low pointer registers; explicit temporary did not change deat
 *   10. BYTES193/3008 unchanged: direct qcopy and separate class labels do not alter allocator. Saved best p8; pau
 */
#include "common.h"
extern void func_L16_002D4C78(char *);
extern void func_L00_00264BB0(void *, float);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898(float);
extern int func_002140B0(int);
extern int func_001F9850(int);
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA748(float, float);
extern int func_001F9938(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L16_002D5280_update(char *, void *, int) __asm__("func_L16_002D5280");
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern void func_00213DE0(void *, int, int, int);
extern void func_L16_002D5438(char *);
extern int func_00215B18(char *, float);
extern void func_L00_00250800(void *, int, void *);
extern char *func_L16_002A1B58(char *, float *, char *);
extern int func_L00_0025D6F0(void *, void *);
extern int func_0022ED80_update(int, int, void *) __asm__("func_0022ED80");
extern void func_0020D678(void *);
extern float func_00214D28(float *, float, float);
extern float func_001F9FA8(float);
extern float func_001F9D10_update(void *, void *) __asm__("func_001F9D10");
extern int func_L00_0028EB98(void *, int);
extern char D_0013E633[];
extern unsigned char D_0015EEB4_update[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern int D_L16_0015F6A8 MACRO_ADDR;
extern int D_L16_0016CD10 MACRO_ADDR;
extern char *D_L16_001B0C30[];
extern char D_L16_00167240[];
extern short D_L16_00161B34;
extern short D_L16_00161B38;
extern short D_L16_00161B3C;

/* Update the floating enemy's patrol, attack and death states. */
void func_L16_002D40B8(char *m) {
    float target[4], temp[4], delta[4];
    char *d = *(char **)(m + 0x78);
    char *source, *owner;
    float distance, angle;
    float *heading;
    int ticks;
    short cls;
    func_L16_002D4C78(m);
    owner = *(char **)(d + 0x110);
    cls = *(short *)(owner + 0xA6);
    switch (cls) {
    case 0:
    case 0x1A3: source = D_0013E633 + 0xEDD; break;
    default: source = owner + 0x10; break;
    }
    qcopy(target, source);
    if (D_0015EEB4_update[3]) func_L00_00264BB0(d + 0x130, 2.5f);
    switch (((unsigned char *)m)[0x20]) {
    case 0:
        *(int *)(d + 0x2EC) = 0;
        d[0x29] = 1;
        *(short *)(d + 0x24) = 3;
        d[0x28] = 2;
        *(float *)(d + 0x30) = 1.5f;
        *(float *)(d + 0x20) = 3.0f;
        *(float *)(d + 0x2F4) = func_00214158();
        *(float *)(d + 0x2FC) = func_00214158();
        *(float *)(d + 0x2E8) = func_00214158();
        *(float *)(d + 0x2F0) = func_002140F8(90.0f, 180.0f) * 0.017453292f * D_0015EE6C;
        *(float *)(d + 0x2F8) = func_002140F8(90.0f, 180.0f) * 0.017453292f * D_0015EE6C;
        *(float *)(d + 0x2E4) = func_002140F8(90.0f, 180.0f) * 0.017453292f * D_0015EE6C;
        *(short *)(d + 0x302) = func_001FA898(func_001F9878(func_002140F8(180.0f, 360.0f)));
        if (func_002140B0(2)) *(unsigned short *)(m + 0x34) |= 0x8000;
        else *(unsigned short *)(m + 0x34) &= 0x7FFF;
        {
            char *position = m + 0x10;
            if (*(int *)(d + 0x120) >= 0) {
                int node = func_001FA898(func_002140F8(0.0f, (float)(*(int *)D_L16_001B0C30[*(int *)(d + 0x120)] - 1)));
                *(short *)(d + 0x300) = node;
                qcopy(position, D_L16_001B0C30[*(int *)(d + 0x120)] + (short)node * 16 + 0x10);
            }
            *(int *)(d + 0x308) = 1;
            qcopy(d + 0x2C0, position);
            qcopy(d + 0x2B0, position);
        }
        if (*(int *)(d + 0x128)) {
            m[0x20] = 10;
            m[0x31] = 0;
            *(unsigned short *)(m + 0x34) = (*(unsigned short *)(m + 0x34) | 1) & 0xEFFF;
            *(int *)(m + 0x94) = 0;
            if (((unsigned char *)m)[0x53] != 1) {
                func_00213DE0(m, 1, 0, func_001F9850(20));
            }
        } else goto idle;
        break;
    case 10:
        if (*(char **)(d + 0x2D0)) {
            m[0x31] = 1;
            *(unsigned short *)(m + 0x34) &= 0xFFFE;
            func_001F9EC0(temp, d + 0x2C0, *(char **)(d + 0x2D0) + 0xC0);
            func_001F9BD8(m + 0x10, temp, *(char **)(d + 0x2D0) + 0x10);
            *(float *)(m + 0x48) = func_001FA748(*(float *)(*(char **)(d + 0x2D0) + 0x48), *(float *)(d + 0x2CC));
        }
        break;
    case 1:
        if (!func_001F9938(d + 0x2DC)) {
            func_L00_001FF4B0(temp, m + 0xC0, *(short *)(d + 0x2DC) * 0.1f);
            func_001F9BD8(temp, temp, m + 0x10);
            func_L16_002D5280_update(m, temp, 1);
        } else {
            heading = (float *)(m + 0x48);
            angle = func_L00_001FF860(*(float *)(d + 0x2B0) - *(float *)(m + 0x10), *(float *)(d + 0x2B4) - *(float *)(m + 0x14));
            func_L00_0025CE58(heading, (float *)(d + 0x2D8), angle, D_0015EE70 * 12.566371f, D_0015EE70 * 12.566371f, D_0015EE6C * 12.566371f);
            distance = func_L16_002D5280_update(m, d + 0x2B0, 1);
            if (((unsigned char *)m)[0x53] != 0) func_00213DE0(m, 0, 0, func_001F9850(10));
            if (distance < 1.0f) m[0x20] = 2;
        }
        break;
    case 2:
        func_001F9938(d + 0x2DC);
        if (*(int *)(d + 0x114) != 2) {
            if (*(short *)(d + 0x2DC) == 0) {
                if (*(int *)(d + 0x120) >= 0) {
                    m[0x20] = 7;
                    *(short *)(d + 0x2DC) = func_001FA898(func_001F9878(func_002140F8(30.0f, 90.0f)));
                    *(short *)(d + 0x300) = (*(short *)(d + 0x300) + 1) % 2;
                    func_L16_002D5438(m);
                } else if (!func_002140B0(20)) {
                    m[0x20] = 4;
                    if (((unsigned char *)m)[0x53] != 2) { func_00213DE0(m, 2, 0, func_001F9850(10)); }
                }
            }
        } else if (func_001F9938(d + 0x302)) {
            if (*(int *)(d + 0x120) >= 0) {
                *(short *)(d + 0x300) = (*(short *)(d + 0x300) + 1) % 2;
                m[0x20] = 3;
                *(short *)(d + 0x302) = func_001F9850(180);
                func_L16_002D5438(m);
            }
        }
        break;
    case 3:
        distance = func_L16_002D5280_update(m, D_L16_001B0C30[*(int *)(d + 0x120)] + (*(short *)(d + 0x300) * 16 + 0x10), 1);
        if (distance < 0.01f || func_001F9938(d + 0x302)) {
            m[0x20] = 2;
            if (((unsigned char *)m)[0x53] != 0) func_00213DE0(m, 0, 0, func_001F9850(10));
            *(short *)(d + 0x302) = func_001FA898(func_001F9878(func_002140F8(180.0f, 360.0f)));
        }
        if (*(float *)(d + 0x2D4) < 2.0f * D_0015EE6C && distance < 0.5f) {
            if (((unsigned char *)m)[0x53] != 0) { ticks = 30; goto animate_idle; }
        }
        break;
    case 4:
        heading = (float *)(m + 0x48);
        angle = func_L00_001FF860(target[0] - *(float *)(m + 0x10), target[1] - *(float *)(m + 0x14));
        func_L00_0025CE58(heading, (float *)(d + 0x2D8), angle, D_0015EE70 * 12.566371f, D_0015EE70 * 12.566371f, D_0015EE6C * 12.566371f);
        if (((unsigned char *)m)[0x70] & 2) {
            m[0x20] = 5;
            if (((unsigned char *)m)[0x53] != 3) { func_00213DE0(m, 3, 0, func_001F9850(10)); }
        }
        break;
    case 5:
        heading = (float *)(m + 0x48);
        angle = func_L00_001FF860(target[0] - *(float *)(m + 0x10), target[1] - *(float *)(m + 0x14));
        func_L00_0025CE58(heading, (float *)(d + 0x2D8), angle, D_0015EE70 * 12.566371f, D_0015EE70 * 12.566371f, D_0015EE6C * 12.566371f);
        if (func_00215B18(m, 2.0f) || func_00215B18(m, 17.0f) || func_00215B18(m, 32.0f)) {
            func_L00_00250800(m, 0, temp);
            func_001F9BF0(delta, target, temp);
            func_L00_001FF4B0(delta, delta, D_0015EE6C * 20.0f);
            func_L16_002A1B58(m, delta, (char *)temp);
        } else if (func_00215B18(m, 10.0f)) {
            int count = *(unsigned short *)(d + 0x2DE) + 1;
            *(unsigned short *)(d + 0x2DE) = count;
            if ((short)count >= 3) {
                m[0x20] = 2;
                if (((unsigned char *)m)[0x53] != 0) func_00213DE0(m, 0, 0, func_001F9850(10));
                *(short *)(d + 0x2DC) = func_001F9850(60);
                *(unsigned short *)(d + 0x2DE) = 0;
            }
        }
        break;
    case 7:
        distance = func_L16_002D5280_update(m, D_L16_001B0C30[*(int *)(d + 0x120)] + (*(short *)(d + 0x300) * 16 + 0x10), 1);
        func_001F9938(d + 0x2DC);
        if (distance < 0.01f || *(short *)(d + 0x2DC) == 0) {
            m[0x20] = 4;
            if (((unsigned char *)m)[0x53] != 2) func_00213DE0(m, 2, 0, func_001F9850(30));
        }
        if (distance < 0.5f && *(float *)(d + 0x2D4) < 2.0f * D_0015EE6C) {
            if (((unsigned char *)m)[0x53] != 2) { func_00213DE0(m, 2, 0, func_001F9850(30)); }
        }
        break;
    case 8:
        if (((unsigned char *)m)[0x70] & 2) {
idle:
            m[0x20] = 2;
            if (((unsigned char *)m)[0x53] != 0) { ticks = 10;
animate_idle:
                func_00213DE0(m, 0, 0, func_001F9850(ticks));
            }
        } else {
            func_L00_001FF4B0(temp, m + 0xC0, -D_0015EE6C);
            func_001F9BD8(m + 0x10, m + 0x10, temp);
        }
        break;
    case 9:
        {
            char *position = m + 0x10;
            qcopy(temp, position);
            if (func_L00_0025D6F0(m, d + 0x70) & 0x160) {
                func_L16_002D55C0(m, position);
                func_0022ED80_update(0, 0, m);
                func_0020D678(m);
                return;
            }
            func_001F9BF0(temp, position, temp);
            func_001F9BD8(d + 0x2C0, d + 0x2C0, temp);
        }
        break;
    case 6:
    default:
        break;
    }
    if (*(int *)&D_L16_00161B34) {
        *(float *)(d + 0x2BC) = *(float *)(m + 0x18);
        if (((unsigned char *)m)[0x20] != 0 && ((unsigned char *)m)[0x20] != 10) {
            func_00214D28((float *)(d + 0x2EC), 0.25f, D_0015EE6C * 0.25f);
            *(float *)(d + 0x2F4) = func_001FA748(*(float *)(d + 0x2F4), *(float *)(d + 0x2F0));
            *(float *)(d + 0x2FC) = func_001FA748(*(float *)(d + 0x2FC), *(float *)(d + 0x2F8));
            *(float *)(d + 0x2E8) = func_001FA748(*(float *)(d + 0x2E8), *(float *)(d + 0x2E4));
            *(float *)(m + 0x10) = *(float *)(d + 0x2C0) + *(float *)(d + 0x2EC) * func_001F9FA8(*(float *)(d + 0x2F4));
            *(float *)(m + 0x14) = *(float *)(d + 0x2C4) + *(float *)(d + 0x2EC) * func_001F9FA8(*(float *)(d + 0x2FC));
            *(float *)(m + 0x18) = *(float *)(d + 0x2C8) + *(float *)(d + 0x2EC) * func_001F9FA8(*(float *)(d + 0x2E8));
        }
        if (func_001F9D10_update(m + 0x10, D_L16_00167240) < 32.0f && !func_L00_0028EB98(m, *(int *)(d + 0x304)))
            *(int *)(d + 0x304) = func_0022ED80_update(7, 4, m);
    }
    if (*(int *)&D_L16_00161B38) func_L16_002D5958(m);
    if (*(int *)&D_L16_00161B3C && *(int *)(D_0013E633 + 0x2EA9) != 22) func_L16_002D5CD0(m);
    if (D_0015EE84 == 18 && D_L16_0015F6A8 == 2 && (unsigned int)(D_L16_0016CD10 - 3) < 3U) func_0020D678(m);
}
