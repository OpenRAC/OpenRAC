/* NON_MATCHING func_L06_00309170 -- src/overlays/shared/vendor_002FF000.c
 * Best so far: SIZE ours 1648 / retail 1664, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Bot door lock update (class 1302): a block that aims the lock (func_001FA748, func_001F9F90, func_001FA8A8 sto
 *   Best candidate p4.c: 1640 bytes against 1664 (24 bytes, six instructions short). Our compile uses $gp-relative
 *   Would unblock: the form of D_0015EE6C in the body (lui vs $gp) and the placement of the 0.5 and pi/2 constants
 *   Stopped at run 6 of 10.
 */
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern int func_001FA8A8(int, int, float);
extern float func_00214158(void);
extern float func_001F9D10(void *, void *);
extern void func_001F49B0(void *, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_001F9FA8(float);
extern void func_L00_001FF240(void *, void *, void *);
extern void func_L06_00309088(unsigned char *a, unsigned char *m);
extern void func_L01_00286530(void *, void *, float, int, int, int, int, int);
extern void func_L06_003089B8(void);
extern unsigned char D_0013E633[];
extern unsigned char D_0015EEB4_m[4] __asm__("D_0015EEB4") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L06_00162374 SDATA(D_L06_00162374);
extern float D_L06_0016237C SDATA(D_L06_0016237C);
extern float D_L06_00162370 SDATA(D_L06_00162370);
extern float D_L06_0016236C SDATA(D_L06_0016236C);
extern float D_L06_00162378 SDATA(D_L06_00162378);
extern char *D_L06_00160058 SDATA(D_L06_00160058);

/* Bot door lock update (class 1302): aims the lock, counts down, and runs the door's slot callbacks. */
void func_L06_00309170(char *moby) {
    float buf[12];
    char *data;
    float x;
    float r2;
    float q;
    float q2;
    float t;
    int ret;
    int st;
    int r14;
    int i;
    char *p;
    int v21;

    data = *(char **)(moby + 0x78);
    if (*(int *)data == -1) {
        return;
    }

    x = 360.0f / D_L06_00162374;
    x = x * 0.0174532924f;
    x = x * D_0015EE6C;
    x = func_001FA748(*(float *)(data + 0x28), x);
    *(float *)(data + 0x28) = x;
    q = func_001F9F90(x);
    *(int *)(moby + 0x90) = func_001FA8A8(0x80464646, 0x80828282, (q + 1.0f) * 0.5f);
    st = *(unsigned char *)(moby + 0x20);
    if (st == 1) {
        goto state1;
    }
    if (st < 2) {
        if (st == 0) {
            *(int *)(data + 0x14) = *(int *)(data + 0x10);
            *(float *)(data + 0x18) = func_00214158();
            *(float *)(data + 0x28) = func_00214158();
            *(unsigned char *)(moby + 0x20) = 1;
            *(float *)(data + 0x24) = D_L06_0016237C;
        }
        return;
    }
    if (st != 2) {
        return;
    }
    x = *(float *)(data + 0x24) - D_0015EE6C * 2.0f;
    *(float *)(data + 0x24) = x;
    if (x < 0.0f) {
        *(float *)(data + 0x24) = 0.0f;
    }
    if (0.0f < *(float *)(data + 0x24)) {
        func_001F49B0((void *)func_L06_003089B8, moby);
    }
    return;

state1:
    *(u128 *)buf = *(u128 *)(moby + 0x10);
    buf[2] = buf[2] + 2.75f;
    r2 = func_001F9D10(D_L06_00167640, moby + 0x10);
    if (r2 < 32.0f) {
        x = *(float *)(data + 0x24) + D_0015EE6C * 2.0f;
        if (D_L06_0016237C < x) {
            x = D_L06_0016237C;
        }
        *(float *)(data + 0x24) = x;
    } else {
        r2 = func_001F9D10(D_L06_00167640, moby + 0x10);
        if (36.0f < r2) {
            x = *(float *)(data + 0x24) - D_0015EE6C * 2.0f;
            *(float *)(data + 0x24) = x;
            if (x < 0.0f) {
                *(float *)(data + 0x24) = 0.0f;
            }
        }
    }
    if (0.0f < *(float *)(data + 0x24)) {
        func_001F49B0((void *)func_L06_003089B8, moby);
        x = 360.0f / D_L06_00162370;
        x = x * 0.0174532924f;
        x = x * D_0015EE6C;
        x = func_001FA748(*(float *)(data + 0x18), x);
        *(float *)(data + 0x18) = x;
        q = func_001F9F90(x);
        t = D_L06_0016236C * q;
        buf[2] = buf[2] + t;
        q2 = func_001F9F90(*(float *)(data + 0x28));
        ret = func_001FA898_r(D_L06_00162378 * q2);
        {
        int three = 3;
        v21 = ((((ret / three) + 0xD) << 16) | ((((ret / 2) + 0x78) << 8) | 0x20000000)) | (ret + 0x96);
        }
        if (*(unsigned char *)(D_0013E633 + 0x2EC1) == 1) {
            r14 = *(int *)(data + 0x14);
            if (r14 < 10) {
                func_L01_00286530(buf, moby, 0.5f, r14 + 2, v21, 0x7F, 4, 0xFF);
            } else {
                if (D_0015EEB4_m[0] != 0) {
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    q = func_001F9F90(x) * 0.150000006f;
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    buf[4] = q;
                    q = func_001F9FA8(x) * 0.150000006f;
                    buf[5] = q;
                    buf[6] = 0.0f;
                    func_L00_001FF240(buf + 8, buf + 4, buf);
                    func_L01_00286530(buf + 4, moby, 0.5f, r14 / 10 + 2, v21, 0x7F, 4, 0xFF);
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    q = func_001F9F90(x) * -0.400000006f;
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    buf[4] = q;
                    q = func_001F9FA8(x) * -0.400000006f;
                    buf[5] = q;
                    buf[6] = 0.0f;
                    func_L00_001FF240(buf + 8, buf + 4, buf);
                    r14 = *(int *)(data + 0x14);
                    func_L01_00286530(buf + 4, moby, 0.5f, r14 % 10 + 2, v21, 0x7F, 4, 0xFF);
                } else {
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    q = func_001F9F90(x) * -0.400000006f;
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    buf[4] = q;
                    q = func_001F9FA8(x) * -0.400000006f;
                    buf[5] = q;
                    buf[6] = 0.0f;
                    func_L00_001FF240(buf + 8, buf + 4, buf);
                    func_L01_00286530(buf + 4, moby, 0.5f, r14 / 10 + 2, v21, 0x7F, 4, 0xFF);
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    q = func_001F9F90(x) * 0.150000006f;
                    x = func_001FA748(*(float *)(moby + 0x48), 1.57079637f);
                    buf[4] = q;
                    q = func_001F9FA8(x) * 0.150000006f;
                    buf[5] = q;
                    buf[6] = 0.0f;
                    func_L00_001FF240(buf + 8, buf + 4, buf);
                    r14 = *(int *)(data + 0x14);
                    func_L01_00286530(buf + 4, moby, 0.5f, r14 % 10 + 2, v21, 0x7F, 4, 0xFF);
                }
            }
        }
    }
    r14 = *(int *)(data + 0x14);
    if (r14 > 0) {
        if (*(unsigned char *)(D_0013E633 + 0x2EC1) != 0xFF) {
            return;
        }
    }
    p = data;
    for (i = 3; i >= 0; i--) {
        if (*(int *)p != -1) {
            func_L06_00309088((unsigned char *)moby, (unsigned char *)D_L06_00160058 + (*(int *)p << 8));
        }
        p += 4;
    }
    *(unsigned char *)(moby + 0x20) = 2;
}
