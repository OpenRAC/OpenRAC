/* NON_MATCHING func_L16_002105C8 -- src/overlays/l16_kalebo3/help_00209D98.c
 * Best so far: SIZE ours 2048 / retail 2040, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   3. SIZE2004/2040: separate iterated pointers restore frame140 and side-end stack spill. Loop still uses shifte
 *   4. SIZE2032/2040: explicit byte offset forces resident high rematerialization throughout, and explicit selecte
 *   5. SIZE2032/2040: reversing account condition fixes its branch/copy block, separate pitch base fixes correspon
 *   6. BYTES176/2040: separate height block supplies missing root reload; size exact. Center ray lacks copied alia
 *   7. BYTES165/2040: byte bool/no change, explicit validity changes material registers; reversed height compariso
 *   8. BYTES276/2040: early first-ray assignment moved setup into prologue and regressed; no loop allocation impro
 *   9. SIZE2048/2040: validity-mask decode exactly fixes material selector; native-width ray address restores poin
 *   10. BYTES1244/2040: pointer-width int folds the ray copy again, while fixing material removes4bytes, shifting 
 */
#include "common.h"
extern void func_001F9BC0(void *);
extern void func_L00_00212D70(void *, void *, int, float, float);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_001F3988(void);
extern int func_L00_001F3958(void);
extern float func_00214440(void *, void *);
extern int func_L00_0025F410(void *);
extern void func_L00_00233D50(float *, float *, float);
extern void *func_L00_0025D390_contact(void *) __asm__("func_L00_0025D390");
extern float func_001F9D10(void *, void *);
extern void func_001252C0(void *, void *);
extern float func_L00_002345B0(float *);
extern float func_L00_001FF860(float, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_L00_002167C8(void *, float *);
extern void func_L00_00233E48(float *, float, float, float);
extern float func_001F9B88(float);
extern int D_0015EE84 MACRO_ADDR;
extern char D_0013E633[];
extern char D_L16_001742E0[];
typedef struct { float height[2]; float pitch[2]; float yaw[2]; } L16SideCorrections;
typedef struct {
    char pad00[0x18];
    void *moby;
    int hit_count;
    float point[4];
    char pad30[0x10];
    float normal[4];
} L16ContactResult;
extern L16ContactResult D_L16_001742C0;
typedef struct {
    char pad00[0x80];
    float position[4];
    char pad90[0x1A4];
    float speed_limit;
    char pad238[0x38];
    float velocity_x;
    float velocity_y;
    float gain;
    char pad27C[4];
    float probe[4];
    char pad290[8];
    float floor_velocity;
    char pad29C[4];
    float normal[4];
    float previous_normal[4];
    float side_height[2];
    float side_pitch[2];
    float side_yaw[2];
    float floor_height;
    float ground_distance;
    float ground_tilt;
    float pitch;
    float yaw;
    float ground_yaw;
    float probe_height;
    float alternate_height;
    int height_timer;
    void *ground_moby;
    int contact_counter;
    char pad304[6];
    short constrained;
    short slope_timer;
    short ground_timer;
    char pad310[0x770];
    void *support;
    char padA84[0x85C];
    short material_type;
    char pad12E2[0xB];
    char material;
    char pad12EE[0xD92];
    int moby;
    char pad2084[8];
    int mode;
    char pad2090[0x23];
    unsigned char contact_mode;
} L16GroundPlayer;

/* Probe player ground contact and compute two side correction angles. */
void func_L16_002105C8(void) {
    float test[4], ray[4], angles[4], side_start[4], side_end[4], side_angles[4], normal[4];
    float *side_end_address;
    float *first_ray;
    long ray_address;
    L16GroundPlayer *p = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
    float length;
    unsigned char grounded;
    p->gain = 1.0f;
    p->ground_timer++;
    p->slope_timer++;
    p->velocity_x = 0.0f;
    p->velocity_y = 0.0f;
    p->ground_tilt = 0.0f;
    p->floor_height = 0.0f;
    func_001F9BC0(p->normal);
    p->constrained = 0;
    p->ground_distance = 32.0f;
    p->ground_moby = 0;
    if (p->height_timer) p->height_timer++;
    grounded = 1;
    if (!p->contact_mode) grounded = 0;
    length = p->speed_limit * 2.2f;
    if (p->mode == 2) length = p->speed_limit * 2.9f;
    first_ray = ray;
    func_L00_00212D70(first_ray, test, grounded, length, -48.0f);
    ray_address = (long)first_ray;
    if (func_L00_001EFFF0(first_ray, test, 2, p->moby, 0)) {
        int valid = ~func_L00_001F3988();
        int selected = 3;
        int material;
        int type;
        if (valid) selected = ~valid;
        p->material = selected;
        type = func_L00_001F3958();
        p->material_type = type;
        if (!(short)type) {
            float height = func_00214440(D_L16_001742E0, p->probe);
            p->probe_height = height;
            if (p->position[2] < height && p->height_timer <= 0) p->height_timer = 1;
            if (!func_L00_001EFFF0((void *)ray_address, test, 0x24, ((L16GroundPlayer *)(D_0013E633 + 0xE1D))->moby, 0)) goto response;
            material = func_L00_001F3988();
            if (D_0015EE84 == 1 || D_0015EE84 == 18) material = 3;
            ((L16GroundPlayer *)(D_0013E633 + 0xE1D))->material = material;
        }
        {
            L16GroundPlayer *ground = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
            if (ground->material_type == 13) {
                ground->probe_height = D_L16_001742C0.point[2];
                ray[2] = D_L16_001742C0.point[2] - 0.01f;
                if (!func_L00_001EFFF0((void *)ray_address, test, 4, ground->moby, 0)) goto response;
            }
            if (ground->material_type == 11) {
                ground->alternate_height = D_L16_001742C0.point[2];
                goto response;
            }
            if (D_L16_001742C0.moby) {
                void *effect;
                if (func_L00_0025F410(D_L16_001742C0.moby)) {
                    ground->constrained = 1;
                    if ((unsigned int)(ground->mode - 21) < 2U) func_L00_00233D50(D_L16_001742C0.point, D_L16_001742C0.point, 1.0f);
                }
                effect = func_L00_0025D390_contact(D_L16_001742C0.moby);
                if (effect && (*(unsigned short *)((char *)effect + 0x1E) & 8)) ((L16GroundPlayer *)(D_0013E633 + 0xE1D))->constrained = 1;
            }
            if (D_L16_001742C0.hit_count > 0) {
                float *point = (float *)(D_0013E633 + 0x10BD);
                L16GroundPlayer *contact;
                float *position, *copied_normal;
                qcopy(point, D_L16_001742C0.point);
                contact = (L16GroundPlayer *)((char *)point - 0x2A0);
                position = (float *)((char *)point - 0x220);
                contact->floor_height = D_L16_001742C0.point[2];
                contact->ground_distance = func_001F9D10(position, point);
                contact->ground_moby = D_L16_001742C0.moby;
                copied_normal = (float *)((char *)point - 0x30);
                func_001252C0(copied_normal, D_L16_001742C0.normal);
                contact->ground_tilt = func_L00_002345B0(D_L16_001742C0.normal);
                contact->ground_yaw = func_L00_001FF860(D_L16_001742C0.normal[0], D_L16_001742C0.normal[1]);
                func_001F9BF0(angles, point, position);
                if (func_001F9C78(angles, copied_normal) > 0.0f) contact->ground_distance = -contact->ground_distance;
                if (contact->ground_distance < 0.02f) {
                    contact->slope_timer = 0;
                    if (contact->ground_tilt <= 0.87266463f || contact->contact_mode == 1 || contact->mode == 22) {
                        L16GroundPlayer *support = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
                        support->ground_timer = 0;
                        support->support = D_L16_001742C0.moby;
                    }
                }
            }
        }
    }
    {
        L16GroundPlayer *account = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
        if (account->ground_timer != 0) account->contact_counter = 0;
        else {
            account->contact_counter++;
            qcopy(account->previous_normal, account->normal);
        }
    }
    {
        L16GroundPlayer *heights = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
        if (heights->position[2] > heights->probe_height) heights->height_timer = 0;
    }
response:
    {
        float *response_vector = (float *)(D_0013E633 + 0x10AD);
        L16GroundPlayer *motion;
        func_001F9BC0(response_vector);
        motion = (L16GroundPlayer *)((char *)response_vector - 0x290);
        switch (motion->contact_mode) {
        case 0: motion->floor_velocity = -1.0f; break;
        case 1:
            qcopy(response_vector, (char *)response_vector - 0x20);
            func_001F9C30(response_vector, response_vector, -1.0f);
            break;
        case 2:
            func_001F9BF0(response_vector, (char *)response_vector - 0x1E0, (char *)response_vector - 0x210);
            func_L00_001FF4B0(response_vector, response_vector, 1.0f);
            break;
        }
    }
    {
        L16GroundPlayer *sides = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
        int mode = sides->mode;
        int i;
        int byte_offset;
        L16SideCorrections *corrections;
        float *height, *pitch, *yaw, *pitch_base;
        sides->pitch = 0.0f;
        sides->yaw = 0.0f;
        sides->side_yaw[0] = 0.0f;
        sides->side_yaw[1] = 0.0f;
        sides->side_pitch[0] = 0.0f;
        sides->side_pitch[1] = 0.0f;
        sides->side_height[0] = 0.0f;
        sides->side_height[1] = 0.0f;
        if (mode == 15 || mode == 21 || mode == 6 || mode == 4 || mode == 5 || mode == 3) return;
        if (!(sides->ground_distance < 0.25f)) return;
        func_L00_002167C8(&sides->velocity_x, angles);
        if (angles[1] > -0.7853982f && angles[1] < 0.7853982f) sides->pitch = angles[1];
        if (angles[0] > -0.7853982f && angles[0] < 0.7853982f) ((L16GroundPlayer *)(D_0013E633 + 0xE1D))->yaw = angles[0];
        corrections = (L16SideCorrections *)(D_0013E633 + 0x10DD);
        side_end_address = side_end;
        pitch_base = corrections->pitch;
        height = corrections->height;
        i = 0;
        yaw = corrections->yaw;
        pitch = pitch_base;
        byte_offset = 0;
        for (; i < 2; i++) {
            float angle = 1.5707964f;
            L16GroundPlayer *current;
            if (i == 1) angle = -angle;
            func_L00_00233E48(side_start, 0.2f, angle, 0.45f);
            current = (L16GroundPlayer *)(D_0013E633 + 0xE1D);
            func_L00_00233E48(side_end_address, 0.2f, angle, -0.45f);
            if (func_L00_001EFFF0(side_start, test, 0x22, current->moby, 0)) {
                *height = D_L16_001742C0.point[2] - current->floor_height;
                if (func_001F9B88(*height) > 0.4f) *height = 0.0f;
                func_001252C0(normal, D_L16_001742C0.normal);
                func_L00_002167C8(normal, side_angles);
                if (side_angles[1] > -0.7853982f && side_angles[1] < 0.7853982f) *(float *)(byte_offset + (char *)pitch_base) = side_angles[1];
                if (side_angles[0] > -0.7853982f && side_angles[0] < 0.7853982f) *yaw = side_angles[0];
            } else {
                *height = 0.0f;
                *pitch = current->pitch;
                *yaw = current->yaw;
            }
            height++;
            pitch++;
            yaw++;
            byte_offset += sizeof(float);
        }
    }
}
