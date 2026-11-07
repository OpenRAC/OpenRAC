void memcard_RestoreInfo(char *buf, int slot, int idx) {
    struct V1Slot {
        char hdr[0x20];
        McEntry e[5];
        char pad[0xC];
    };
    struct V1Card {
        struct V1Slot slots[1];
    };
    ((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].valid = memcard_TestChecksum(buf) == 0;
    buf += 0x10;
    ((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].a = *(int *)buf;
    buf += 0xC;
    ((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].b = *(int *)buf;
    buf += 0xC;
    ((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].c = *(int *)buf;
    buf += 0xC;
    ((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].d = *(int *)buf;
    buf += 0xC;
    memcpy(((struct V1Card *)memory_card_state_s)->slots[slot].e[idx].name, buf, 8);
}
