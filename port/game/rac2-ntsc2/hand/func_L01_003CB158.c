/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* func_002837F8 as it is: the distance between two points. */
extern float openrac_distance(void *, void *) __asm__("func_002837F8");

/* An Oozla mover's update (level 1, 0x003C9D00), the decompilation's C with one change: its first
 * state measures from the marker to s + 0x80, the point the matching C leaves in $a1 (the qcopy just
 * before wrote it) and does not pass to func_002837F8. */
void func_L01_003CB158(char *obj)
{
    float vec[4];
    char *s = *(char **)(obj + 0x68);
    float blend = 0.0f;
    switch (*(unsigned char *)(obj + 0x20)) {
    case 0:
        *(int *)(s + 0x90) = -1;
        *(int *)(s + 0x5C) |= 4;
        *(float *)(s + 0x70) = 0.0f;
        *(int *)(s + 0x6C) = 0;
        *(float *)(s + 0x74) = 0.0f;
        *(int *)(obj + 0x98) = 0;
        qcopy(s + 0x80, obj + 0x10);
        *(float *)(s + 0x98) = openrac_distance(D_L01_001B264C[*(int *)(s + 0x94)].position, s + 0x80);
        func_002A85B8(obj, 1, -1);
        break;
    case 1:
        if (*(unsigned char *)(obj + 0xBC)) {
            *(int *)(obj + 0x98) = *(int *)(*(char **)(obj + 0x24) + 0x10);
            func_002A85B8(obj, 2, -1);
        }
        break;
    case 2:
        *(float *)(s + 0x70) += 1.0f / *(float *)(s + 0x60);
        if (*(float *)(s + 0x70) > 1.0f) *(float *)(s + 0x70) = 1.0f;
        else if (*(float *)(s + 0x70) < 0.0f) *(float *)(s + 0x70) = 0.0f;
        blend = (1.0f - func_00283B30(*(float *)(s + 0x70) * 3.1415927f)) * 0.5f;
        if (*(int *)(s + 0x94) == -1) {
            func_002838D0(vec, obj + 0xC0, blend * 10.0f);
            func_002836A0(obj + 0x10, s + 0x80, vec);
        } else {
            func_002836A0(vec, D_L01_001B264C[*(int *)(s + 0x94)].position, s + 0x80);
            func_002838D0(vec, vec, *(float *)(s + 0x98) * blend);
            func_00283670(obj + 0x10, s + 0x80, vec);
        }
        if (*(float *)(s + 0x70) >= 1.0f) {
            *(int *)(s + 0x6C) = *(int *)(s + 0x64);
            func_002A85B8(obj, 3, -1);
        }
        break;
    case 3:
        blend = 1.0f;
        if (func_002832F8(s + 0x6C)) func_002A85B8(obj, 4, -1);
        break;
    case 4:
        *(float *)(s + 0x70) -= 1.0f / *(float *)(s + 0x60);
        if (*(float *)(s + 0x70) > 1.0f) *(float *)(s + 0x70) = 1.0f;
        else if (*(float *)(s + 0x70) < 0.0f) *(float *)(s + 0x70) = 0.0f;
        blend = (1.0f - func_00283B30(*(float *)(s + 0x70) * 3.1415927f)) * 0.5f;
        if (*(int *)(s + 0x94) == -1) {
            func_002838D0(vec, obj + 0xC0, blend * 10.0f);
            func_002836A0(obj + 0x10, s + 0x80, vec);
        } else {
            func_002836A0(vec, D_L01_001B264C[*(int *)(s + 0x94)].position, s + 0x80);
            func_002838D0(vec, vec, *(float *)(s + 0x98) * blend);
            func_00283670(obj + 0x10, s + 0x80, vec);
        }
        if (*(float *)(s + 0x70) <= 0.0f) {
            *(int *)(obj + 0x98) = 0;
            func_002A9400(*(unsigned char *)(obj + 0x21), 0);
            func_002A85B8(obj, 1, -1);
        }
        break;
    }
    *(unsigned char *)(obj + 0xBC) = 0;
    if ((unsigned int)(*(unsigned char *)(obj + 0x20) - 2) < 3) {
        func_L00_002E7718(func_L01_003CAC30, obj);
        func_002838D0(vec, obj + 0xC0, *(float *)(s + 0x68) * ((float)*(int *)(s + 0x7C) * 0.016666668f));
        func_002AE558(s + 0x20, vec, obj + 0xF0, obj + 0xF0);
    }
    qcopy(vec, obj + 0x10);
    vec[3] = 10.0f;
    if (*(unsigned char *)(obj + 0x20) >= 2 && (*(unsigned char *)(obj + 0x31) || func_00284768(vec, 255.0f) != -1)) {
        if (*(int *)(s + 0x90) == -1) {
            *(int *)(s + 0x90) = func_L00_002FDB60(s + 0x80, 10.0f, 0.0f, blend * 0.8f, blend * 0.9f, blend * 1.1f);
        } else {
            char *light = D_L01_001C7640 + *(int *)(s + 0x90) * 32;
            *(float *)(light + 0) = blend * 0.8f;
            *(float *)(light + 4) = blend * 0.9f;
            *(float *)(light + 8) = blend * 1.1f;
        }
    } else if (*(int *)(s + 0x90) != -1) {
        func_00291CC0(*(int *)(s + 0x90));
        *(int *)(s + 0x90) = -1;
    }
}
