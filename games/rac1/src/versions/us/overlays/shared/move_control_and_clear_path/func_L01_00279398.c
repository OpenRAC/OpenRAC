typedef struct {
    char pad0[0x10];
    float p[4];
    float q[2];
    float a;
    char pad1[0x1A];
    short f;
} GGv1;
void begin_dialogue_camera_transition(float scale, char *moby) {
    GGv1 *c = (GGv1 *)&fx_cutscene;
    c->f = 1;
    qcopy(c->p, moby + 0x10);
    c->p[0] += FastCos(*(float *)(moby + 0x48)) * scale;
    c->p[1] += FastSin(*(float *)(moby + 0x48)) * scale;
    clear_u64_value(c->q);
    c->a = FastAddRots(*(float *)(moby + 0x48), 3.14159274101257324f);
}
