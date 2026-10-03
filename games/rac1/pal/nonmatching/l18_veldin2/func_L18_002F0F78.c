/* NON_MATCHING func_L18_002F0F78 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 10/1192 (99.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ## Round 1 (match worker, 12 runs)
 *   UpdateMoby_1355 state machine (states 0 init, 2 wander, 3 dive, 4 land; level-state check despawns). Best: p8.
 *   Mattered: unsigned-char state loads (lbu); (float)i<4.0f loop; locals a=EE70,b=EE6C declared in that order wit
 *   Remaining: (1) `li 4` for `moby[0x20]=4` in state 3 lands in $v0, retail $v1; (2) mov.s $f17,$f12 vs addiu $t2
 */
extern float func_001FA748(float, float);
extern void func_0020D678(void *);
extern float func_00214158(void);
extern float func_00214358(void *, int, float);
extern int func_002140B0(int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_00214D28(float *, float, float);
extern void func_001F9C08(void *, void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern void func_0022ED80(int, int, int);
extern int func_001F9850(int);
extern float func_L00_0025CC58(float *p, int n, float x, float y);
extern int func_001F9908(int *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, int, float, float, int, int, int, int);
extern int D_L18_0015F6A8 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L18_0016D310;
extern float D_L18_00167840[];
extern int D_L18_0015F660 MACRO_ADDR;
extern short D_L18_001622A0;
extern short D_L18_001622A4;
extern short D_L18_001622A8;
extern short D_L18_001622C4;
extern int func_L18_002F1510(char *moby, float *vec);
extern void func_L18_002F16D8(char *moby, void *out);
extern void func_L18_002F1C38(char *moby);

/* State machine for the class 1355 moby: dispatches on moby state (0 init, 2 wander, 3 dive, 4 land). */
void func_L18_002F0F78(char *moby) {
    char *d = *(char **)(moby + 0x78);
    if (((unsigned char *)moby)[0x20] != 4) {
        *(float *)(moby + 0x40) = func_001FA748(*(float *)(moby + 0x40),
                                  *(float *)&D_L18_001622A0 * 0.017453292f * D_0015EE6C);
    }
    if (D_L18_0015F6A8 == 2 && D_L18_0016D310 == 3) {
        func_0020D678(moby);
        return;
    }
    switch (((unsigned char *)moby)[0x20]) {
    case 0: {
        float a = D_0015EE70;
        float b = D_0015EE6C;
        *(float *)(d + 0x158) = a * 10.0f;
        *(float *)(d + 0x154) = b * 12.566371f;
        *(float *)(d + 0x150) = a * 3.1415927f;
        *(float *)(d + 0x15C) = b * 10.0f;
        *(float *)(d + 0x168) = func_00214158();
        *(short *)(moby + 0x32) = 0xFF;
        qcopy(d + 0x120, moby + 0x10);
        *(float *)(d + 0x128) += 5.0f;
        *(float *)(d + 0x128) = func_00214358(d + 0x120, 0, 0.5f);
        func_L18_002F16D8(moby, d + 0x110);
        moby[0x20] = 1;
        *(unsigned short *)(moby + 0x34) |= 1;
        moby[0x31] = 0;
        *(int *)(moby + 0x94) = 0;
        break;
    }
    case 2: {
        char *p = d + 0x110;
        if (func_L18_002F1510(moby, (float *)p)) {
            func_L18_002F16D8(moby, p);
        }
        func_L18_002F1C38(moby);
        if (func_002140B0(*(int *)&D_L18_001622C4)) {
            return;
        }
        moby[0x20] = 3;
        *(float *)(d + 0x130) = func_001F9F90(*(float *)(moby + 0x48)) * (*(float *)&D_L18_001622A4 * D_0015EE6C);
        *(float *)(d + 0x134) = func_001F9FA8(*(float *)(moby + 0x48)) * (*(float *)&D_L18_001622A4 * D_0015EE6C);
        *(float *)(d + 0x138) = *(float *)(d + 0x144);
        *(int *)(d + 0x14C) = 0;
        break;
    }
    case 3: {
        float v[4];
        float w[4];
        func_001F9BF0(v, d + 0x120, moby + 0x10);
        func_L00_001FF4B0(v, v, *(float *)&D_L18_001622A8 * D_0015EE6C);
        func_00214D28((float *)(d + 0x14C), 1.0f, D_0015EE6C);
        func_001F9C08(v, d + 0x130, v, *(float *)(d + 0x14C));
        func_001F9BD8(moby + 0x10, moby + 0x10, v);
        *(float *)(moby + 0x48) = func_L00_001FF860(v[0], v[1]);
        *(float *)(moby + 0x44) = -func_L00_001FF860(func_001F9CE8(v), v[2]);
        func_L00_001FF4B0(w, moby + 0xC0, 1.0f);
        func_001F9BD8(w, w, moby + 0x10);
        func_L18_002F1C38(moby);
        if (w[2] < *(float *)(d + 0x128)) {
            float t;
            func_0022ED80(1, 0, (int)moby);
            ((unsigned char *)moby)[0x20] = 4;
            *(int *)(d + 0x160) = func_001F9850(0x5A);
            t = *(float *)(d + 0x128) + 0.7f;
            if (*(float *)(moby + 0x18) < t) {
                *(float *)(moby + 0x18) = t;
            }
        }
        break;
    }
    case 4: {
        float x = *(float *)(moby + 0x44);
        if (x < 0.87266463f) {
            x = 0.87266463f;
        }
        func_L00_0025CC58((float *)(moby + 0x44), 0, x, D_0015EE6C * 12.566371f);
        if (func_001F9908((int *)(d + 0x160))) {
            int i;
            char *pos = moby + 0x10;
            moby[0x20] = 2;
            *(short *)(d + 0x164) = 0;
            func_L00_0025F4A8(*(void **)(d + 0x16C), &D_L18_0015F660, pos, 1.0f, 1.0f, 10, 3, 16,
                              1.5f, 0.5f, 9.0f, 2, 1.0f, 5.0f, 0, 0, -1, 0);
            qcopy(pos, D_L18_00167840);
            *(float *)(moby + 0x18) += 20.0f;
            for (i = 0; (float)i < 4.0f; i++) {
                qcopy(d + 0x10 + i * 16, pos);
            }
        } else {
            func_L18_002F1C38(moby);
        }
        break;
    }
    }
}
