typedef struct { int count, selection; int *strings; int enabled[25]; } MenuList_2CB648;
extern MenuList_2CB648 D_00210A40;
extern char D_001AB900[];
extern int D_001AB8F4 __attribute__((sda));
extern int D_001AB8F8 __attribute__((sda));
extern void func_0027DDC0(int, int, int, int);
extern char *func_00288FB8(int);
extern int func_00115DA8(char *, char *, ...);
extern void func_0027EB88(int, int, int, int, int, int);
extern void func_0027FAA8(int, int, unsigned long, char *, int);

void func_002CB648(void)
{
    char text[256];
    int entry = 0;
    int i = 0;
    int row;
    char *first;
    unsigned int colour;
    MenuList_2CB648 *base;
    int *item;
    text[255] = 0;
    func_0027DDC0(0, 0, 0, 0x60);
    for (; i <= D_00210A40.count * 2; i += 2, entry++) {
        base = &D_00210A40;
        item = base->enabled + entry;
        if (*item) {
            row = entry == 24 ? 36 : i;
            first = func_00288FB8(base->strings[i + 1]);
            func_00115DA8(text, D_001AB900, first, func_00288FB8(base->strings[i]));
            func_0027EB88(D_001AB8F8 + row * D_001AB8F4,
                         D_001AB8F8 + row * D_001AB8F4 + 15,
                         64, 448, 96, 0x442D00);
            colour = base->selection == entry ? 0x80FFDE8D : 0x80808080;
            if (*item == 0) colour = 0x80000000;
            func_0027FAA8(256, D_001AB8F8 + row * D_001AB8F4, (unsigned long)colour, text, -1);
        }
    }
}
