int update_timed_menu_sequence(char *arg0) {
    PadState *pad = (PadState *)g_controller_state;
    struct MenuSystem *g = (struct MenuSystem *)menu_system;
    int fast = (*(long *)&pad->held & 0xF) == 0xF;
    int x;

    if (g->unkDC == 0 || (fast && (pad->held & 0x10))) {
        if (*(int *)&pad->pressed_unmasked & 0xD00) {
            if (g->close_locked == 0) {
                return 1;
            }
        }
        {
            PadState *pad2 = (PadState *)g_controller_state;
            if (*(int *)&pad2->pressed_unmasked & 0x10) {
                struct MenuSystem *g2 = (struct MenuSystem *)menu_system;
                MenuPage *t = g2->current->back;
                if (t != 0) {
                    g2->next = t;
                } else if (g2->close_locked == 0) {
                    return -1;
                }
            }
        }
    }
    x = *(int *)(arg0 + 0x40);
    *(int *)(arg0 + 0x40) = x - 1;
    if (fast) {
        *(int *)(arg0 + 0x40) = x - 2;
    }
    if (*(int *)(arg0 + 0x40) < 0) {
        *(int *)(arg0 + 0x40) = 0;
    }
    switch (*(int *)(arg0 + 0x50)) {
        case 0:
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xB4);
            *(int *)(arg0 + 0x3C) = 0;
            timed_sequence_title_id = 0x50A9;
            *(char **)(arg0 + 0x34) = timed_sequence_page1;
            break;
        case 1:
        case 5:
        case 8:
        case 12:
        case 16:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            *(int *)(arg0 + 0x54) = 0;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 2:
        case 6:
        case 9:
        case 13:
        case 17:
            x = *(int *)(arg0 + 0x3C);
            *(int *)(arg0 + 0x3C) = x + 10;
            if (fast) {
                *(int *)(arg0 + 0x3C) = x + 20;
            }
            if (*(int *)(arg0 + 0x54) == 0) {
                return 0;
            }
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xB4);
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 4:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            *(int *)(arg0 + 0x50) = 5;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xB4);
            *(int *)(arg0 + 0x3C) = 0;
            timed_sequence_title_id = 0x50D6;
            *(char **)(arg0 + 0x34) = timed_sequence_page2;
            break;
        case 3:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            timed_sequence_title_id = 0x50D4;
            *(int *)(arg0 + 0x34) = 0x50D5;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xF0);
            *(int *)(arg0 + 0x3C) = 0;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 7:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            timed_sequence_title_id = 0x5106;
            *(char **)(arg0 + 0x34) = timed_sequence_page3;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xF0);
            *(int *)(arg0 + 0x3C) = 0;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 10:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            timed_sequence_title_id = 0x5136;
            *(int *)(arg0 + 0x34) = 0x5137;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xF0);
            *(int *)(arg0 + 0x3C) = 0;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 11:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xB4);
            *(int *)(arg0 + 0x3C) = 0;
            timed_sequence_title_id = 0x5138;
            *(char **)(arg0 + 0x34) = timed_sequence_page4;
            *(int *)(arg0 + 0x50) = 12;
            break;
        case 14:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            timed_sequence_title_id = 0;
            *(int *)(arg0 + 0x34) = 0x5143;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xF0);
            *(int *)(arg0 + 0x3C) = 0;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            break;
        case 15:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            *(int *)(arg0 + 0x40) = scale_game_frame_count(0xB4);
            *(int *)(arg0 + 0x3C) = 0;
            timed_sequence_title_id = 0x5144;
            *(char **)(arg0 + 0x34) = timed_sequence_page5;
            *(int *)(arg0 + 0x50) = 16;
            break;
        case 18:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            timed_sequence_title_id = 0;
            *(int *)(arg0 + 0x34) = 0x5175;
            *(int *)(arg0 + 0x50) = *(int *)(arg0 + 0x50) + 1;
            *(int *)(arg0 + 0x40) = scale_game_frame_count(300);
            break;
        case 19:
            if (*(int *)(arg0 + 0x40) != 0) {
                return 0;
            }
            {
                struct MenuSystem *g3 = (struct MenuSystem *)menu_system;
                if (g3->unkDC != 0) {
                    return 1;
                }
            }
            break;
    }
    return 0;
}
