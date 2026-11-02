/* Adapted from RAC1 src/game/pause.c, draw_menu_choice_list. */
extern void func_002FC560(int, long);
extern void func_0027C540(int);
extern void func_0027C660(void);
extern void func_0027F538(int,int,long,signed char *,int);
typedef struct { int text; unsigned char *value; int names[4]; } OptionB7;
typedef struct { char pad[0x20]; int width, height; char pad28[12]; OptionB7 *items; int selected; } MenuB7;
extern char *func_00288FB8(int);
extern void func_0027F980(int,int,unsigned long,char *,int);
int func_002DB7F0(MenuB7 *m) {
    OptionB7 *p, *it;
    int n, i, step, y, color;
    func_002FC560(0x47,0x2004B);
    func_0027C540(0);
    n = 0;
    for (p = m->items; p->text; p++)
        n++;
    step = m->height / (n + 1);
    p = m->items;
    y = step - 8;
    for (i = 0; m->items[i].text; i++) {
        it = &m->items[i];
        if (i == m->selected)
            color = 0x8020FFFF;
        else
            color = 0x80FFA888;
        func_0027F538(12,y,color,(signed char *)func_00288FB8(it->text),-1);
        func_0027F980(m->width-12,y,0x80FFA888,func_00288FB8(it->names[*it->value]),-1);
        y += step;
    }
    func_0027C660();
    return 2;
}
