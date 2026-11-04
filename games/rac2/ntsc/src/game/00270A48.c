typedef void (*ObjectDispatch)(void *);
typedef struct { ObjectDispatch entries[5]; } ObjectDispatchRecord;
extern ObjectDispatchRecord D_0026E900[];
extern char *D_001B5390;
extern char D_001B5750[];
extern void func_00270A00(char *);
typedef struct {
    char pad00[0x30];
    float pos[3];
    char pad3C[0x64 - 0x3C];
    float last[3];
    char pad70[0x8C - 0x70];
    short kind;
    char pad8E[0xA0 - 0x8E];
} Obj_270A48;
extern int D_001B7EB0[];
extern int func_00270838(void *, void *);
extern void func_00270598(void *);
extern void func_002704F0(char *);
extern void func_00270210(void);

/* Pick the best live object over the current one, then refresh and dispatch it. */
int func_00270A48(void)
{
    Obj_270A48 *best = (Obj_270A48 *)D_001B5390;
    int changed;
    int i;
    ObjectDispatch callback;
    func_00270A00((char *)best);
    changed = 0;
    for (i = 0; i < 48; i++) {
        if (D_001B7EB0[i] != 0 && &((Obj_270A48 *)D_001B5750)[i] != best && func_00270838(&((Obj_270A48 *)D_001B5750)[i], best)) {
            best = &((Obj_270A48 *)D_001B5750)[i];
            changed = 1;
        }
    }
    if (changed) func_00270598(best);
    callback = D_0026E900[best->kind].entries[3];
    func_002704F0((char *)best);
    if (callback != 0) callback(best);
    {
        float *src = best->pos;
        float *dst = best->last;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    func_00270210();
    return -1;
}
