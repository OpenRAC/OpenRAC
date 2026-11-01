typedef struct { char pad0[0x14]; int f14; char pad18[0x10]; } E;
extern int func_L05_0037AA88(int);
extern E ref_L05_0037AC00_00[];
int func_L05_0037AC00(void) {
    int r = func_L05_0037AA88(6);
    if (r < 0) goto z;
    return ref_L05_0037AC00_00[r].f14;
z:  return 0;
}
