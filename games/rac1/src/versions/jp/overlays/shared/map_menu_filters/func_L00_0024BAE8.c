void MarkSelectedCardSlot(void) {
    char *p = (char *)memory_card_state;
    int *item;
    if (((struct MemoryCardState *)p)->state >= 3 || ((struct MemoryCardState *)p)->err >= 0) {
        int index = ((struct MemoryCardState *)p)->cur;
        char *r = p + 0xB0;
        item = (int *)(index * 192 + r);
        if (*item == 1)
            *item = 2;
    }
    p = (char *)memory_card_state;
    if (*(int *)(p + 0x1C) < -1 || *(int *)(p + 0xEC)) {
        mode_freeze_state = 3;
        return;
    }
    if (*(int *)(p + 0x14) == -2) {
        if (*(int *)(p + 0xC) + *(int *)(p + 0xAC) < 351) {
            mode_freeze_state = 19;
            return;
        }
        mode_freeze_state = 12;
        return;
    }
    if (*(int *)(p + 0x14) >= -1)
        mode_freeze_state = 16;
}
