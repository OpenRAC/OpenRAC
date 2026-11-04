extern char D_00189E20[];
extern int D_001A79F0;
extern int D_L00_001B19C8;
extern char D_L00_001BF228[];
extern float func_00282C48(float);
extern void func_L00_002B47A8(void *, void *, int, float, float);
extern int func_002761F8(void *, void *, int, void *, int);
extern short func_00278E78(void);
extern void func_L00_002C42E0(int, int);
extern int func_L00_002B9B88(int, int);

/* Decide whether the resident should transition after falling or leaving support. */
int func_L00_002C8E10(void)
{
    float start[4], end[4];
    float range;
    char *p = D_00189E20;
    int falling;

    if (*(int *)(p + 0x2294) == 0x74 || *(int *)(p + 0x2294) == 0x67)
        return 0;
    falling = 0;
    if (*(unsigned char *)(p + 0x1494)) {
        int motion = *(int *)(p + 0x229C);
        if (motion == 0x11) {
            if (*(int *)(p + 0x2298) != 1 && *(float *)(p + 0x118) > 0.0f
                    && *(float *)(p + 0x88) > *(float *)(p + 0x330) - 0.4f)
                falling = 1;
        } else if (motion != 0x12 && motion != 3) {
            range = 0.2f;
            if (range < func_00282C48(*(float *)(p + 0xF8)) + 0.07f)
                range = func_00282C48(*(float *)(p + 0xF8)) + 0.07f;
            if (func_00282C48(*(float *)(p + 0x330) - (*(float *)(p + 0x88) + 0.45f)) < range
                    && (*(int *)(p + 0x229C) != 4 || *(unsigned char *)(p + 0x49E))
                    && *(float *)(p + 0x24C4) > 0.8f && *(float *)(p + 0x118) < 0.0f)
                falling = 1;
            {
                char *q = D_00189E20;

                if (*(float *)(q + 0x88) < *(float *)(q + 0x330) - 0.8f
                        && (*(int *)(q + 0x229C) != 4 || *(unsigned char *)(q + 0x49E)))
                    falling = 1;
            }
        }
    }
    p = D_00189E20;
    range = 0.27f;
    if (range < func_00282C48(*(float *)(p + 0xF8)) + 0.07f)
        func_00282C48(*(float *)(p + 0xF8));
    if (*(int *)(p + 0x2294) == 0x12 && *(unsigned char *)(p + 0x49E)
            && *(float *)(p + 0x88) < *(float *)(p + 0x330) - 0.7f)
        falling = 1;
    if (D_001A79F0 != 0xC) {
        char *resident = D_00189E20;
        int motion = *(int *)(resident + 0x229C);
        if (motion != 0x11 && motion != 0x12 && motion != 7
                && *(int *)(resident + 0x2294) != 0x12 && (D_L00_001B19C8 & 15) == 0) {
            float distance = 4.0f;
            if (D_L00_001B19C8 % 0x3F == 0)
                distance = 16.0f;
            func_L00_002B47A8(start, end, 0, distance, 1.3f);
            if (func_002761F8(start, end, 2, *(void **)(resident + 0x2290), 0)) {
                *(short *)(resident + 0x1490) = func_00278E78();
                if (*(short *)(resident + 0x1490) == 0) {
                    start[2] = *(float *)D_L00_001BF228 - 0.01f;
                    if (!func_002761F8(start, end, 2, *(void **)(resident + 0x2290), 0)) {
                        if (*(int *)(resident + 0x229C) == 0x14 || *(int *)(resident + 0x24CC) == 0)
                            func_L00_002C42E0(0x67, 1);
                        else
                            func_L00_002C42E0(0x33, 1);
                        return 1;
                    }
                }
            }
        }
    }
    if (falling) {
        char *fallState = D_00189E20;
        if (*(int *)(fallState + 0x229C) == 0x14 || *(int *)(fallState + 0x24CC) == 0)
            func_L00_002C42E0(0x67, 1);
        else
            func_L00_002C42E0(0x36, 1);
        func_L00_002B9B88(3, 0);
    }
    return falling;
}
