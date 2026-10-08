/* NON_MATCHING func_L06_002EB5C8 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: BYTES 13/768 (98.3% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern short D_L06_00161CA8, D_L06_00161CAC, D_L06_00161CB0, D_L06_00161CB4, D_L06_00161CB8, D_L06_00161CBC;
extern short D_L06_00161CC0, D_L06_00161CC4, D_L06_00161CC8, D_L06_00161CCC, D_L06_00161CD0, D_L06_00161CD4;
extern short D_L06_00161CD8, D_L06_00161CDC, D_L06_00161CE0, D_L06_00161CE4, D_L06_00161CE8, D_L06_00161CEC;
extern short D_L06_00161CF0, D_L06_00161CF4;
extern float D_0015EE6C_x __asm__("D_0015EE6C") MACRO_ADDR;
extern float D_0015EE70_x __asm__("D_0015EE70") MACRO_ADDR;
extern void func_L00_00260958(float *v, float s);
extern float func_00214158(void);
extern float func_002140F8(float, float);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9F90(float);
extern int func_001F9850(int);
extern float func_001F9878(float);
extern int func_001FA898_x(float) __asm__("func_001FA898");
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);

/* Table-driven debris burst (same shape as func_L05_002DC4C8): throws pieces along dir with random
 * spread, each trailing smoke puffs; counts, speeds, colours and lifetimes come from the level table. */
void func_L06_002EB5C8(void *m, void *dir) {
    float p[4];
    float v[4];
    float a[4];

    int i, j;
    for (i = 0; i < *(int *)&D_L06_00161CA8; i++) {
        char *pos = (char *)m + 0x10;
        float ang, sp, up;
        qcopy(p, pos);
        func_L00_00260958(p, *(float *)&D_L06_00161CB0);
        p[2] += 0.25f;
        ang = func_00214158();
        sp = func_002140F8(*(float *)&D_L06_00161CBC, *(float *)&D_L06_00161CC0);
        up = func_002140F8(*(float *)&D_L06_00161CC4, *(float *)&D_L06_00161CC8);
        func_001F9C30(v, dir, func_002140F8(0.0f, *(float *)&D_L06_00161CF4) * D_0015EE6C_x);
        v[0] += func_001F9F90(ang) * (sp * D_0015EE6C_x);
        v[1] += func_001F9FA8(ang) * (sp * D_0015EE6C_x);
        v[2] += up * D_0015EE6C_x;
        qcopy(a, v);
        a[2] -= *(float *)&D_L06_00161CCC * D_0015EE70_x * (float)func_001F9850(*(int *)&D_L06_00161CEC);
        for (j = 0; j < *(int *)&D_L06_00161CAC; j++) {
            float r = func_002140F8(0.5f, 1.5f);
            int c1, c2, n1, n2;
            v[3] = r * *(float *)&D_L06_00161CD0;
            a[3] = r * *(float *)&D_L06_00161CD4;
            func_L00_00260958(p, *(float *)&D_L06_00161CB4);
            func_L00_00260958(a, *(float *)&D_L06_00161CB8 * D_0015EE6C_x);
            c1 = func_001FA8A8(*(int *)&D_L06_00161CD8, *(int *)&D_L06_00161CE0, func_002140F8(0.0f, 1.0f));
            c2 = func_001FA8A8(*(int *)&D_L06_00161CDC, *(int *)&D_L06_00161CE4, func_002140F8(0.0f, 1.0f));
            n1 = func_001F9850(*(int *)&D_L06_00161CE8);
            n2 = func_001F9850(*(int *)&D_L06_00161CEC);
            func_00219780(p, v, a, c1, c2, n1, n2,
                          func_001FA898_x(func_001F9878(func_002140F8((float)*(int *)&D_L06_00161CF0 * 0.5f,
                                                                       (float)*(int *)&D_L06_00161CF0 * 2.5f))), -1);
        }
    }
}
