/* Transforms the two vectors by the object matrix (the second one also lifted by 2) and reports success when the object is in step 5 with its marker value. The two small-data vectors are 8-byte sda arrays so that their addresses come out as `addiu $gp`. */
extern float D_L12_001B1C00[2] __attribute__((sda));
extern float D_L12_001B1C10[2] __attribute__((sda));
extern void func_002F1E08(float *, float *, char *);
extern void func_002F1928(float *, float *, char *);
extern float func_0031F718(float, float *, int);

int func_L12_00419D68(char *obj, float *a, float *b) {
    char *m;
    char *r;
    if (*(short *)(obj + 0xAA) != 0xFA5 || *(unsigned char *)(obj + 0x20) != 5) {
        return 0;
    } else {
        m = obj + 0xC0;
        func_002F1E08(b, D_L12_001B1C00, m);
        r = obj + 0x10;
        func_002F1928(b, b, r);
        func_002F1E08(a, D_L12_001B1C10, m);
        func_002F1928(a, a, r);
        a[2] = a[2] + 2.0f;
        a[2] = func_0031F718(0.5f, a, 0);
    }
    return 1;
}
