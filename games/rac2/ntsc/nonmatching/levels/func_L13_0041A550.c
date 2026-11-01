/* opus-1 tier (v2060), USA v1.01: 1504/1504 bytes, 0/1 places match,
 * 77 of 376 rows differ, first difference +0xBC.
 * Profile: -O2 -G8 -ffunction-sections, in an isolated -g8.c unit.
 * RAC1 counterpart: plasmabot.c start_vendor_beam_pulse.
 * Recovered here: the char * spawn prototype (a true one, game/002BAA28.c
 * returns a pointer; ignored call_value) fixes the first spawn loop's
 * scheduling and the slti register; a reused local s17 for the colour pick
 * keeps the test branch alive.
 * Still differs:
 * 1. PRE spill slots. Retail: 0x470 moby+0x10, 0x474 beam+0x18 (cnt), 0x478
 *    work, 0x47C delta; here the two pairs are swapped, 0x480..0x48C agree.
 *    Needed bucket order (raw hashes 14129 < 14138 < 15070 < 15086), which
 *    holds for table sizes 103 107 111 113 125 127 129 133 141 147 151 153 155
 *    181 183 185 199 207 213 217 219 223 227 229 231 261. This form has 171,
 *    the one-armed form 169: 181 needs about +20 cuids, 155 about -30, both
 *    more than five initial values can give. Retail may have more dead code
 *    at gcse time; unproven.
 * 2. Second spawn loop scheduling. Retail: addiu a0,2; addiu s1,0x10;
 *    addiu s2,0x10; jal rand; swc1 f0,0x42C; jal rand; addiu a0,0xA. Here the
 *    increments land after rand(10) and addiu a0,0xA sits before the jal.
 *    A one-armed `if (func_002A77E0(2)) s17 = (D_L13_001B16C0);` gives retail's
 *    order there (flow1 empties the arm; flow2 appears to tidy the jump away,
 *    measured only through the schedule), but cse1's path then runs through the
 *    if, spawn args reuse pseudos 305/311, loop hoists them, and 141 of 379 rows
 *    differ (+12 bytes). Retail seems two-armed at cse1 and one-armed by flow2.
 *    `if (c) s17 = A; else unused = B;` gives the same 141 of 379 rows.
 * Next useful experiment: a real statement that blocks cse1's path through the
 * colour pick in the second loop and adds or removes enough cuids for a valid
 * table size. All declarations are below.
 */
#include "common.h"
typedef union { int q __attribute__((mode(TI))); float f[4]; } Vq_02C80;
typedef struct { int idx; char pad4[14]; short s12; char pad14[4]; unsigned char cnt[8]; Vq_02C80 pts[32],off[32],a[4][4],b[4][4]; int r620; char pad624[0x650-0x624]; float rotation[3]; } D_02C80;
extern int func_002A7820(int,int),func_002A77E0(int),func_00283CF0(float);
extern float func_002A7878(float,float),func_002A78D8(float,float),func_002A7940(void),func_00283CE0(int),func_00282E80(void *,void *);
extern void func_00282C88(void *),func_002A7A00(void *,float,float),func_00282CC0(void *,void *,void *),func_00282D30(void *,void *,float),func_00283410(void *,void *),func_002830C0(void *,void *,void *);
extern char *func_002BAA28(void *,void *,void *,int,int,int,int,int,int);
extern int D_L13_001B161C __attribute__((sda)),D_L13_001B1620 __attribute__((sda));
extern float D_L13_001B16A0 __attribute__((sda)),D_L13_001B16A4 __attribute__((sda)),D_L13_001B16B8 __attribute__((sda));
extern int D_L13_001B16C0 __attribute__((sda)),D_L13_001B16C4 __attribute__((sda)),D_L13_001B16AC __attribute__((sda)),D_L13_001B16B0 __attribute__((sda));
extern char *D_L13_001B2A0C __attribute__((sda));
extern char D_L13_001B98C0[];
void func_L13_0041A550(char *moby) {
    D_02C80 *beam = (*(D_02C80 **)(moby+0x68));
    Vq_02C80 path[32];
    Vq_02C80 path_steps[32];
    Vq_02C80 work, delta;
    Vq_02C80 matrix[4];

    float length;
    int low, high, duration;
    int i, j, k, branch;

    {
        int min_ticks = D_L13_001B161C;
        int max_ticks = D_L13_001B1620;
        *(float *)(moby+0x1C)=1.0f;
        duration = func_002A7820(min_ticks,max_ticks);
    }
    beam->s12=(short)duration;
    beam->r620=(short)duration;
    length=func_002A7878(4.0f,8.0f);

    for (i = 0; i < 32; i++) {
        float y_offset = func_00283CE0(i);
        float last = func_00283CE0(0x1F);
        float y;
        y_offset = y_offset / last;
        y_offset = (0.5f - y_offset) * length;
        if (i < 3 || i > 28) {
            func_00282C88(&path[i]);
            func_00282C88(&path_steps[i]);
        } else {
            func_002A7A00(&path[i], 0.0f, D_L13_001B16A0);
            func_002A7A00(&path_steps[i], 0.0f, D_L13_001B16A4 * (1.0f/60.0f));
        }
        y = path[i].f[1];
        path[i].f[3] = 1.0f;
        path[i].f[1] = y + y_offset;
    }

    for (j = 0; j < 32; j++) {
        if (j == 0 || j == 31) {
            qcopy(&beam->pts[j], &path[j]);
            qcopy(&beam->off[j], &path_steps[j]);
        } else {
            func_00282CC0(&work, &path[j - 1], &path[j]);
            func_00282CC0(&work, &work, &path[j + 1]);
            func_00282D30(&beam->pts[j], &work, 0.333f);
            func_00282CC0(&work, &path_steps[j - 1], &path_steps[j]);
            func_00282CC0(&work, &work, &path_steps[j + 1]);
            func_00282D30(&beam->off[j], &work, 0.333f);
        }
    }

    for (branch = 0; branch < 4; branch++) {
        int start;
        float spread = (D_L13_001B16A0);
        func_002A7A00(&work, spread, spread);
        start = func_00283CF0(func_002A7878((float)branch * 6.0f, (float)(branch + 1) * 6.0f)) + 4;
        beam->cnt[branch] = (unsigned char)start;
        qcopy(&beam->a[0][branch], &beam->pts[start]);
        qcopy(&beam->b[0][branch], &beam->off[start]);
        /*K*/
        {
            char *b_col, *prev_col, *cur_col;
            prev_col = (char *)(branch * 16 + (int)beam);
            prev_col = prev_col + 0x420;
            cur_col = (char *)(branch * 16 + (int)beam);
            cur_col = cur_col + 0x420;
            b_col = (char *)(branch * 16 + (int)beam);
            b_col = b_col + 0x520;
            for (k = 1; k < 4; k++) {
                func_002A7A00(&delta, 0.1f, 0.1f);
                func_002A7A00(b_col + k * 64, 0.0f, (D_L13_001B16A4) * (1.0f/60.0f));
                func_00282CC0(cur_col + k * 64, prev_col + (k - 1) * 64, &work);
                func_00282CC0(cur_col + k * 64, cur_col + k * 64, &delta);
            }
        }
    }


    if (func_00282E80(moby+0x10, D_L13_001B98C0) >= 4.0f) {
        int s17;
        int l;
        char *frame=(char *)((beam->idx<<7)+(int)D_L13_001B2A0C);
        beam->rotation[2]=*(float *)(frame+0x78);
        beam->rotation[1]=*(float *)(frame+0x74);
        beam->rotation[1]+=func_002A78D8(0.0f,30.0f)*0.0174532942f;
        beam->rotation[0]=func_002A7940();
        func_00283410(matrix,beam->rotation);
        qcopy(&matrix[3], moby+0x10);
        for (l = 0; l < 32; l += 8) {
            Vq_02C80 *p = &beam->pts[l];
            func_002830C0(&work, p, matrix);
            qcopy(&delta, &p[32]);
            delta.f[3] = (D_L13_001B16B8);
            s17 = func_002A77E0(2) ? (D_L13_001B16C0) : (D_L13_001B16C4);
            func_002BAA28(&work, &delta, &delta, (D_L13_001B16AC), (D_L13_001B16B0), l >> 1, 15, func_002A77E0(10) + 20, s17);
        }
        for (k = 0; k < 2; k++) {
            int n;
            func_002830C0(&work, &beam->a[3][k], matrix);
            qcopy(&delta, &beam->b[3][k]);
            delta.f[3] = (D_L13_001B16B8);
            s17 = func_002A77E0(2) ? (D_L13_001B16C0) : (D_L13_001B16C4);
            n = (func_002A77E0(10) + 20);
            func_002BAA28(&work, &delta, &delta, (D_L13_001B16AC), (D_L13_001B16B0), beam->cnt[k] >> 1, 15, n, (D_L13_001B16C4));
        }
    }
}