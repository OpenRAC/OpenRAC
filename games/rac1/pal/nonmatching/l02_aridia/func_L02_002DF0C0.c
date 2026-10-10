/* NON_MATCHING func_L02_002DF0C0 -- src/overlays/l02_aridia/vendor_002A59D8.c
 * Best so far: SIZE ours 4828 / retail 4852, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - case 11 (+f04..): the save-bit test: retail `lw D_0014171B+0x591; 1 << D_0015EE84; and` against ours
 *   `lw ..0x31A($s1); srav; andi` - our pseudo for D_0014171B+0x277 is reused (CSE of the two offsets);
 *   write the second as its own symbol/expression (address rules: separate blocks get separate symbols).
 *   - the final call: ours passes &w.old_rot from $fp (see above).
 *   ## Stopped here (wrap-up)
 *   Best: e2.c (SIZE 4832/4852, with the modelled line-469 edit). Lessons from 365 that may apply here: a char
 *   store kills cached global loads (QImode aliases everything); CSE path-following can be broken by writing a
 *   `||` test the other way round; killing stores are scheduled first, then source order. No budget run used.
 */
extern s32 D_0015EE84 MACRO_ADDR;
extern void func_L00_001FF4B0(void *, void *, float);
typedef float PVec50[4] __attribute__((aligned(16)));
typedef unsigned int PQuad50 __attribute__((mode(TI)));
typedef struct PData50 PData50;
typedef struct PMoby50 {
    char pad0[0x10]; PVec50 pos;
    unsigned char state; char pad21[0xF];
    unsigned char alpha, nearby; short brightness; unsigned short flags;
    char pad36[2]; unsigned long lighting; PVec50 rot;
    char pad50[0x28]; PData50 *data; char pad7C[0x34];
    unsigned char persistent; char padB1[0xB]; unsigned char end;
} PMoby50;
struct PData50 {
    char pad0[0x60]; int endpoint_a, endpoint_b, path, pad6C;
    int wheel[4], timer; PMoby50 *upper, *lower;
    float upper_velocity, lower_velocity, upper_offset, lower_offset, z_velocity;
    int destination; float progress, progress_velocity; int timeout;
    float max_speed, acceleration;
};
typedef struct PPath50 { int count; char pad4[12]; PVec50 points[1]; } PPath50;
typedef struct PPlayer50 {
    char pad0[0x2FC]; PMoby50 *pod; char pad300[0xE]; short mode;
    char pad310[0x1D94]; unsigned char blocked;
} PPlayer50;
extern PMoby50 *pod_allocate50(int) __asm__("func_0020D348");
extern char *pod_particle50(float, void *, int, int, int, unsigned char, short, unsigned char) __asm__("func_L01_00286530");
extern void func_L02_002E03B8(char *);
extern void func_L02_002E04E8(char *);
extern void func_L00_00234768(void *, float, int);
extern void func_L00_002EBF50(void *, void *, int, int, int);
extern void func_L02_002F9ED8(float, float, float, float, float, float);
extern void func_L00_002EBE88(void *);
extern void func_L00_002EBEE0(void *);
extern int func_L00_002347B8(void);
extern int func_L02_0022BE40(int, int);
extern void func_L00_002EC0C8(int);
extern void func_L02_00237C98(void);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_L02_002F9E50(int, int);
extern void func_00215CA8(int *, int, void *, float *, int, float);
extern int func_L00_00203F20(int, int);
extern float func_001F9CB8(void *);
extern int D_0015EFA4 MACRO_ADDR;
extern float Pdebug50 SDATA(D_L02_00161CD4);
extern float Pactive50 SDATA(D_L02_00161CD0);
extern float PheightA50 SDATA(D_L02_00161CB0);
extern float PheightB50 SDATA(D_L02_00161CB4);
extern float Pmaxheight50 SDATA(D_L02_00161CC0);
extern float Paccelheight50 SDATA(D_L02_00161CC4);
extern float PcameraA50 SDATA(D_L02_00161CD8);
extern float PcameraB50 SDATA(D_L02_00161CDC);
extern float PcameraC50 SDATA(D_L02_00161CE0);
extern float PcameraD50 SDATA(D_L02_00161CE4);
extern float PcameraE50 SDATA(D_L02_00161CE8);
extern float PcameraF50 SDATA(D_L02_00161CF0);

void func_L02_002DF0C0(PMoby50 *m) {
    struct { PVec50 delta, old_rot, scratch, camera, target, angles; } w;
    PData50 *d = m->data;
    float *pos, *rot;
    int i, result, control;
    float speed;
    PPlayer50 *player;
    float progress;
    int selected, other;
    char *point;
    int elapsed;
    func_001F9C30(w.delta, m->pos, -1.0f);
    qcopy_nc(w.old_rot, m->rot);
    if (Pdebug50 != 0.0f) {
        for (i = 0; i < ((PPath50 *)D_L02_001B0DB0[d->path])->count; i++) {
            qcopy(w.scratch, ((PPath50 *)D_L02_001B0DB0[d->path])->points[i]);
            pod_particle50(0.1f, w.scratch, (int)m, 4, 0x800000FF, 0x7F, 2, 0xFF);
        }
    }
    switch (m->state) {
    case 0: {
        PPath50 **paths;
        float *copy_pos, *copy_rot;
        char *linked;
        m->alpha = 0xFF; m->end = 1; copy_pos = m->pos;
        qcopy(copy_pos, (d->endpoint_b << 7) + D_L02_0016016C + 0x30);
        m->pos[2] -= 6.0f; m->state = 1;
        d->upper = pod_allocate50(0x421);
        d->upper->brightness = 0x40; d->upper->nearby = 1;
        d->upper->lighting = m->lighting; d->upper->flags = m->flags;
        linked = (char *)d->upper; qcopy(linked + 0x10, copy_pos); copy_rot = m->rot; qcopy(linked + 0x40, copy_rot);
        d->lower = pod_allocate50(0x422);
        d->lower->brightness = 0x40; d->lower->nearby = 1;
        d->lower->lighting = m->lighting; d->lower->flags = m->flags;
        linked = (char *)d->lower; qcopy(linked + 0x10, copy_pos); qcopy(linked + 0x40, copy_rot);
        d->lower_offset = -1.0f; d->upper_offset = -1.0f;
        pos = copy_pos; rot = copy_rot;
        paths = (PPath50 **)D_L02_001B0DB0;
        d->acceleration = D_0015EE70 * 0.1f * (float)paths[d->path]->count;
        d->max_speed = D_0015EE6C * 0.3f * (float)paths[d->path]->count;
        func_L02_002E04E8((char *)m);
        break;
    }
    case 1:
        pos = m->pos; rot = m->rot;
        if (((unsigned char *)D_0014171B)[0xAA35 + m->persistent + (D_0015EE84 << 4)] == 0xFF) {
            d->timer = func_001F9850(30); m->state = 2;
        }
        break;
    case 2:
        pos = m->pos; rot = m->rot;
        speed = D_0015EE6C * 5.0f;
        func_L00_001FF4B0(w.scratch, (void *)((d->wheel[0] << 8) + D_L02_00160058_m + 0xC0), -speed);
        point = (char *)((d->wheel[0] << 8) + D_L02_00160058_m + 0x10);
        func_001F9BD8(point, point, w.scratch);
        func_L00_001FF4B0(w.scratch, (void *)((d->wheel[1] << 8) + D_L02_00160058_m + 0xC0), speed);
        point = (char *)((d->wheel[1] << 8) + D_L02_00160058_m + 0x10);
        func_001F9BD8(point, point, w.scratch);
        func_L00_001FF4B0(w.scratch, (void *)((d->wheel[2] << 8) + D_L02_00160058_m + 0xC0), -speed);
        point = (char *)((d->wheel[2] << 8) + D_L02_00160058_m + 0x10);
        func_001F9BD8(point, point, w.scratch);
        func_L00_001FF4B0(w.scratch, (void *)((d->wheel[3] << 8) + D_L02_00160058_m + 0xC0), speed);
        point = (char *)((d->wheel[3] << 8) + D_L02_00160058_m + 0x10);
        func_001F9BD8(point, point, w.scratch);
        func_L00_00251E30((void *)((d->wheel[0] << 8) + D_L02_00160058_m));
        func_L00_00251E30((void *)((d->wheel[1] << 8) + D_L02_00160058_m));
        func_L00_00251E30((void *)((d->wheel[2] << 8) + D_L02_00160058_m));
        func_L00_00251E30((void *)((d->wheel[3] << 8) + D_L02_00160058_m));
        if (func_001F9908(&d->timer)) { m->state = 3; control = 1; goto control_transition50; }
        break;
    case 3:
        pos = m->pos; rot = m->rot;
        func_00214D88(&m->pos[2], &d->z_velocity, *(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38), Paccelheight50 * D_0015EE70, Paccelheight50 * D_0015EE70, Pmaxheight50 * D_0015EE6C);
        func_L02_002E04E8((char *)m);
        if (*(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38) - 0.0001f <= m->pos[2]) { m->state = 4; control = 0; goto control_transition50; }
        break;
    case 4:
        pos = m->pos; rot = m->rot;
        d->upper_offset = 2.0f; d->lower_offset = -4.0f;
        func_L02_002E03B8((char *)m);
        if (func_001F9B88(d->upper->pos[2] - m->pos[2] - 2.0f) < 0.0001f && func_001F9B88(d->lower->pos[2] - m->pos[2] + 4.0f) < 0.0001f) m->state = 5;
        break;
    case 5: {
        PPath50 **paths;
        void *target_position, *camera_angles;
        if (m->end == 0) {
            if (func_001F9D48(D_0013E633 + 0xE9D, (d->endpoint_b << 7) + D_L02_0016016C + 0x30) < 16.0f) {
                qcopy(m->pos, (d->endpoint_b << 7) + D_L02_0016016C + 0x30);
                m->pos[2] -= 6.0f; d->lower_offset = -1.0f; d->upper_offset = -1.0f;
                pos = m->pos; rot = m->rot;
                func_L02_002E04E8((char *)m); m->state = 3; m->end = 1;
                func_0022ED80(1, 0, (int)m); break;
            }
        }
        if (m->end == 1) {
            if (func_001F9D48(D_0013E633 + 0xE9D, (d->endpoint_a << 7) + D_L02_0016016C + 0x30) < 16.0f) {
                qcopy(m->pos, (d->endpoint_a << 7) + D_L02_0016016C + 0x30);
                m->pos[2] -= 6.0f; d->lower_offset = -1.0f; d->upper_offset = -1.0f;
                pos = m->pos; rot = m->rot;
                func_L02_002E04E8((char *)m); m->end = 0; m->state = 3;
                func_0022ED80(1, 0, (int)m); break;
            }
        }
        pos = m->pos; rot = m->rot;
        player = (PPlayer50 *)(D_0013E633 + 0xE1D);
        if (player->pod == m && player->mode == 0 && player->blocked == 0) {
            qcopy(w.camera, m->pos); w.camera[2] += 2.0f;
            if (m->end == 0) {
                paths = (PPath50 **)D_L02_001B0DB0;
                d->destination = 0; progress = (float)(paths[d->path]->count - 1); d->progress = progress;
                qcopy(w.scratch, (d->endpoint_a << 7) + D_L02_0016016C + 0x70);
            } else {
                paths = (PPath50 **)D_L02_001B0DB0;
                d->progress = 0.0f; d->destination = paths[d->path]->count - 1;
                qcopy(w.scratch, (d->endpoint_b << 7) + D_L02_0016016C + 0x70); progress = d->progress;
            }
            qcopy(w.target, paths[d->path]->points[func_001FA898(progress)]);
            target_position = w.target; camera_angles = w.angles;
            func_001F9BC0(w.angles);
            w.angles[2] = func_L00_001FF860(w.camera[0] - w.target[0], w.camera[1] - w.target[1]);
            w.angles[1] = -func_L00_001FF860(func_001F9D48(w.target, w.camera), w.camera[2] - w.target[2]);
            m->state = 6; d->timeout = func_001F9850(180);
            func_L00_00234768(pos, w.scratch[2], 0);
            func_L00_002EBF50(D_L02_00167440, D_L02_00167440 + 0x10, 3, func_001F9850(240), 0);
            func_L02_002F9ED8(PcameraA50, PcameraB50, PcameraC50, PcameraD50, PcameraE50, PcameraF50);
            func_L00_002EBE88(target_position); func_L00_002EBEE0(camera_angles);
        }
        break;
    }
    case 6:
        result = func_L00_002347B8();
        pos = m->pos; rot = m->rot;
        if (result == 2) {
            m->state = 7; func_0022ED80(2, 0, (int)m); func_L02_0022BE40(0x72, 0);
        } else if (result == 0) {
            func_L00_002EC0C8(2); m->state = 13;
        } else if (func_001F9908(&d->timeout)) {
            func_L00_002EC0C8(2); m->state = 13; func_L02_00237C98();
        }
        break;
    case 7:
        pos = m->pos; rot = m->rot;
        d->lower_offset = -1.0f; d->upper_offset = -1.0f;
        func_L02_002E03B8((char *)m);
        if (func_001F9B88(d->upper->pos[2] - m->pos[2] + 1.0f) < 0.01f && func_001F9B88(d->lower->pos[2] - m->pos[2] + 1.0f) < 0.01f) m->state = 8;
        break;
    case 8: {
        void *target_position;
        pos = m->pos; rot = m->rot;
        func_00214D88(&m->pos[2], &d->z_velocity, *(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38) - 6.0f, Paccelheight50 * D_0015EE70, Paccelheight50 * D_0015EE70, Pmaxheight50 * D_0015EE6C);
        func_L02_002E04E8((char *)m);
        if (m->pos[2] < *(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38) - 5.999f) {
            if (m->end == 0) {
                m->end = 1; selected = d->endpoint_b << 7;
                qcopy_nc(m->pos, D_L02_0016016C + selected + 0x30);
                qcopy_nc(w.target, D_L02_0016016C + selected + 0x70);
                other = d->endpoint_a;
            } else {
                m->end = 0; selected = d->endpoint_a << 7;
                qcopy_nc(m->pos, D_L02_0016016C + selected + 0x30);
                qcopy_nc(w.target, D_L02_0016016C + selected + 0x70);
                other = d->endpoint_b;
            }
            target_position = w.target;
            func_001F9BF0(w.scratch, D_L02_0016016C + selected + 0x30, D_L02_0016016C + (other << 7) + 0x30);
            m->pos[2] -= 6.0f;
            func_001F9BD8(w.camera, D_0013E633 + 0xE9D, w.scratch);
            func_L00_00217718(w.camera, target_position, 0x72, 0);
            d->z_velocity = -d->z_velocity; m->state = 9;
            func_L02_002F9E50(2, func_001F9850(180));
        }
        break;
    }
    case 9:
        func_00214D88(&d->progress, &d->progress_velocity, (float)d->destination, d->acceleration, d->acceleration, d->max_speed);
        if (d->destination != 0) {
            if ((float)d->destination <= d->progress) goto path_complete50;
            goto follow_path50;
        } else if (d->progress <= (float)d->destination) {
        path_complete50:
            m->state = 10;
        }
    follow_path50:
        func_00215CA8((int *)D_L02_001B0DB0[d->path], 0, w.target, w.camera, 0, d->progress);
        qcopy(w.scratch, m->pos); w.scratch[2] += 8.0f;
        pos = m->pos; rot = m->rot;
        func_001F9BC0(w.camera);
        w.camera[2] = func_L00_001FF860(w.scratch[0] - w.target[0], w.scratch[1] - w.target[1]);
        w.camera[1] = -func_L00_001FF860(func_001F9D48(w.target, w.scratch), w.scratch[2] - w.target[2]);
        func_L00_002EBE88(w.target); func_L00_002EBEE0(w.camera);
        break;
    case 10:
        pos = m->pos; rot = m->rot;
        func_00214D88(&m->pos[2], &d->z_velocity, *(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38), Paccelheight50 * D_0015EE70, Paccelheight50 * D_0015EE70, Pmaxheight50 * D_0015EE6C);
        func_L02_002E04E8((char *)m);
        if (*(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38) <= m->pos[2]) {
            m->state = 11; control = 0;
        control_transition50:
            func_0022ED80(control, 0, (int)m);
        }
        break;
    case 11:
        pos = m->pos; rot = m->rot;
        d->upper_offset = 2.0f; d->lower_offset = -4.0f;
        func_L02_002E03B8((char *)m);
        if (func_001F9B88(d->upper->pos[2] - m->pos[2] - 2.0f) < 0.01f && func_001F9B88(d->lower->pos[2] - m->pos[2] + 4.0f) < 0.01f) {
            elapsed = func_001F9850(D_0015EFA4) - *(unsigned short *)(D_0014171B + 0x277) * 600;
            if ((int)((float)func_001F9850(3600) * 60.0f) < elapsed && !(*(int *)(D_0014171B + 0x591) & (1U << D_0015EE84))) func_L00_00203F20(0x7DD, 0x48);
            m->state = 13; func_L00_002EC0C8(2); func_L02_0022BE40(0, 1);
        }
        break;
    case 12:
        pos = m->pos; rot = m->rot;
        m->pos[2] += D_0015EE6C * 4.0f;
        if (*(float *)((d->endpoint_a << 7) + D_L02_0016016C + 0x38) <= m->pos[2]) m->state = 5;
        break;
    case 13:
        pos = m->pos; rot = m->rot;
        if (*(PMoby50 **)(D_0013E633 + 0x1119) != m) m->state = 5;
        break;
    case 14: {
        float *copy_pos, *copy_rot;
        char *linked;
        copy_pos = m->pos;
        d->upper = pod_allocate50(0x421);
        d->upper->brightness = 0x40; d->upper->nearby = 1;
        d->upper->lighting = m->lighting; d->upper->flags = m->flags;
        linked = (char *)d->upper; qcopy(linked + 0x10, copy_pos); copy_rot = m->rot; qcopy(linked + 0x40, copy_rot);
        d->lower = pod_allocate50(0x422);
        d->lower->brightness = 0x40; d->lower->nearby = 1;
        d->lower->lighting = m->lighting; d->lower->flags = m->flags;
        linked = (char *)d->lower; qcopy(linked + 0x10, copy_pos); qcopy(linked + 0x40, copy_rot);
        m->state = 15; pos = copy_pos; rot = copy_rot;
        break;
    }
    case 15:
        pos = m->pos; rot = m->rot;
        d->upper_offset = PheightA50; d->lower_offset = PheightB50;
        func_L02_002E03B8((char *)m);
        if (Pactive50 != 0.0f) m->state = 16;
        break;
    case 16:
        d->upper_offset = 2.0f; d->lower_offset = -4.0f;
        func_L02_002E03B8((char *)m);
        if (Pactive50 == 0.0f) goto inactive_pod50;
        pos = m->pos; rot = m->rot;
        if (func_001F9B88(d->upper->pos[2] - m->pos[2] - 2.0f) < 0.01f && func_001F9B88(d->lower->pos[2] - m->pos[2] + 4.0f) < 0.01f) m->state = 17;
        break;
    case 17:
        d->lower_offset = -1.0f; d->upper_offset = -1.0f;
        func_L02_002E03B8((char *)m);
        if (Pactive50 == 0.0f) {
        inactive_pod50:
            m->state = 15; rot = m->rot; pos = m->pos;
            break;
        }
        pos = m->pos; rot = m->rot;
        if (func_001F9B88(d->upper->pos[2] - m->pos[2] + 1.0f) < 0.01f && func_001F9B88(d->lower->pos[2] - m->pos[2] + 1.0f) < 0.01f) m->state = 16;
        break;
    default:
        pos = m->pos; rot = m->rot; break;
    }
    func_001F9BD8(w.delta, w.delta, pos);
    if (func_001F9CB8(w.delta) > 4.0f) func_001F9BC0(w.delta);
    func_L00_002617B0((char *)d + 0x20, w.delta, w.old_rot, rot);
}
