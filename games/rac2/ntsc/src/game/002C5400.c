typedef struct {
    unsigned char b0, b1, b2, b3;
    int color;
    unsigned char seed, alpha;
    short size;
    float life;
    float position[4];
    float payload[8];
} Particle;
extern void func_00282CC0(void *, void *, void *);
extern int func_00282978(void *);
extern void func_002B9180(void *);
extern float func_00283CE0(int);
extern int func_00283D38(int, int, float);

/* Particle tick: drift (around an anchor when it has one) and fade with the remaining life. */
void func_002C5400(Particle *p)
{
    float *pl = p->payload;
    if (((char **)pl)[7] != 0) {
        pl[0] += pl[4];
        pl[1] += pl[5];
        pl[2] += pl[6];
        func_00282CC0(p->position, ((char **)pl)[7] + 0x10, pl);
    } else {
        p->position[0] += pl[4];
        p->position[1] += pl[5];
        p->position[2] += pl[6];
    }
    if (func_00282978(&p->size)) {
        func_002B9180(p);
    } else if (((char **)pl)[7] != 0) {
        p->color = func_00283D38(p->color & 0xFFFFFF, p->color, func_00283CE0(p->size - 1) / func_00283CE0(p->size));
    } else {
        p->color = func_00283D38(0, p->color, func_00283CE0(p->size - 1) / func_00283CE0(p->size));
    }
}
