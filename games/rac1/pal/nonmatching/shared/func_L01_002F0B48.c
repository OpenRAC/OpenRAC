/* NON_MATCHING func_L01_002F0B48 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: BYTES 5/788 (99.4% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini40: started from staged17/788. First copy qcopy_nc p0 retains point address acrosscopy and reduces5/788, s
 */
typedef struct { char pad0[0x250]; float jitter; } DebrisSettings_2F0B48;
typedef struct { char pad0[0x10]; float position[4]; char pad20[0x58]; DebrisSettings_2F0B48 *data; } DebrisSource_2F0B48;
extern short D_L01_00161A7C, D_L01_00161A80, D_L01_00161A84, D_L01_00161A88, D_L01_00161A8C, D_L01_00161A90;
extern short D_L01_00161A94, D_L01_00161A98, D_L01_00161A9C, D_L01_00161AA0, D_L01_00161AA4, D_L01_00161AA8;
extern short D_L01_00161AAC, D_L01_00161AB0, D_L01_00161AB4, D_L01_00161AB8, D_L01_00161ABC, D_L01_00161AC0;
extern short D_L01_00161AC4, D_L01_00161AC8;
extern float D_0015EE6C_x __asm__("D_0015EE6C") MACRO_ADDR;
extern float D_0015EE70_x __asm__("D_0015EE70") MACRO_ADDR;
extern void func_L00_00260958(float *v, float s);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9F90(float);
extern int func_001F9850(int);
extern float func_001F9878(float);
extern int func_001FA8A8_y(int, int, float) __asm__("func_001FA8A8");
extern int func_001FA898_x(float) __asm__("func_001FA898");
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);

/* Table-driven debris burst (same shape as func_L05_002DC4C8; the start jitter scales with d->250): throws pieces along dir with random
 * spread, each trailing smoke puffs; counts, speeds, colours and lifetimes come from the level table. */
void func_L01_002F0B48(DebrisSource_2F0B48 *m, float *dir) {
    float p[4];
    float v[4];
    float a[4];
    DebrisSettings_2F0B48 *d = m->data;

    int i, j;
    for (i = 0; i < *(int *)&D_L01_00161A7C; i++) {
        float *pos = m->position;
        float ang, sp, up;
        qcopy_nc(p, pos);
        func_L00_00260958(p, *(float *)&D_L01_00161A84 * d->jitter);
        p[2] += 1.0f;
        ang = func_00214158();
        sp = func_002140F8(*(float *)&D_L01_00161A90, *(float *)&D_L01_00161A94);
        up = func_002140F8(*(float *)&D_L01_00161A98, *(float *)&D_L01_00161A9C);
        func_001F9C30(v, dir, func_002140F8(0.0f, *(float *)&D_L01_00161AC8) * D_0015EE6C_x);
        v[0] += func_001F9F90(ang) * (sp * D_0015EE6C_x);
        v[1] += func_001F9FA8(ang) * (sp * D_0015EE6C_x);
        v[2] += up * D_0015EE6C_x;
        qcopy(a, v);
        a[2] -= *(float *)&D_L01_00161AA0 * D_0015EE70_x * (float)func_001F9850(*(int *)&D_L01_00161AC0);
        for (j = 0; j < *(int *)&D_L01_00161A80; j++) {
            float r = func_002140F8(0.5f, 1.5f);
            int c1, c2, n1, n2;
            v[3] = r * *(float *)&D_L01_00161AA4;
            a[3] = r * *(float *)&D_L01_00161AA8;
            func_L00_00260958(p, *(float *)&D_L01_00161A88);
            func_L00_00260958(a, *(float *)&D_L01_00161A8C * D_0015EE6C_x);
            c1 = func_001FA8A8_y(*(int *)&D_L01_00161AAC, *(int *)&D_L01_00161AB4, func_002140F8(0.0f, 1.0f));
            c2 = func_001FA8A8_y(*(int *)&D_L01_00161AB0, *(int *)&D_L01_00161AB8, func_002140F8(0.0f, 1.0f));
            n1 = func_001F9850(*(int *)&D_L01_00161ABC);
            n2 = func_001F9850(*(int *)&D_L01_00161AC0);
            func_00219780(p, v, a, c1, c2, n1, n2,
                          func_001FA898_x(func_001F9878(func_002140F8((float)*(int *)&D_L01_00161AC4 * 0.5f,
                                                                       (float)*(int *)&D_L01_00161AC4 * 2.5f))), -1);
        }
    }
}
