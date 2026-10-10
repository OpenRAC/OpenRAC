/* NON_MATCHING func_L11_002D21F0 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: BYTES 13/780 (98.3% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   mini35 main-only: started from staged108/780, updated current turning-helper prototype. Typed BossMoby/BossMov
 *   hq12 s10 (5 runs, p7-p11): p10 (the rail index read inline as d->rail[k], no arr local) takes the case 9 block
 */
typedef struct {
    char pad0[0x10]; float position[4]; unsigned char state;
    char pad21[0x1f]; float rotation[4]; char pad50[3];
    unsigned char animation; char pad54[4]; float opacity;
    char pad5c[0x1c]; char *data;
} BossMoby_2D21F0;
typedef struct {
    char pad0[0xf0]; int rail[2]; char padf8[0xc]; int child;
    char pad108[0x18]; float target[4]; float velocity[4];
    char pad140[8]; float verticalSpeed; char pad14c[4];
    float turnSpeed; float moveSpeed; int railSelect;
} BossMovement_2D21F0;
extern int D_L11_00160058_d __asm__("D_L11_00160058") MACRO_ADDR;
extern char D_L11_0016D2E0[];
extern short D_L11_00161470;
extern short D_L11_00161480;
extern float D_0015EE6C_d __asm__("D_0015EE6C") MACRO_ADDR;
extern float D_0015EE70_d __asm__("D_0015EE70") MACRO_ADDR;
extern float func_L00_001FF860(float, float);
extern float func_L00_0025CE58(float *, float, float *, float, float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_00215B18(char *, float);
extern void func_001F9BF0(void *, void *, void *);
extern int func_001F9850(int);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CE8(void *);
extern float func_L00_0025BC48(float *a, float *b, float *out, float speed, float g);
extern float func_001F9CB8(void *);
extern void func_001F9BC0(void *);
extern int func_L00_00261568_c(char *, char *, void *, void *, void *, void *) __asm__("func_L00_00261568");
extern int func_L11_0030B850(unsigned char *moby);
extern void func_00213DE0(void *, int, int, int);
extern void func_L11_002CCB50(char *moby);

/* Boss movement: turns toward and walks to its target point, lunges at it in a jump arc, follows its rail
 * moby, or (rarely) triggers its idle taunt. */
void func_L11_002D21F0(char *m) {
    BossMoby_2D21F0 *boss = (BossMoby_2D21F0 *)m;
    BossMovement_2D21F0 *d = (BossMovement_2D21F0 *)boss->data;
    switch (boss->state) {
    case 0: case 1: case 2: case 3: case 11: case 12:
        break;
    case 4: case 5: case 6: case 8: case 10: {
        float *rz = &boss->rotation[2];
        float yaw = func_L00_001FF860(d->target[0] - boss->position[0], d->target[1] - boss->position[1]);
        func_L00_0025CE58(rz, yaw, &d->turnSpeed, D_0015EE70_d * 12.566371f,
                          D_0015EE70_d * 25.132742f, D_0015EE6C_d * 12.566371f);
        d->velocity[0] = func_001F9F90(boss->rotation[2]) * d->moveSpeed;
        {
            float y = func_001F9FA8(boss->rotation[2]);
            d->velocity[2] = d->verticalSpeed - D_0015EE70_d * 20.0f;
            d->velocity[1] = y * d->moveSpeed;
        }
        if (0.0f < d->velocity[2]) d->velocity[2] = 0.0f;
        break;
    }
    case 7: {
        int launch = func_00215B18(m, 9.0f);
        float *v = d->velocity;
        float *tgt;
        if (launch) {
            tgt = d->target;
            func_001F9BF0(v, tgt, m + 0x10);
            func_001F9C30(v, v, 1.0f / (float)func_001F9850(0x3C));
            d->velocity[2] = func_L00_0025BC48((float *)(m + 0x10), (float *)tgt, 0, func_001F9CE8(v), -(D_0015EE70_d * 10.8f));
            boss->opacity = 0.6666667f;
        }
        if (0.0f < func_001F9CB8(v)) d->velocity[2] -= D_0015EE70_d * 10.8f;
        if (func_00215B18(m, 29.0f)) {
            func_001F9BC0(v);
            boss->opacity = 1.0f;
        }
        break;
    }
    case 9: {
        int k = (unsigned int)d->railSelect > 0;
        char *o = (char *)(D_L11_00160058_d + (d->rail[k] << 8));
        func_L00_00261568_c(m, o, &D_L11_00161470, &D_L11_00161480, m + 0x10, m + 0x40);
        if (func_L11_0030B850((unsigned char *)o) && boss->animation != 5) {
            func_00213DE0(m, 5, 0, func_001F9850(0x14));
        }
        break;
    }
    case 13: {
        char *s = D_L11_0016D2E0;
        if (*(int *)(s + 0x30) == 1 && *(int *)(s + 0x34) == func_001F9850(0x7D0)) {
            func_L11_002CCB50((char *)(D_L11_00160058_d + (d->child << 8)));
        }
        break;
    }
    }
}
