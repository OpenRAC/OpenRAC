/* NON_MATCHING func_L06_002FEB78 -- src/overlays/l06_blarg/vendor_002FE5D0.c
 * Best so far: SIZE ours 732 / retail 740, checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini16 a02: transforms and submits front/back sky-mesh faces. Best equal-size p6.c BYTES 91/740; typed camera 
 *   Stopped budget 8/8. Left spilled vector-pointer initialization, repeated matrix pointer reload, inner index ba
 */
extern int func_001F4868(int);
extern void func_001FA190(void *);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9EE8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9EC0(void *, void *, void *);
extern float func_001F9C78(void *, void *);
extern void func_L00_001FD1D8(void *, void *, int);
extern char D_L06_00167500[];
extern float D_L06_001EBA80[][4];
extern char D_L06_00167640[];
extern float D_L06_001E4D80[][4];
extern float D_L06_001E3040[][2];
extern short D_L06_001DFA40[][8];
extern float D_L06_001DC340[][4];
extern short D_L06_00162030, D_L06_00162034, D_L06_00162020, D_L06_00162010, D_L06_0016200C, D_L06_00162014, D_L06_00162018, D_L06_0016201C, D_L06_00162024, D_L06_00162028, D_L06_0016202C;
typedef struct { float vertices[4][4]; int color[4]; float uv[4][2]; long header[4]; } Surface06;

/* transforms and submits front and back faces of the sky mesh */
void func_L06_002FEB78(void) {
    Surface06 front, back;
    float matrix[16], normal[4], dir[4];
    float u = *(float *)&D_L06_00162030 * *(float *)(D_L06_00167500 + 0x158);
    float v = *(float *)&D_L06_00162034 * *(float *)(D_L06_00167500 + 0x154);
    long flags;
    int i, j;
    front.header[1] = func_001F4868(*(int *)&D_L06_00162020);
    back.header[1] = func_001F4868(42);
    flags = (long)*(int *)&D_L06_0016200C | ((long)*(int *)&D_L06_00162010 << 2) |
            ((long)*(int *)&D_L06_00162014 << 4) | ((long)*(int *)&D_L06_00162018 << 6) |
            ((long)*(int *)&D_L06_0016201C << 32);
    front.header[3] = back.header[3] = flags;
    front.header[2] = back.header[2] = 0xFF9000000260L;
    front.header[0] = back.header[0] = 0;
    func_001FA190(matrix);
    front.color[0] = front.color[1] = front.color[2] = front.color[3] = *(int *)&D_L06_00162024;
    back.color[0] = back.color[1] = back.color[2] = back.color[3] = *(int *)&D_L06_00162028;
    for (i = 0; i < 0x360; i++) {
        func_001F9C30(dir, D_L06_001EBA80[i], *(float *)&D_L06_0016202C);
        func_001F9EE8(dir, dir, matrix);
        func_001F9BF0(normal, dir, D_L06_00167640);
        func_L00_001FF4B0(normal, normal, 1.0f);
        func_001F9EC0(dir, D_L06_001E4D80[i], matrix);
        if (func_001F9C78(normal, dir) <= 0.0f) {
            for (j = 0; j < 4; j++) {
                int vertex = D_L06_001DFA40[i][j * 2];
                int tex = D_L06_001DFA40[i][j * 2 + 1];
                func_001F9C30(front.vertices[j], D_L06_001DC340[vertex], *(float *)&D_L06_0016202C);
                qcopy(back.vertices[j], front.vertices[j]);
                front.uv[j][0] = D_L06_001E3040[tex][0] + u;
                front.uv[j][1] = D_L06_001E3040[tex][1] + v;
                back.uv[j][0] = D_L06_001E3040[tex][0];
                back.uv[j][1] = D_L06_001E3040[tex][1];
            }
            func_L00_001FD1D8(&front, matrix, 0);
            func_L00_001FD1D8(&back, matrix, 0);
        }
    }
}
