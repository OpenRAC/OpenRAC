/* NON_MATCHING func_L16_002E4C08 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: SIZE ours 1652 / retail 1664, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Run 4 BYTES110/1664: separate zero assignment after qcopy recovers f20 and exact size. Remaining s2/s3 swap, f
 *   Run 5 BYTES107/1664: integer-first effect alias fixed argument swap; explicit speeds changed effect scheduling
 *   Run 6 COMPILE: field replacement also caught padding name and placed a declaration after assignment; fixed in 
 *   Run 7 BYTES103/1664: typed data unchanged codegen; chained reset exactly fixes all reset-store offsets.
 *   Run 8 BYTES102/1664: separate step assignment and flags-before-data retained exact size; saved registers uncha
 *   Run 9 BYTES112/1664: reused step/horizontal locals and integer-first turn did not improve scheduling; reverted
 *   Run 10 BYTES103/1664: outer step and reordered fields improve time register; separate heading assignment did n
 *   Run 11 BYTES112/1664: precomputed constants unchanged from p8; best p7 is BYTES102/1664. Saved-register s2/s3 
 */
typedef struct { char pad[0xD0]; int counter; float timer; int path, other_path, trigger; float vx, vy, vz, turn; char *child; int unused, mode; } L16CrateData;
typedef struct { char pad[0x80]; float position[4]; char pad90[0x1AC]; void *standing; } L16CratePlayer;
extern L16CratePlayer D_0013E633_crate __asm__("D_0013E633");
extern char D_L16_001E89A0[], D_L16_001E89D8[], D_L16_001E8A20[], D_L16_001E8A58[], D_L16_001E8AA0[];
extern short D_L16_00161E54, D_L16_00161E58;
extern char *D_L16_001B0C30[] MACRO_ADDR;
extern void func_00213DE0(void *, int, int, int);
extern void func_00213D28(void *, int, int);
extern int func_001E9730();
extern void func_L14_002FFD88(void *);
extern void func_L00_0025D5B0(float, void *, void *, int, int, int);
extern void func_L00_002592B0_crate(float, float, float, float, void *, void *) __asm__("func_L00_002592B0");
extern char *func_L14_00300F80(void *, void *);
extern void func_L14_003000B0(void *);
extern void func_L14_00301090(void *);
extern void func_L14_00300130(void *);
extern int func_L16_002E5408(void *);
extern float func_L00_0025C918(void *, void *, float, float, float, float);
extern int func_L00_0025D6F0(void *, void *);
extern void func_L00_00260108(void *, void *, int, float, float);
extern void func_L00_0025E590(void *, void *);

/* Update the triggered moving platform and its linked moby. */
void func_L16_002E4C08(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    char *hit;
    if ((m[0x70] & 2) && m[0x52] == m[0x53] && m[0x52] == 1)
        func_00213DE0(m, 4, 0, func_001F9850(5));
    if (((L16CratePlayer *)((char *)&D_0013E633_crate + 0xE1D))->standing == m && m[0x53] != 1 && m[0x52] != 1)
        func_00213DE0(m, 1, 0, func_001F9850(5));
    hit = func_L00_0025B478(m, 0x210000, 0);
    if (!hit && *(char **)(d + 0xF4))
        hit = func_L00_0025B478(*(char **)(d + 0xF4), 0x210000, 0);
    if (hit) {
        float step = D_0015EE6C;
        if (m[0x20] != 6) {
        char *motion = d + 0x70;
        *(float *)(d + 0x80) = 0.008f;
        *(int *)(d + 0x94) = 1;
        *(float *)(d + 0x8C) = step * 12.0f;
        *(float *)(d + 0x84) = 0.0005f;
        *(float *)(d + 0x88) = step * 24.0f;
        d[0xAD] = 0;
        func_L00_0025D5B0(func_L00_001FF860(*(float *)(m + 0x10) - ((L16CratePlayer *)((char *)&D_0013E633_crate + 0xE1D))->position[0], *(float *)(m + 0x14) - ((L16CratePlayer *)((char *)&D_0013E633_crate + 0xE1D))->position[1]), m, motion, 1, 1, 0);
        d[0x67] = 0x78;
        func_L00_002584A8(m, 0, -1);
        m[0x20] = 6;
    }
    }
    m[0xA4] = 0xFF;
    switch (m[0x20]) {
    case 0:
        if (*(int *)(d + 0xFC) == 2) {
            m[0x20] = 1;
            *(int *)(d + 0xF0) = 0;
            func_00213D28(m, 4, 0);
            *(char **)(d + 0xF4) = 0;
            break;
        }
        if (*(int *)(d + 0xD8) == -1) {
            func_001E9730(D_L16_001E89A0, *(short *)(m + 0xB2));
            goto remove;
        }
        if (*(int *)D_L16_001B0C30[*(int *)(d + 0xD8)] == 0) {
            func_001E9730(D_L16_001E89D8, *(short *)(m + 0xB2));
            goto remove;
        }
        if (*(int *)(d + 0xFC) != 1) {
            if (*(int *)(d + 0xDC) == -1) {
                func_001E9730(D_L16_001E8A20, *(short *)(m + 0xB2));
                goto remove;
            }
            if (*(int *)D_L16_001B0C30[*(int *)(d + 0xDC)] == 0) {
                func_001E9730(D_L16_001E8A58, *(short *)(m + 0xB2));
                goto remove;
            }
        }
        if (*(int *)(d + 0xE0) == -1) {
            func_001E9730(D_L16_001E8AA0, *(short *)(m + 0xB2));
            goto remove;
        }
        func_L14_002FFD88(m);
        m[0x20] = 2;
        m[0x30] = 0x80;
        *(unsigned short *)(m + 0x34) |= 0x41;
        *(int *)(m + 0x94) = 0;
        *(char **)(d + 0xF4) = 0;
        break;
    case 1:
        func_L00_002592B0_crate(func_L00_001FF860(((L16CratePlayer *)((char *)&D_0013E633_crate + 0xE1D))->position[0] - *(float *)(m + 0x10), ((L16CratePlayer *)((char *)&D_0013E633_crate + 0xE1D))->position[1] - *(float *)(m + 0x14)), 0.005f, 0.2f, 0.0f, m, d + 0xF0);
        goto animate;
    case 2:
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(d + 0xE0))) {
            if (*(int *)(d + 0xFC) == 1) func_00213D28(m, 0, 0);
            else {
                *(char **)(d + 0xF4) = func_L14_00300F80(m, m + 0x10);
                func_L14_003000B0(m);
                func_00213D28(m, 2, 0);
            }
            *(unsigned short *)(m + 0x34) &= 0xFFBE;
            *(int *)(m + 0x94) = *(int *)(*(char **)(m + 0x24) + 0x10);
            m[0x20] = 4;
        }
        break;
    case 4:
        if (*(int *)(d + 0xFC) == 0) func_L14_003000B0(m);
        if (func_L16_002E5408(m)) {
            m[0x20] = 3;
            func_00213DE0(m, 3, 0, 5);
        }
        goto animate;
    case 3: {
        L16CrateData *a = *(L16CrateData **)(m + 0x78);
        char *path = D_L16_001B0C30[a->path];
        float target[4];
        float zero = 0.0f;
        if (a->mode == 0) func_L14_003000B0(m);
        qcopy(target, path + *(int *)path * 16);
        func_L00_0025C918(m + 0x10, &a->vx, target[0], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, zero);
        func_L00_0025C918(m + 0x14, &a->vy, target[1], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, zero);
        func_L00_0025C918(m + 0x18, &a->vz, target[2], *(float *)&D_L16_00161E54, *(float *)&D_L16_00161E58, zero);
        if ((m[0x70] & 2) && m[0x52] == m[0x53]) {
            int mode = a->mode;
            if (mode == 1) {
                m[0x20] = mode;
                a->turn = zero;
                func_00213D28(m, 4, 0);
            } else {
                if (a->child) {
                    func_L14_00301090(a->child);
                    a->child = 0;
                }
                m[0x20] = 5;
                func_00213DE0(m, 0, 0, 20);
                a->timer = zero;
                a->counter = 0;
            }
        }
        goto animate;
    }
    case 5:
        if (func_L16_002E5408(m)) {
            char *path;
            m[0x20] = 2;
            *(int *)(m + 0x94) = 0;
            *(unsigned short *)(m + 0x34) |= 0x41;
            *(int *)(d + 0xD4) = 0;
            *(int *)(d + 0xD0) = 0;
            *(float *)(d + 0xE4) = *(float *)(d + 0xD4);
            *(float *)(d + 0xE8) = *(float *)(d + 0xD4);
            *(float *)(d + 0xEC) = *(float *)(d + 0xD4);
            *(float *)(d + 0xF0) = *(float *)(d + 0xD4);
            path = D_L16_001B0C30[*(int *)(d + 0xD8)];
            qcopy(m + 0x10, path + 0x10);
            *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(path + 0x20) - *(float *)(path + 0x10), *(float *)(path + 0x24) - *(float *)(path + 0x14));
            break;
        }
animate:
        func_L14_00300130(m);
        goto finish;
    case 6:
        if (func_L00_0025D6F0(m, d + 0x70) & 3) {
            func_L00_00260108(m, m + 0x10, -1, 1.0f, 13.0f);
            goto remove;
        }
        if (*(float *)(m + 0x18) < 5.0f) {
remove:
            func_0020D678(m);
            return;
        }
    }
finish:
    func_L00_0025E590(m, d + 0x60);
    return;
}
