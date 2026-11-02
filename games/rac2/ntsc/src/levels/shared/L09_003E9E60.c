typedef struct {
    float x, y, z, w;
} Vec9E60;
extern struct { char pad[128]; Vec9E60 position; } D_00189E20;
typedef struct {
    char pad[0x10];
    Vec9E60 position;
    unsigned char mode;
    char pad21[0x47];
    void *state;
} Object9E60;
extern void func_00282CF0(char *, void *, char *);
extern void func_00282FB8(char *, char *, float);
extern float func_00283C80(float, float);
extern float func_002A7798(float);
extern float func_00282DF0(char *);
extern void func_002AAA00(char *, float, float, float, float, char *, int);
extern void func_002AAC80(float *, float, float, float, float);
extern float func_002EAB28(float, float);
extern float func_002EB478(float, float);
extern void func_00282F20(char *, char *, float);
extern void func_00282CC0(char *, char *,char *);
int func_L09_003E9E60(Object9E60 *obj, void *buf, float f)
{
    char *object = (char *)obj;
    struct State {
        char pad0[0x64];
        float field64, field68, field6C, field70, field74, field78;
        int pad7C;
        float field80;
        char pad84[0x6C];
        float velocity[4];
        int pad100;
        float field104;
        char pad108[16];
        short current, previous;
    };
    struct State *state = *(struct State **)(object + 0x68);
    float v[4];
    float speed;
    float scale;
    int result = 1;
    float angle;
    char *pos;
    if (state->current != -1) {
        scale = 1.0f;
        func_00282CF0((char *)v, buf, (char *)&obj->position);
        pos = (char *)&obj->position;
        func_00282FB8((char *)v, (char *)v, state->field78);
        angle = func_002EAB28(v[0], v[1]);
        if (*(unsigned char *)(object + 0x20) < 4) {
            func_002AAA00(object + 0xF8, angle, state->field64, state->field68, state->field6C, (char *)&state->field104, 0);
            scale = func_00283C80(angle, *(float *)(object + 0xF8)) / state->field80;
            if (scale > 1.0f) scale = 1.0f;
            else if (scale < 0.0f) scale = 0.0f;
            scale = func_002A7798(1.0f - scale);
        } else {
            float y = obj->position.y;
            float x = obj->position.x;
            angle = func_002EAB28(D_00189E20.position.x - x, D_00189E20.position.y - y);
            func_002AAA00(object + 0xF8, func_002EB478(angle, 1.5707964897155762f), state->field64, state->field68, state->field6C, (char *)&state->field104, 0);
        }
        speed = func_00282DF0((char *)state->velocity);
        func_002AAC80(&speed, f, state->field70, state->field74, state->field78);
        speed *= scale;
        func_00282F20((char *)state->velocity, (char *)v, speed);
        func_00282CC0(pos, pos, (char *)state->velocity);
        if (state->current != state->previous) result = 0;
    }
    return result;
}
