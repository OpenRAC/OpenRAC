/* Builds the object matrix and its inverse, brings the offset into the object frame and
   applies a step to the second vector whose sign follows the offset's first component. The
   step size is one of two resident floats. Every callee is a ref_ name: the retail keeps
   level-resident copies of the helpers. */
extern float D_L03_001B02D0 __attribute__((sda));
extern float D_L03_001B02D4 __attribute__((sda));
extern void ref_L03_003CA148_01(float *, char *);
extern void ref_L03_003CA148_02(float *, float *);
extern void ref_L03_003CA148_03(float *, float *, char *);
extern void ref_L03_003CA148_00(char *, float *, float *, int);
extern void ref_L03_003CA148_04(float *, float *, float);
extern void ref_L03_003CA148_05(float *, float *, char *);

void func_L03_003CA148(char *obj, float *a, float *b) {
    float v[4] __attribute__((aligned(16)));
    float m[16] __attribute__((aligned(16)));
    float n[16] __attribute__((aligned(16)));
    float s;

    ref_L03_003CA148_01(m, obj + 0xF0);
    ref_L03_003CA148_02(n, m);
    ref_L03_003CA148_03(v, a, obj + 0x10);
    ref_L03_003CA148_00(obj, v, v, 0);
    if (*(short *)(obj + 0xAA) == 0xE87) {
        s = D_L03_001B02D0;
    } else {
        s = D_L03_001B02D4;
    }
    if (v[0] < 0.0f) {
        s = -s;
    }
    ref_L03_003CA148_04(b, m, s);
    ref_L03_003CA148_05(b, b, obj + 0x10);
}
