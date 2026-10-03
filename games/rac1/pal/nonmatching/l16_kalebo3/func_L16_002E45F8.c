/* NON_MATCHING func_L16_002E45F8 -- src/overlays/l16_kalebo3/vendor_002A50F0.c
 * Best so far: BYTES 58/1364 (95.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   5. Maintain moving timer, or capture player's target position.
 *   6. Randomize target with yaw/pitch offsets when its timer expires.
 *   7. Copy active target to stack, choose movement rate.
 *   8. Subtract elevated moby eye from target; derive yaw and pitch.
 *   9. Clamp both angles and split yaw between head/body.
 *   10. Update body/head smoothing with tick-scaled rates.
 *   Runs 1-5: SIZE 1384 then 1360. Float signatures, late rate initialization, and direct global accesses fix all 
 *   Runs 6-9: BYTES 58/1364. p5 uses direct resident aim fields, reproducing retail addiu -0x80 and all path instr
 */
extern int func_L12_002E7EE0(void);
extern int func_L00_002676E8(void *, void *);
extern int func_L00_00267290(void *, void *);
extern void func_L00_00217718(void *, void *, int, int);
extern int func_L00_00203F20(int, int);
extern void func_L00_002618D8(int, int);
extern void func_L00_002512D8(int);
extern void func_L00_00286128(void *, void *);
extern int func_0020BFC8(int, int);
extern float func_001FA790(float, float);
extern float func_001FA850(float, float);
extern float func_001F9878(float);
extern int func_001FA898(float);
extern float func_001F9CE8(void *);
extern void func_00215C00(void *, float, float, float);
extern void func_L00_00263950(void *, void *, int, float, float);
extern float D_0015EE64 MACRO_ADDR;
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern short D_L16_0015F6A8;
extern float D_L16_001D9990[8];
typedef struct {
    char pad00[0x20];
    char activation[0x40];
    char body[0x60];
    float padC0;
    float pitch;
    float yaw;
    float padCC;
    float height;
    char padD4[0xC];
    char head[0x60];
    char pad140[8];
    float head_yaw;
    char pad14C[0x14];
    int activation_region;
    int entry_region;
    int moving_timer;
    int target_timer;
    float target[4];
} L16WatchData;
typedef struct {
    char pad00[0x10];
    float position[4];
    unsigned char state;
    char pad21[0xF];
    unsigned char opacity;
    char pad31[0x17];
    float yaw;
    char pad4C[7];
    unsigned char animation;
    char pad54[0x24];
    L16WatchData *data;
    char pad7C[0x34];
    unsigned char widget;
} L16WatchMoby;
typedef struct {
    char pad00[0x80];
    float position[4];
    char pad90[0x40];
    float aim[4];
    char padE0[0x22E];
    short disabled;
    char pad310[0x1D7C];
    int mode;
} L16WatchPlayer;
typedef struct {
    char pad00[0x70];
    unsigned char entered;
    unsigned char mission;
} L16WatchFlags;
typedef struct {
    float target[4];
    float eye[4];
    float delta[4];
} L16WatchScratch;
extern L16WatchPlayer D_0013E633_watch __asm__("D_0013E633");
extern L16WatchFlags D_0013D355_watch __asm__("D_0013D355");

/* Activate the watcher and steer its body and head toward the player. */
void func_L16_002E45F8(L16WatchMoby *m) {
    L16WatchScratch scratch;
    L16WatchData *d;
    unsigned char state;
    void *position;
    L16WatchFlags *flags;
    float rate;
    float head_rate;
    int tracking;
    void *activation;
    d = m->data;
    func_L12_002E7EE0();
    state = m->state;
    activation = d->activation;
    switch (state) {
    case 0:
        func_L00_002676E8(m, activation);
        m->state = 1;
        *(float *)((char *)d + 0x2C) = 20.0f;
        m->opacity = 0xFF;
        break;
    case 1: {
        L16WatchPlayer *player = (L16WatchPlayer *)((char *)&D_0013E633_watch + 0xE1D);
        if (player->disabled == 0 && player->mode != 15 && player->mode != 22) {
            if (func_00215570((char *)player + 0x80, d->activation_region) &&
                func_L00_00267290(m, activation)) m->state = 2;
        }
        break;
    }
    case 2:
        if (*(int *)&D_L16_0015F6A8 != 2) {
            char *player;
            m->state = 1;
            func_L00_00217718(D_L16_001D9990, D_L16_001D9990 + 4, 0, 1);
            func_L00_00203F20(0x3E82, 0x84);
            func_L00_002618D8(0x21, 1);
            func_L00_002512D8(m->widget);
            player = (char *)&D_0013E633_watch + 0xE9D;
            func_L00_00286128(player, player + 0x10);
            func_0020BFC8(0, -1);
            m->state = 3;
        }
        break;
    }
    flags = (L16WatchFlags *)((char *)&D_0013D355_watch + 0x13B);
    if (!flags->entered && func_00215570((char *)&D_0013E633_watch + 0xE9D, d->entry_region)) flags->entered = 1;
    if (!((L16WatchFlags *)((char *)&D_0013D355_watch + 0x13B))->mission && *(int *)(D_0013E633 + 0x2EA9) == 15) ((L16WatchFlags *)((char *)&D_0013D355_watch + 0x13B))->mission = 1;
    rate = 0.02f;
    head_rate = 0.3f;
    tracking = 0;
    if (m->animation == 0) {
        char *player = (char *)&D_0013E633_watch + 0xE9D;
        tracking = 1;
        if (func_001F9D48(position = m->position, player) < 8.0f &&
            func_001FA850(m->yaw,
                func_L00_001FF860(((L16WatchPlayer *)((char *)&D_0013E633_watch + 0xE1D))->aim[0] - m->position[0],
                    ((L16WatchPlayer *)((char *)&D_0013E633_watch + 0xE1D))->aim[1] - m->position[1])) < 1.5707964f) {
            if (func_001F9CB8(player + 0x80) > 0.01f) d->moving_timer = func_001F9850(120);
            else func_001F9908(&d->moving_timer);
        } else if (d->moving_timer) {
            d->moving_timer = 0;
            qcopy(d->target, (char *)&D_0013E633_watch + 0xEED);
        }
        if (func_001F9908(&d->target_timer)) {
            float heading;
            d->target_timer = func_001FA898(func_001F9878(func_002140F8(180.0f, 300.0f)));
            heading = func_001FA748(m->yaw, func_002140F8(-90.0f, 90.0f) * 0.017453292f);
            func_00215C00(d->target, 6.0f, heading, func_002140F8(0.0f, 30.0f) * 0.017453292f);
            func_001F9BD8(d->target, d->target, position);
        }
        if (d->moving_timer) {
            qcopy(scratch.target, (char *)&D_0013E633_watch + 0xEED);
            rate = 0.04f;
            head_rate = 0.3f;
        } else {
            qcopy(scratch.target, d->target);
        }
    }
    if (tracking) {
        float yaw;
        float pitch;
        qcopy(scratch.eye, m->position);
        scratch.eye[2] += 1.5f;
        func_001F9BF0(scratch.delta, scratch.target, scratch.eye);
        yaw = func_001FA790(func_L00_001FF860(scratch.delta[0], scratch.delta[1]), m->yaw);
        pitch = -func_L00_001FF860(func_001F9CE8(scratch.delta), scratch.delta[2]);
        if (yaw > 1.5707964f) yaw = 1.5707964f;
        else if (yaw < -1.5707964f) yaw = -1.5707964f;
        if (pitch > 0.5235988f) pitch = 0.5235988f;
        else if (pitch < -0.5235988f) pitch = -0.5235988f;
        d->pitch = pitch;
        d->yaw = yaw * 0.7f;
        d->head_yaw = yaw * 0.3f;
    }
    if (D_0015EEB0[0]) d->height = 2.75f;
    func_L00_00263950(m, d->body, 0, rate * D_0015EE64, head_rate * D_0015EE64);
    func_L00_00263950(m, d->head, 2, rate * D_0015EE64, head_rate * D_0015EE64);
}
