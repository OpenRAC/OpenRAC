/* Shuts the scene down: runs the helpers, clears the resident flag and, when the scene record has no active entry, lifts the copy of its next record (third component adjusted) before the final transition. */
typedef int TI128 __attribute__((mode(TI)));
extern char D_00189E20[];
extern char D_00152C00[];
extern void func_002B7320(void);
extern void func_0026F728(void);
extern void func_0026F750(int);
extern void func_0026F798(void);
extern void func_0026F778(char *, void *, int, int);
extern void func_00272198(float);
extern void func_002B8518(void);
extern float func_00283B98(float, float);

void func_002E7248(void) {
    char *p;
    char *scene;
    float v[4] __attribute__((aligned(16)));

    func_002B7320();
    p = D_00189E20;
    p[0x22B5] = 0;
    func_0026F728();
    scene = D_00152C00;
    if (*(short *)(scene + 0x2A) != 0) {
        func_0026F750(1);
        p[0x22B5] = 0;
        func_0026F728();
        func_0026F798();
        func_00272198(8.0f);
        func_002B8518();
    } else {
        p = scene + 0x80;
        *(TI128 *)v = *(TI128 *)p;
        v[2] = func_00283B98(v[2], -2.3f);
        scene += 0x70;
        func_0026F778(scene, v, 0, 1);
        func_0026F778(scene, p, 0, 0);
        func_00272198(8.0f);
        func_002B8518();
    }
}
