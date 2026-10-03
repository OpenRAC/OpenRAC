/* NON_MATCHING func_L07_0031C7C0 -- src/overlays/l07_umbris/vendor_0031BDB8.c
 * Best so far: BYTES 24/508 (95.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1133: 6-state machine (jump table) easing moby[0x44]/d[0x18] with func_001FA748/001FA790 while the 
 *   p3.c size equal; left: case 1 mul operand order (p4 fixed it with v=D_0015EE6C local but reusing v in case 4 r
 */
extern int func_0022ED80(int, int, int);
extern float func_001F9F90(float);
extern float func_001FA748(float, float);
extern float func_001FA790(float, float);
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L07_00160058 MACRO_ADDR;

// UpdateMoby_1133: state machine that eases a value toward targets while the linked moby is alive.
void func_L07_0031C7C0(char *moby) {
    char *d = *(char **)(moby + 0x78);
    char *o;
    float v;
    if (d) {
        switch ((unsigned char)moby[0x20]) {
        case 0:
            *(int *)(moby + 0x44) = 0;
            moby[0x20] = 1;
            *(int *)(d + 0x18) = 0;
            break;
        case 1:
            if (*(int *)d != -1 && (o = D_L07_00160058 + (*(int *)d << 8)) != 0 &&
                (unsigned char)o[0x20] != 0xFE && (unsigned char)o[0x20] != 0xFD &&
                (unsigned char)o[0x20] != *(int *)(d + 4)) {
                return;
            }
            v = *(float *)(d + 0xC) * D_0015EE6C;
            *(float *)(d + 0x18) = v;
            *(float *)(moby + 0x44) = v;
            func_0022ED80(0, 0, (int)moby);
            moby[0x20] = 3;
            break;
        case 3:
            *(float *)(d + 0x18) = func_001FA748(*(float *)(d + 0x18), func_001F9F90(*(float *)(moby + 0x44)) * (*(float *)(d + 0xC) * D_0015EE6C));
            *(float *)(moby + 0x44) = func_001FA748(*(float *)(moby + 0x44), *(float *)(d + 0x18));
            o = D_L07_00160058 + (*(int *)d << 8);
            *(float *)(d + 0x18) = *(float *)(d + 0x18) * *(float *)(d + 0x10);
            if (o != 0 && (unsigned char)o[0x20] != 0xFE && (unsigned char)o[0x20] != 0xFD) {
                if ((unsigned char)o[0x20] != *(int *)(d + 8)) {
                    return;
                }
                moby[0x20] = 4;
            }
            break;
        case 4:
            if (*(int *)d == -1 || (o = D_L07_00160058 + (*(int *)d << 8)) == 0 ||
                (unsigned char)o[0x20] == 0xFE || (unsigned char)o[0x20] == 0xFD ||
                (unsigned char)o[0x20] == *(int *)(d + 4)) {
                func_0022ED80(0, 0, (int)moby);
                moby[0x20] = 2;
            } else {
                v = func_001FA790(*(float *)(moby + 0x44), *(float *)(d + 0x14));
                *(float *)(moby + 0x44) = v;
                if (0.0f < v) {
                    *(float *)(moby + 0x44) = 0.0f;
                    *(float *)(d + 0x18) = 0.0f;
                    moby[0x20] = 1;
                }
            }
            break;
        case 2:
        case 5:
            break;
        }
    }
}
