typedef struct {
    unsigned char pad00[0xC];
    float rowHeight;
    int count;
    int selected;
    int state;
    int backward;
    int firstVisibleOffset;
    int lastVisibleOffset;
    unsigned char pad28[0x68];
    int scrollOffset;
    int frames;
} ListSelection;

extern const int D_001AE1CC __attribute__((sda));
extern int func_002E5E58(int, int, void *);
int func_00342978(ListSelection *self, int flags)
{
    int row;
    int offset;
    int selection;
    self->frames++;
    switch (self->state) {
    case 0:
        if ((flags & 0x1000) && self->selected > 0) {
            func_002E5E58(3, 0, 0);
            self->backward = 1;
            self->state = 1;
        } else if ((flags & 0x4000) && self->selected < self->count - 1) {
            func_002E5E58(3, 0, 0);
            self->backward = 0;
            self->state = 1;
        }
        break;
    case 1:
        self->scrollOffset = 0;
        self->state = 2;
        /* Fall through to advance the first scroll step. */
    case 2:
        if (!self->backward) offset = self->scrollOffset - 4;
        else offset = self->scrollOffset + 4;
        self->scrollOffset = offset;
        if ((float)offset >= self->rowHeight || (float)offset <= -self->rowHeight) {
            if (!self->backward) selection = ++self->selected;
            else selection = --self->selected;
            self->selected = selection;
            if (selection < 0) self->selected = 0;
            if (self->selected > self->count - 1) self->selected = self->count - 1;
            self->state = flags ? self->state : 0;
            self->state = self->selected == 0 || self->selected == self->count - 1 ? 0 : self->state;
            self->scrollOffset = 0;
        }
        break;
    }
    row = self->selected;
    self->firstVisibleOffset = 0;
    if (row > 0) {
        if (0 > -D_001AE1CC) do {
            row--;
            self->firstVisibleOffset--;
        } while (row > 0 && self->firstVisibleOffset > -D_001AE1CC);
    }
    row = self->selected;
    self->lastVisibleOffset = 0;
    if (row < self->count - 1) {
        while (row < self->count - 1 && self->lastVisibleOffset < D_001AE1CC) {
            row++;
            self->lastVisibleOffset++;
        }
    }
    return 0;
}
