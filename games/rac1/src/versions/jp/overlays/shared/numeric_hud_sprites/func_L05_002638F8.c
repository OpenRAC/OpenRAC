int draw_trick_combo_hud(char *a) {
    unsigned char *u = (unsigned char *)(a + 0x70);
    char buf[0x80];
    int out[4];
    int xa, xb;
    float f20, f21, f0, f12;
    char *E;
    int r, sel, cnt, k, idx, h, acc, id, id2, i, s16, r3, t;

    f21 = (float)u[0] / 10.0f;
    if (f21 > 1.0f) {
        f21 = 1.0f;
    }
    E = hero_globals_base + 0xE1D;
    if (*(int *)(E + 0x208C) != 0x16) {
        return *(int *)(a + 0x58);
    }
    if (u[2] != 0) {
        sprintf_r(buf, rilgar_fmt_number, trick_combo_spins * 0x168);
        f20 = IntToFloat(u[1]);
        r = scale_ticks(*(int *)&trick_combo_flash_ticks);
        f12 = f20 / (float)r;
        if (u[2] == 1 || u[2] == 3) {
            acc = FastTweenColor(*(int *)&trick_combo_flash_color_from, *(int *)&trick_combo_flash_color_to, f12);
        } else {
            acc = *(int *)&trick_combo_text_color;
        }
        h = truncate_float_to_s32_r(f21 * 128.0f) << 24;
        acc += h;
        if (u[2] == 1 || u[2] == 2) {
            FontPrintCenterLarge_x(*(int *)&trick_combo_spin_text_x + 1, *(int *)&trick_combo_spin_text_y + 1, h, (int)buf, -1);
            FontPrintCenterLarge_x(*(int *)&trick_combo_spin_text_x, *(int *)&trick_combo_spin_text_y, acc, (int)buf, -1);
            sprintf_r(buf, rilgar_fmt_count_and_word, trick_combo_trick_count,
                                   msg_string_r(trick_combo_trick_count == 1 ? 0x51CD : 0x51CE));
            FontPrintCenterLarge_x(*(int *)&trick_combo_count_text_x + 1, *(int *)&trick_combo_count_text_y + 1, h, (int)buf, -1);
            FontPrintCenterLarge_x(*(int *)&trick_combo_count_text_x, *(int *)&trick_combo_count_text_y, acc, (int)buf, -1);
        } else if (u[2] == 3) {
            sel = -1;
            if (trick_combo_trick_count == 3) {
                sel = 0;
                if (trick_combo_trick_flag3 != 0) {
                    sel = 2;
                    if (trick_combo_trick_flag2 == 0) {
                        sel = 1;
                    } else if (trick_combo_trick_flag1 != 0) {
                        sel = 3;
                    }
                }
            } else if (trick_combo_trick_count == 4) {
                sel = 4;
            }
            if (sel >= 0) {
                if (trick_combo_spins >= 4) {
                    k = 2;
                } else {
                    k = trick_combo_spins >= 2;
                }
                idx = sel + k * 5;
                trick_combo_recent = idx;
                trick_combo_score = trick_combo_score_table[idx];
                cnt = 1;
                for (i = 1; i < 4; i++) {
                    if ((&trick_combo_recent)[i] == trick_combo_recent) {
                        cnt++;
                    }
                }
                trick_combo_score = trick_combo_score / cnt;
                sprintf_r(buf, rilgar_fmt_number, trick_combo_score);
                draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y - 0x18, acc, (int)buf);
                sprintf_r(buf, rilgar_fmt_string, msg_string_r(idx + 0x5093));
                draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y, acc, (int)buf);
            } else if (trick_combo_trick_count > 0) {
                int n = 0;
                int m;
                for (m = 0; m < 4 && n < trick_combo_trick_count; m++) {
                    if ((&trick_combo_trick_flags)[m] != 0) {
                        out[n] = m;
                        n++;
                    }
                }
                if (trick_combo_trick_count == 1) {
                    trick_combo_score = trick_combo_spins * 10;
                    sprintf_r(buf, rilgar_fmt_number, trick_combo_score);
                    draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y - 0x18, acc, (int)buf);
                    sprintf_r(buf, rilgar_fmt_single_trick_line, msg_string_r(out[0] + 0x51D4), trick_combo_spins,
                                           msg_string_r(trick_combo_spins == 0x168 ? 0x50A8 : 0x50A9));
                    draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y, acc, (int)buf);
                } else {
                    trick_combo_score = trick_combo_spins * 20;
                    sprintf_r(buf, rilgar_fmt_number, trick_combo_score);
                    draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y - 0x18, acc, (int)buf);
                    sprintf_r(buf, rilgar_fmt_double_trick_line, msg_string_r(out[0] + 0x51D4),
                                           msg_string_r(out[1] + 0x51D4), trick_combo_spins);
                    draw_framed_text(*(int *)&trick_combo_result_x, *(int *)&trick_combo_result_y, acc, (int)buf);
                }
            }
        }
    }

    xa = *(int *)(a + 0x50);
    xb = *(int *)(a + 0x54);
    *(int *)(a + 0x58) = 0x100;
    *(int *)(a + 0x5C) = 0x40;
    hud_screen_position(a, &xa, &xb);
    s16 = (*(int *)(a + 0x74) * 0xDD) / *(int *)(a + 0x8) + 0x1B;
    id = GetIconFrame(0x7558, 1);
    draw_cropped_hud_sprite(id, xa, xb, s16, 0x40, 0x80);
    id2 = GetIconFrame(0x7558, 0);
    HudSprite(id2, xa, xb, 0x100, 0x40, 0x80);
    E = hero_globals_base + 0xE1D;
    t = (unsigned char)E[0x8AE];
    if (scale_ticks(0x78) < t) {
        r3 = scale_ticks(0x3C);
        f0 = IntToFloat(rilgar_frame_count % r3) / (float)r3;
        f0 = FastSin(f0 * 6.283180236816406f - 3.141590118408203f);
        r3 = FastTweenColor(*(int *)&trick_combo_banner_color_from, *(int *)&trick_combo_banner_color_to, f0 * 0.5f + 0.5f);
        sprintf_r(buf, msg_string_r(0x5371));
        draw_framed_text(*(int *)&trick_combo_banner_x + 1, *(int *)&trick_combo_banner_y + 1, 0x80000000, (int)buf);
        draw_framed_text(*(int *)&trick_combo_banner_x, *(int *)&trick_combo_banner_y, r3, (int)buf);
    }
    return *(int *)(a + 0x58);
}
