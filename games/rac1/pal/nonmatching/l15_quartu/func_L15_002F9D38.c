/* NON_MATCHING func_L15_002F9D38 -- src/overlays/l15_quartu/vendor_002EDB50.c
 * Best so far: BYTES 26/700 (96.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char *D_L15_00167480_9d38 __asm__("D_L15_00167480");
extern char D_0013A5E0[];
extern int func_L00_002E9870(char *);
extern void func_L00_002E9838(char *);
extern int func_001F9850(int);
extern void func_L02_002F79D0(float, float, float);
extern float func_001FA888(int);
extern void func_L00_002E9E20(void *, float, float);
extern void func_L00_002E9900(int, float, float);
extern void func_L00_002E9AD0(void);
extern void func_L00_002E9968(float, float);
extern void func_L00_002E99A0(int, float, float);

/* Earthquake event: while active, rumbles the camera in random bursts and rocks the linked moby. */
void func_L15_002F9D38(void *mv) {
    char *m = mv;
    char *e = *(char **)(D_L15_0015F050 + *(short *)(m + 0x84) * 32 + 0x1C);
    char *t = D_L15_0016016C + *(int *)(e + 0x48) * 0x80;
    int on;
    int a, b, c, k;
    float s;
    float v;
    if (func_L00_002E9870(m) <= 0) {
        *(short *)(e + 0x20) = 0;
        return;
    }
    if (func_L15_002F99F8(m) == 0) {
        *(short *)(e + 0x20) = 0;
        func_L00_002E9838(m);
        return;
    }
    *(int *)(*(char **)(D_L15_00167480_9d38 + 0x70) + 0x230) = 2;
    on = 1;
    *(short *)(e + 0x20) += 1;
    a = func_001F9850(300);
    b = func_001F9850(400);
    c = func_001F9850(560);
    k = func_001F9850(200);
    func_L02_002F79D0(1.0f, 12.0f, 0.11f);
    {
        char *p = D_0013A5E0 + 0x2460;
        if (*(float *)(p + 0x100) != 0.0f || *(float *)(p + 0x104) != 0.0f) {
            *(short *)(e + 0x20) = a;
        }
    }
    if (*(short *)(e + 0x20) >= a) {
        *(int *)(e + 0x3C) += 1;
        if (*(int *)(e + 0x3C) < b) {
            on = 0;
        } else if (func_L15_002F9AE8(m, 30.0f, 0.0f) != 0) {
            *(short *)(e + 0x20) = c;
        } else {
            on = 0;
        }
    }
    if (*(short *)(e + 0x20) >= c) {
        on = 1;
        *(int *)(e + 0x3C) = 0;
        *(short *)(e + 0x20) = on;
    }
    if (k < *(short *)(e + 0x20) && *(short *)(e + 0x20) < a) {
        *(int *)(e + 0x3C) = a;
        *(short *)(e + 0x20) = k;
    }
    s = func_001FA888(*(short *)(e + 0x20)) / func_001FA888(k);
    v = *(float *)e * 0.017453292f * s;
    if (on) {
        func_L00_002E9E20(t + 0x30, v, 0.0f);
    }
    if (*(float *)(e + 0x34) != 0.0f) {
        func_L00_002E9900(0, *(float *)(e + 0x34), 0.003f);
        func_L00_002E9AD0();
    }
    if (*(float *)(e + 0x38) != 0.0f) {
        func_L00_002E9968(*(float *)(e + 0x38), 0.003f);
    }
    if (*(float *)(e + 0x44) != 0.0f) {
        func_L00_002E99A0(0, *(float *)(e + 0x44), 0.005f);
    }
}
