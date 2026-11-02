typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    TI128 v0, v1, v2, v3;
    char pad[0x20];
} Local51C0;

extern char *func_0029EFD0(int);
extern void func_002833F0(void *, void *);
extern int func_002AFFD8(void *, void *, int);
extern void func_00282F20(void *, void *, float);
extern void func_00282DD8(void *, void *, void *);
extern float func_00282DB0(void *, void *);
extern float func_00283198(float);
extern void func_00282D30(void *, void *, float);
extern float func_00283180(float);
extern void func_00283958(void *, void *);
extern void func_00283738(void *, void *, void *);
extern void func_0029F338(void *);
extern char *D_L00_001B05DC __attribute__((sda));
extern char D_0018C0B0[];

char *func_L00_003E51C0(TI128 *position, TI128 *direction)
{
    Local51C0 local;
    char *p = func_0029EFD0(0xC06);
    char *data = *(char **)(p + 0x68);
    char *basis;
    char *axis;
    char *point;
    float half;
    short header;

    if (p != 0) {
        header = 0xFF;
        *(short *)(p + 0x32) = header;
        *(char *)(p + 0x30) = header;
        *(unsigned short *)(p + 0x34) |= 0x100;
        *(char *)(p + 0x20) = 0;
        point = p + 0x10;
        *(TI128 *)point = *position;
        *(TI128 *)data = *direction;
        basis = p + 0xC0;
        axis = p + 0xE0;
        func_002833F0(basis, *(char **)D_0018C0B0 + 0xF0);
        func_002AFFD8(point, &local.v2, 1);
        func_00282F20(&local.v2, &local.v2, 1.0f);
        func_00282DD8(&local.v1, axis, &local.v2);
        half = func_00282DB0(axis, &local.v2) * 0.5f;
        func_00282D30(&local.v0, &local.v1, func_00283198(half));
        *(float *)((char *)&local.v0 + 12) = func_00283180(half);
        func_00283958(&local.v0, &local.v3);
        func_00283738(basis, &local.v3, basis);
        if (D_L00_001B05DC != 0)
            func_0029F338(D_L00_001B05DC);
        D_L00_001B05DC = p;
    }
    return p;
}
