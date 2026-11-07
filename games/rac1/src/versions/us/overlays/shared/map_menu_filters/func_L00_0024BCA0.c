void CardMenu_AdvanceOrAlternate(void) {
    MenuState *p = &memory_card_state;
    if (p->v[53] == 2 && p->v[55] < 0) {
        if (p->v[57]) {
            mode_freeze_state = 18;
            mode_freeze_flags_m |= 64;
        } else {
            p->v[48] = 0;
            p->v[55] = 7;
            p->v[5] = 0;
            p->v[56] = 0;
            mode_freeze_state = 16;
        }
    }
}
