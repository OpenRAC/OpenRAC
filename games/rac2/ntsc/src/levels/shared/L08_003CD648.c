typedef int Quadword __attribute__((mode(TI)));
extern char D_00189EA0[];
extern void func_002A0F48(char *);
/* The caller deliberately supplies source even though this callee reloads it. */
extern void func_L08_003CD600(char *, Quadword *);

void func_L08_003CD648(char *context, Quadword *source)
{
    char *actor;
    Quadword *transform = (Quadword *)D_00189EA0;
    transform[0] = source[0];
    transform[1] = source[1];
    actor = *(char **)(D_00189EA0 + 0x2210);
    if (actor != 0) {
        *(Quadword *)(actor + 0x10) = transform[0];
        *(Quadword *)(actor + 0xF0) = transform[1];
        func_002A0F48(actor);
    }
    func_L08_003CD600(context, source);
}
