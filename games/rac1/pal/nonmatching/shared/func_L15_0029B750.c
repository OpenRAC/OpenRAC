/* NON_MATCHING func_L15_0029B750 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: SIZE ours 1604 / retail 1588, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Shared moby update (levels 15 and 17): a 0x1B-class state machine on moby[0x20] with a jump table over r = fun
 *   Still off: the register pick (ours puts data in $16 and the switch value in $2; retail has data in $17 and the
 *   Budget (5 runs of 10) used on this function; the next steps are a register-order experiment on the switch valu
 */
extern char *func_L00_0025B478(void *, int, int);
extern int func_L00_0025B4D0(void *, void *, void *, int, int *, float *, int, int);
extern void func_L15_0029B2B0(void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_L00_001FF860(float, float);
extern void func_L00_0025BBA0(void *, float *, void *, void *);
extern void func_L00_0025D5B0(void *, void *, float, int, int, int);
extern void func_L00_0025E4B0(void *m, short *p);
extern void func_L00_0025E590(void *, void *);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern void func_L08_00211A38(float *);
extern float func_001F9D10(void *, void *);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern int func_L00_00260FB0(void *, void *, float, int, int, void *, int);
extern float func_001FA850(float, float);
extern float func_001F9B88(float);
extern int *D_L15_001B0DB0[];
extern int D_L15_0015F6A8 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern int D_L15_00160058_m __asm__("D_L15_00160058") MACRO_ADDR;
extern short D_L15_00161474;
extern short D_L15_00161478;
extern short D_L15_0016147C;
extern short D_L15_00161480;
extern short D_L15_00161484;
extern short D_L15_00161488;
extern short D_L15_0016148C;
extern short D_L15_00161490;
extern short D_L15_00161494;

/* Update for shared moby class 0x15F-era object: state machine that aims, steps and fires its shot. */
void func_L15_0029B750(void *mobyv, int flag)
{
    char *data, *q, *e, *P;
    unsigned char *moby = mobyv;
    int r16, ii, r2;
    float ff, vec[4], dd, dd2, f0, f1, dx, dy, ang, r, ka, kb;

    if (moby[0x20] == 0) return;
    data = *(char **)(moby + 0x78);
    f0 = *(float *)(*(char **)(moby + 0x24) + 0x24) * *(float *)&D_L15_00161474;
    *(float *)(moby + 0x2C) = f0;
    q = func_L00_0025B478(moby, 0x330000, 0);
    ff = 0.0f;
    r16 = func_L00_0025B4D0(moby, q, data + 0x20, 0, &ii, &ff, 0, 4);
    if (ii == 1) goto ba60;
    if (moby[0x20] == 0x17) goto ba60;
    if (*(unsigned char *)(D_0013E633 + 0x2EC1) == 0) func_L15_0029B2B0(moby);
    f0 = *(float *)(data + 0x20) - ff;
    *(float *)(data + 0x20) = f0;
    e = *(char **)(q + 0x20);
    if ((*(short *)(e + 0xA6) ^ 0x47) == 0) r16 = 3;
    if (f0 <= 0.0f) r16 = 1;
    *(int *)(data + 0x90) = func_001FA898_r(512.0f);
    *(float *)(data + 0x98) = 0.5f;
    *(int *)(data + 0x94) = 9;
    *(float *)(data + 0x80) = *(float *)&D_L15_00161478 * D_0015EE70;
    data[0xAD] = 0;
    if (r16 >= 12) goto ba4c;
    switch (r16) {
    case 0:
    case 11:
        goto ba4c;
    case 1:
    case 2:
        kb = D_0015EE70;
        ka = D_0015EE6C;
        *(float *)(data + 0x80) = *(float *)&D_L15_0016147C * kb;
        *(float *)(data + 0x88) = *(float *)&D_L15_00161490 * ka;
        *(float *)(data + 0x8C) = *(float *)&D_L15_0016148C * ka;
        *(float *)(data + 0x84) = *(float *)&D_L15_00161494 * ka;
        *(unsigned short *)(moby + 0x34) &= 0xEFFF;
        *(float *)(data + 0xC0) = 8.0f;
        *(float *)(data + 0xC4) = 16.0f;
        e = *(char **)(q + 0x20);
        dx = *(float *)(moby + 0x10) - *(float *)(e + 0x10);
        dy = *(float *)(moby + 0x14) - *(float *)(e + 0x14);
        dd2 = func_L00_001FF860(dx, dy);
        qcopy(vec, q + 0x10);
        func_L00_0025BBA0(vec, &dd2, data + 0x88, data + 0x8C);
        func_L00_0025D5B0(moby, data + 0x70, dd2, 8, 1, 0);
        moby[0x20] = 0x17;
        moby[0xBC] = 2;
        data[0x67] = 0xF0;
        goto ba44;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        kb = D_0015EE70;
        ka = D_0015EE6C;
        *(float *)(data + 0x80) = *(float *)&D_L15_00161478 * kb;
        *(float *)(data + 0x88) = *(float *)&D_L15_00161484 * ka;
        *(float *)(data + 0x8C) = *(float *)&D_L15_00161480 * ka;
        *(float *)(data + 0x84) = *(float *)&D_L15_00161488 * ka;
        *(float *)(data + 0xC0) = 5.0f;
        *(float *)(data + 0xC4) = 10.0f;
        e = *(char **)(q + 0x20);
        dx = *(float *)(moby + 0x10) - *(float *)(e + 0x10);
        dy = *(float *)(moby + 0x14) - *(float *)(e + 0x14);
        dd = func_L00_001FF860(dx, dy);
        qcopy(vec, q + 0x10);
        func_L00_0025BBA0(vec, &dd, data + 0x88, data + 0x8C);
        func_L00_0025D5B0(moby, data + 0x70, dd, 7, 1, 0);
        moby[0x20] = 0x15;
        moby[0xBC] = 2;
        data[0x67] = 0x78;
        goto ba44;
    case 9:
    case 10:
        data[0x67] = 0xFA;
        goto ba4c;
    }
ba44:
ba4c:
    func_L00_0025E4B0(moby, (short *)(data + 0x60));
ba60:
    moby[0xA4] = 0xFF;
    func_L00_0025E590(moby, data + 0x60);
    if (flag) return;
    if (*(int *)(data + 0x38) != 0) {
        f0 = func_001F9878(func_002140F8(180.0f, 240.0f));
        r2 = func_001FA898_r(f0);
        *(int *)(data + 0x38) = 0;
        *(float *)(data + 0x130) = (float)r2;
    }
    func_L08_00211A38((float *)(data + 0x130));
    if (func_001F9D10(moby + 0x10, D_0013E633 + 0xEED) < 64.0f) {
        qcopy(vec, moby + 0x10);
        vec[2] = vec[2] + 1.0f;
        P = 0;
        if (*(int *)(data + 0x12C) != -1) P = (char *)D_L15_00160058_m + (*(int *)(data + 0x12C) << 8);
        r2 = func_L00_001EFFF0(vec, D_0013E633 + 0xEED, 2, P, 0);
        data[0x15A] = (r2 == 0);
    } else {
        data[0x15A] = 0;
    }
    {
        int *Pp = D_L15_001B0DB0[*(int *)(data + 0x120)];
        r2 = func_L00_00260FB0(moby, data + 0xD0, 128.0f, 0, 0, (char *)Pp + 0x10, *(int *)Pp);
    }
    r16 = 0;
    if (r2 == 2) {
        data[0x19F] = 0;
        data[0x15B] = 1;
        *(short *)(data + 0x158) = 0;
        goto bd48;
    }
    dx = *(float *)(data + 0xD0) - *(float *)(moby + 0x10);
    dy = *(float *)(data + 0xD4) - *(float *)(moby + 0x14);
    ang = func_L00_001FF860(dx, dy);
    r = func_001FA850(*(float *)(moby + 0x48), ang);
    if (r < 1.5707964f) {
        if (*(short *)(D_0013E633 + 0x1125) == 0) {
            r16 = data[0x15A];
            goto bc3c;
        }
        if (func_001F9B88(*(float *)(moby + 0x18) - *(float *)(data + 0xD8)) < 4.0f) {
            r16 = data[0x15A];
            goto bc3c;
        }
    }
    if (moby[0x20] == 0x10 || moby[0x20] == 0x11) r16 = data[0x15A];
bc3c:
    if (*(int *)(data + 0x114) == 1) {
        data[0x15E] = data[0x15B] + 1;
    } else if (*(unsigned char *)(D_0013E633 + 0x2EC1) == 3) {
        if (data[0x15E] >= 2) data[0x15B] = 1;
    } else {
        data[0x15E] = 0;
    }
    data[0x19F] = 1;
    if (r16 == 0) {
        if (data[0x15A] == 0) data[0x15B] = 1;
        data[0x19F] = 0;
        *(int *)(data + 0x114) = 2;
        *(short *)(data + 0x158) = 0;
    } else {
        if ((*(short *)(data + 0x156) == 0 || data[0x15B] != 0) && *(unsigned char *)(D_0013E633 + 0x2EC1) == 3) {
            *(int *)(data + 0x114) = 2;
            *(short *)(data + 0x158) = 1;
        } else if (data[0x15E] != 0 && *(int *)(data + 0x114) != 1) {
            *(int *)(data + 0x114) = 2;
            *(short *)(data + 0x158) = 1;
        } else if (*(unsigned char *)(D_0013E633 + 0x2EC1) != 3 && *(int *)(data + 0x114) != 2) {
            data[0x15B] = 0;
        }
    }
    if (D_L15_0015F6A8 == 2) *(int *)(data + 0x114) = D_L15_0015F6A8;
bd48:
    r2 = *(int *)(data + 0x110);
    if (r2 != 0) return;
    *(int *)(data + 0x110) = *(int *)(D_0013E633 + 0x2E9D);
}
