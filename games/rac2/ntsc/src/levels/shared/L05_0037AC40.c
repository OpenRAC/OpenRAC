typedef struct { char pad0[0x10]; int f10; char pad14[0x14]; } E;
extern int func_L05_0037AA88(int);
extern E D_002A1800[];
int func_L05_0037AC40(void) {
    int r = func_L05_0037AA88(6);
    if (r < 0) goto z;
    return D_002A1800[r].f10;
z:  return 0;
}
