void process_unlock_code_input(void) {
    char *pad = g_controller_state;
    int lim0, lim;
    int va, vb, vc;
    int pressed, direction, count, j, code, index, matched;
    if ((*(long *)(pad + 0x1C0) & 0xF) == 6) {
        pressed = *(int *)(pad + 0x1C4);
        if (!(pressed & 0xF0A0))
            return;

        if (unlock_code_input_count >= 20)
            return;
        if (pressed & 0x1000)
            direction = 0;
        else if (pressed & 0x4000)
            direction = 1;
        else if (pressed & 0x8000)
            direction = 2;
        else if (pressed & 0x2000)
            direction = 3;
        else
            direction = (pressed & 0x80) ? 4 : 5;
        count = unlock_code_input_count;
        unlock_code_input[count++] = direction;
        unlock_code_input_count = count;
        if (count != 20)
            return;
        code = -1;
        index = 2;
        lim0 = 0x93;
        lim = *(int *)&lim0;
        for (; index < lim; index++) {
            matched = 1;
            for (j = 0; j < 20; j++) {
                if (unlock_code_input[j] != unlock_code_key_table[(j * index + index) & 0xff]) {
                    matched = 0;
                    break;
                }
            }
            if (matched) {
                code = index - 2;
                break;
            }
        }
        if (code == -1)
            return;
        va = code - 0x24;
        vb = code - 0x37;
        vc = code - 0x3d;
        if (code < 0x25) {
            (item_available)[code] = 1;
            (gHavePersuader + 5)[code] = 1;
        } else if ((unsigned)(code - 0x25) < 0x12) {
            unlock_planet(va);
        } else if ((unsigned)vb < 6) {
            char *p3 = gSpecialItems;
            p3[vb] = 1;
        } else if ((unsigned)vc < 0x1e) {
            unsigned char *q = gSkillPoints + vc;
            if (!*q) {
                *q = 1;
                allocate_voice_for_bank_entry(1, 0, 0);
                queue_dialogue_message(0x542D, -1);
            }
        }
    } else {
        unlock_code_input_count = 0;
    }
}
