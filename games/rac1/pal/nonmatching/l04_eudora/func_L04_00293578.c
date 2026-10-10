/* NON_MATCHING func_L04_00293578 -- src/overlays/l04_eudora/vuchain_00293490.c
 * Best so far: SIZE ours 712 / retail 716, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
typedef struct {
    int id;
    float speed;
    float rate;
    float start;
    float len;
    char pad14[0xC];
    float d20[2];
    float a28[2];
    float b30[2];
    float d38[2];
    float a40[2];
    float b48[2];
} AnimCh_293578;
extern char D_L04_001DB660[];
extern char D_L04_001DB6F0[];
extern float func_L04_00242868(int a, unsigned int b);
extern float func_L04_002428C8(int a, unsigned int b);
extern float func_001F9878(float);
extern void func_001F9978(void);
extern int func_001E9730();
extern void func_0020DB98(char *arg0, int arg1, void *arg2, char *arg3);
extern void func_0020DAF8(void *, int, void *);
extern void func_0020D960(char *, int, void *);
extern float func_L00_001FF860(float, float);

/* Sets up the moby's 13 animation channels (start/length of each clip and the wrapped spans between its
 * key ranges), its two limb joints, and resets the animation state. */
void func_L04_00293578(char *m, char *d) {
    int ids[6];
    float mat[16];
    float *mp = mat;
    char *tab = d + 0x70;
    int i;
    char *p;
    for (i = 0; i < 13;) {
        int o = i << 2;
        if ((*(AnimCh_293578 **)(tab + o))->id < 0) {
            func_001E9730(D_L04_001DB660, i, *(short *)(m + 0xA6));
            func_001F9978();
        }
        i++;
        (*(AnimCh_293578 **)(tab + o))->start = func_L04_00242868(((unsigned char *)m)[0x22], (*(AnimCh_293578 **)(tab + o))->id);
        (*(AnimCh_293578 **)(tab + o))->len = func_L04_002428C8(((unsigned char *)m)[0x22], (*(AnimCh_293578 **)(tab + o))->id) - (*(AnimCh_293578 **)(tab + o))->start;
        if ((*(AnimCh_293578 **)(tab + o))->len != 0.0f) {
            int k;
            for (k = 0; k < 2; k++) {
                if ((*(AnimCh_293578 **)(tab + o))->a40[k] <= (*(AnimCh_293578 **)(tab + o))->b48[k]) {
                    (*(AnimCh_293578 **)(tab + o))->d38[k] = (*(AnimCh_293578 **)(tab + o))->b48[k] - (*(AnimCh_293578 **)(tab + o))->a40[k];
                } else {
                    (*(AnimCh_293578 **)(tab + o))->d38[k] = (*(AnimCh_293578 **)(tab + o))->b48[k] + (*(AnimCh_293578 **)(tab + o))->len - (*(AnimCh_293578 **)(tab + o))->a40[k];
                }
                if ((*(AnimCh_293578 **)(tab + o))->a28[k] <= (*(AnimCh_293578 **)(tab + o))->b30[k]) {
                    (*(AnimCh_293578 **)(tab + o))->d20[k] = (*(AnimCh_293578 **)(tab + o))->b30[k] - (*(AnimCh_293578 **)(tab + o))->a28[k];
                } else {
                    (*(AnimCh_293578 **)(tab + o))->d20[k] = (*(AnimCh_293578 **)(tab + o))->b30[k] + (*(AnimCh_293578 **)(tab + o))->len - (*(AnimCh_293578 **)(tab + o))->a28[k];
                }
            }
            (*(AnimCh_293578 **)(tab + o))->rate = (*(AnimCh_293578 **)(tab + o))->speed / func_001F9878((*(AnimCh_293578 **)(tab + o))->len + (*(AnimCh_293578 **)(tab + o))->len);
        }
    }
    ids[0] = ((unsigned char *)d)[0xF0];
    ids[1] = ((unsigned char *)d)[0xF1];
    ids[2] = ((unsigned char *)d)[0x1A0];
    ids[3] = ((unsigned char *)d)[0x1A1];
    ids[4] = ((unsigned char *)d)[0xB4];
    ids[5] = ((unsigned char *)d)[0xB5];
    func_0020DB98(m, 6, ids, d);
    for (i = 0; i < 2; i++) {
        p = d + i * 0xB0;
        func_0020DAF8(m, ((unsigned char *)p)[0xF0], mp);
        *(int *)(p + 0x104) = 0;
        *(int *)(p + 0xFC) = 0;
        *(float *)(p + 0xF8) = func_L00_001FF860(mat[4], mat[5]);
        if (((unsigned char *)p)[0x161] == 0) {
            func_0020D960(m, ((unsigned char *)p)[0xF3], p + 0x160);
        }
        if (((unsigned char *)p)[0x121] == 0) {
            func_0020D960(m, ((unsigned char *)p)[0xF2], p + 0x120);
        }
    }
    *(short *)(d + 0xEE) = 0;
    d[0xB7] = 0;
    *(int *)(d + 0xB8) = 0;
    *(int *)(d + 0xBC) = 0;
    *(int *)(d + 0xC0) = 0;
    *(int *)(d + 0xC8) = 0;
    *(int *)(d + 0xE8) = 0;
    *(int *)(d + 0xD8) = 0;
    *(int *)(d + 0xE4) = 0;
    *(short *)(d + 0xEC) = 0;
    d[0xB6] = 5;
    func_001E9730(D_L04_001DB6F0, 0x250);
}
