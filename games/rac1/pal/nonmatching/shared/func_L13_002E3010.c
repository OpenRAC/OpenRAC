/* NON_MATCHING func_L13_002E3010 -- src/overlays/shared/vendor_002B8FC0.c
 * Best so far: SIZE ours 1636 / retail 1664, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Update function (state machine, cases 0..6 via jump table) for ammocrate / jet-fighter health crate; calls the
 *   Runs so far: p0 (COMPILE fixed to int func_001F9908, then SIZE 1632/1664), p1 (unsigned char d, separate 0xFE/
 *   Left: retail keeps the level base in $16 as a hi-part (`daddu $16,$2,$0` before the `%lo` addiu) and recompute
 *   Unblock: the source form that gives retail's hi-part base (likely a symbol-with-addend pointer that the compil
 */
extern char D_0013E633[];
extern char *D_L13_00160058 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L13_001741D8;
extern int func_001F9908(int *arg0);
extern float func_001FA748(float, float);
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern float func_001F9D10(void *, void *);
extern void func_0022ED80(int, int, int);
extern float func_001F9B50(float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_0020D678(void *);
extern void func_L00_00263B78(float, float, char *, float *, float *);

// Update function for the ammocrate and jet-fighter health crate mobies: a state machine that moves the crate and tracks the hero.
void func_L13_002E3010(unsigned char *m) {
    char *g = D_0013E633 + 0xE1D;
    unsigned char *hero = *(unsigned char **)(g + 0x15F0);
    unsigned char *d = *(unsigned char **)(m + 0x78);
    int flag = 0;
    float f20 = 100000.0f;
    float tmp[4];
    float tmp2[4];

    switch (m[0x20]) {
    case 0:
        if (d[0xC] != 0) {
            *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24);
            m[0x20] = 4;
        } else {
            *(unsigned short *)(m + 0x34) |= 1;
            *(int *)(m + 0x94) = 0;
            m[0x31] = 0;
            m[0x20] = 1;
            *(float *)(m + 0x2C) = *(float *)(*(char **)(m + 0x24) + 0x24) * 0.1f;
        }
        break;
    case 1:
        if (*(int *)d == -1) {
            m[0x20] = 3;
        } else {
            m[0x20] = 2;
        }
        break;
    case 2: {
        unsigned char *e = (unsigned char *)D_L13_00160058 + (*(int *)d << 8);
        if (e == 0) {
            m[0x20] = 3;
        } else if (e[0x20] == 0xFE) {
            m[0x20] = 3;
        } else if (e[0x20] == 0xFD) {
            m[0x20] = 3;
        } else if (*(int *)(d + 4) == -1) {
            m[0x20] = 3;
        } else if (e[0x20] == *(int *)(d + 4)) {
            m[0x20] = 3;
        }
        break;
    }
    case 3:
        if (*(int *)(d + 8) == -1 || func_001F9908((int *)(d + 0x10))) {
            m[0x20] = 4;
        }
        if (m[0x20] == 4) {
            char *p = *(char **)(m + 0x24);
            m[0x31] = 1;
            *(int *)(m + 0x94) = *(int *)(p + 0x10);
            *(unsigned short *)(m + 0x34) &= 0xFFFE;
            *(float *)(m + 0x2C) = *(float *)(p + 0x24) * 0.1f;
        }
        break;
    case 4: {
        char *p;
        char *q;
        short cls;
        *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), D_0015EE6C * 0.7853982f);
        if (*(float *)(m + 0x10) < 20.0f) *(float *)(m + 0x10) = 20.0f;
        if (1000.0f < *(float *)(m + 0x10)) *(float *)(m + 0x10) = 1000.0f;
        if (*(float *)(m + 0x14) < 20.0f) *(float *)(m + 0x14) = 20.0f;
        if (1000.0f < *(float *)(m + 0x14)) *(float *)(m + 0x14) = 1000.0f;
        if (*(float *)(m + 0x18) < 20.0f) *(float *)(m + 0x18) = 20.0f;
        if (1000.0f < *(float *)(m + 0x18)) *(float *)(m + 0x18) = 1000.0f;
        func_L00_00250800(m, 0, tmp);
        p = *(char **)(m + 0x24);
        *(float *)(m + 0x2C) = *(float *)(m + 0x2C) + (*(float *)(p + 0x24) - *(float *)(m + 0x2C)) * 0.1f;
        if (func_L00_001F10E0(2.0f, tmp, 1, m)) {
            q = D_L13_001741D8;
            if (q != 0) {
                short c = *(short *)(q + 0xA6);
                if (c == 0x4DA || c == 0x45 || c == 0x563) {
                    flag = 1;
                }
            }
        }
        if (*(int *)(g - 0xBB0 + 0x2084) == 0x32) {
            if (hero != 0 && hero[0x20] != 0xFE && hero[0x20] != 0xFD) {
                cls = *(short *)(hero + 0xA6);
                if (cls == 0x4DA || cls == 0x45 || cls == 0x563) {
                    f20 = func_001F9D10(hero + 0x10, m + 0x10);
                }
            }
        }
        if (flag != 0 || f20 < 4.0f) {
            cls = *(short *)(m + 0xA6);
            *(int *)(m + 0x94) = 0;
            if (cls == 0xE0) {
                unsigned char *h2;
                char *s = g - 0xBB0;
                s[0x15F6] += 5;
                h2 = *(unsigned char **)(s + 0x15F0);
                if (h2 != 0 && h2[0x20] != 0xFE && h2[0x20] != 0xFD) {
                    func_0022ED80(4, 0, (int)h2);
                }
                s = g - 0xBB0;
                if (s[0x15F7] < s[0x15F6]) {
                    s[0x15F6] = s[0x15F7];
                }
                m[0x20] = 5;
            } else if (cls == 0xE4) {
                unsigned char *h2;
                char *s = g - 0xBB0;
                *(float *)(s + 0x15FC) = *(float *)(s + 0x15FC) + 10.0f;
                h2 = *(unsigned char **)(s + 0x15F0);
                if (h2 != 0 && h2[0x20] != 0xFE && h2[0x20] != 0xFD) {
                    func_0022ED80(5, 0, (int)h2);
                }
                s = g - 0xBB0;
                if (*(float *)(s + 0x1600) < *(float *)(s + 0x15FC)) {
                    *(float *)(s + 0x15FC) = *(float *)(s + 0x1600);
                }
                m[0x20] = 5;
            } else {
                m[0x20] = 5;
            }
        } else {
            if (f20 < 40.0f && m[0x31] != 0) {
                char *mp = (char *)m + 0x10;
                f20 = 0.8f / func_001F9B50(f20);
                func_001F9BF0(tmp2, hero + 0x10, mp);
                func_L00_001FF4B0(tmp2, tmp2, f20);
                func_001F9BD8(mp, mp, tmp2);
            }
        }
        break;
    }
    case 5: {
        char *p = *(char **)(m + 0x24);
        float t = *(float *)(p + 0x24) - *(float *)(m + 0x2C);
        float n;
        *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), t + t);
        n = *(float *)(m + 0x2C) + (*(float *)(p + 0x24) * 0.01f - *(float *)(m + 0x2C)) * 0.3f;
        *(float *)(m + 0x2C) = n;
        if (n < *(float *)(p + 0x24) * 0.05f) {
            m[0x20] = 6;
        }
        break;
    }
    case 6:
        if (d[0xD] == 0) {
            func_0020D678(m);
            return;
        }
        {
            char *p = *(char **)(m + 0x24);
            *(unsigned short *)(m + 0x34) |= 1;
            *(int *)(m + 0x94) = 0;
            m[0x31] = 0;
            *(float *)(m + 0x2C) = *(float *)(p + 0x24) * 0.1f;
            *(int *)(d + 0x10) = *(int *)(d + 8);
            m[0x20] = 1;
        }
        break;
    default:
        break;
    }

    func_L00_00263B78(0.7f, D_0015EE6C * 1.2217305f, (char *)m, (float *)(d + 0x18), (float *)(d + 0x14));
    if (*(float *)(m + 0x10) < 20.0f) *(float *)(m + 0x10) = 20.0f;
    if (1000.0f < *(float *)(m + 0x10)) *(float *)(m + 0x10) = 1000.0f;
    if (*(float *)(m + 0x14) < 20.0f) *(float *)(m + 0x14) = 20.0f;
    if (1000.0f < *(float *)(m + 0x14)) *(float *)(m + 0x14) = 1000.0f;
    if (*(float *)(m + 0x18) < 20.0f) *(float *)(m + 0x18) = 20.0f;
    if (1000.0f < *(float *)(m + 0x18)) *(float *)(m + 0x18) = 1000.0f;
}
