extern int mode_freeze_flags MACRO_ADDR;
extern struct TextRegionP D_E1_001E77F0;
extern struct TextRegionP D_E1_001E7808;

void draw_freeze_message_box(void) {
    char buf[0x200];

    SetupGifPaging(0);
    switch (((MessageBoxP *)mode_freeze_info)->state) {
        case 5: {
            f32 alpha;

            alpha = 1.0f - (f32)((MessageBoxP *)mode_freeze_info)->timer / (f32)scale_game_frame_count(30);
            DrawUIFrame(0x50, 0x154, 0x60, 0x1A0, (s32)(alpha * 80.0f));
            {
                struct TextRegionP box = D_E1_001E77F0;
                u8 *p;
                s32 color;
                s32 tint;
                s32 y;
                s32 top;
                s32 mid;
                f32 pos;
                f32 angle;
                f32 pivot;

                color = FastTweenColor(freeze_text_fade_color_start, freeze_text_fade_color_end, alpha);
                libc_strncpy((void *)0x70000000, msg_string(0x4E2B), 0x400);
                p = (u8 *)0x70000000;
                while (*p >= 2) {
                    p++;
                }
                if (*p == 1) {
                    do {
                        *p++ = 0;
                    } while (*p == 1);
                }
                ((void (*)(struct TextRegionP *, long, char *, s32, s32, u8 *))FontPrintWindow)(
                    &box, color, (char *)0x70000000, -1, GetEffectTex(1), normal_font_metrics);
                pivot = 272.0f;
                box.flags |= 4;
                y = box.anchor_y;
                y += box.rendered_height;
                ((void (*)(struct TextRegionP *, long, char *, s32, s32, u8 *))FontPrintWindow)(
                    &box, color, (char *)p, -1, GetEffectTex(1), normal_font_metrics);
                box.flags ^= 4;
                box.anchor_y = 0x136 - box.rendered_height;
                ((void (*)(struct TextRegionP *, long, char *, s32, s32, u8 *))FontPrintWindow)(
                    &box, color, (char *)p, -1, GetEffectTex(1), normal_font_metrics);
                tint = FastTweenColor(0x20FFFF, 0x8020FFFF,
                                      1.0f - (f32)((MessageBoxP *)mode_freeze_info)->unk24 /
                                                 (f32)scale_game_frame_count(30));
                FontPrintCenter(0x100, 0x140, tint, msg_string(0x524A), -1);
                mid = (y + box.anchor_y) >> 1;
                VU1_addGSregister(0x47, 0x3004B);
                top = mid - 0x20;
                draw_hud_sprite(GetIconFrame(0x755D, 0), 0xE0, top, 0x40, 0x40, 0x80);
                pos = (f32)(mid << 4);
                angle = (f32)(game_frame_counter % 55) * -6.2831855f / 55.0f;
                draw_rotated_sprite(4096.0f, pos, pivot, pivot, angle, 0x40, 0x40,
                                    GetFrameTex(GetIconFrame(0x755D, 1)));
            }
            break;
        }
        case 3: {
            char *text;
            s32 left;
            s32 right;
            s32 mid;
            s32 f;

            text = freeze_prompt_default_text;
            left = 0;
            mid = 0;
            right = 0;
            switch (*(s32 *)&mode_freeze_state) {
                case 6:
                    left = 0x524F;
                    right = 0x524B;
                    text = msg_string(0x4FAF);
                    break;
                case 12:
                    if (!(mode_freeze_flags & 2)) {
                        mid = 0x4FA8;
                        text = msg_string(0x4FA7);
                        break;
                    }
                    /* fall through */
                case 13:
                    left = 0x524F;
                    right = 0x524B;
                    text = msg_string(0x4FAD);
                    break;
                case 10:
                case 11:
                    text = msg_string(0x4FC1);
                    break;
                case 7:
                case 8:
                    text = msg_string(0x4FB7);
                    break;
                case 14:
                case 15:
                    text = msg_string(0x4FB8);
                    break;
                case 17:
                    mid = 0x524A;
                    text = msg_string(0x4FBA);
                    break;
                case 18:
                    mid = 0x524A;
                    text = msg_string(0x4FBC);
                    break;
                case 2:
                    mid = 0x524A;
                    text = msg_string(0x4FA6);
                    if (((MessageBoxP *)mode_freeze_info)->timer != 0) {
                        mid = 0;
                    }
                    break;
                case 19:
                    if (mode_freeze_flags & 4) {
                        mid = 0x4FA8;
                        text = msg_string(0x4FA7);
                    } else if (boot_level_active != 0) {
                        left = 0x524E;
                        right = 0x524B;
                        sprintf(buf, freeze_prompt_number_format, msg_string(0x4FA9), 1, 1, msg_string(0x4FAA));
                        text = buf;
                    } else {
                        mid = 0x4FA8;
                        text = msg_string(0x4FA9);
                    }
                    break;
                case 21:
                    mid = 0x524A;
                    text = msg_string(0x4FBD);
                    break;
                case 20:
                    mid = 0x524A;
                    text = msg_string(0x4FBB);
                    break;
                case 24:
                    left = 0x524E;
                    right = 0x524B;
                    sprintf(buf, freeze_prompt_number_format, msg_string(0x4FAE), 1, 1, msg_string(0x4FAA));
                    text = buf;
                    break;
                case 23:
                    left = 0x524E;
                    right = 0x524B;
                    text = msg_string(0x4FB1);
                    break;
                case 3:
                case 4:
                    f = mode_freeze_flags;
                    if (boot_level_active != 0 && (f & 2)) {
                        left = 0x524E;
                        right = 0x524B;
                        text = msg_string(0x4FAB);
                    } else if (f & 4) {
                        mid = 0x4FA8;
                        text = msg_string(0x4FA7);
                    } else {
                        mid = 0x4FA8;
                        text = msg_string(0x4FAC);
                    }
                    break;
                case 5:
                    mid = 0x4FA8;
                    sprintf(buf, freeze_prompt_number_format, msg_string(0x4FB0), 1, 1, msg_string(0x4FA7));
                    text = buf;
                    break;
            }
            draw_box_p(
                text, left, mid, right,
                (struct TextRegionP){0, ((ScreenP *)g_screen_extent)->height, 0x60, 0x1A0, 0x100, 0x68, 0, 0, 0x10, 5});
            break;
        }
        case 6: {
            struct TextRegionP box;
            s32 text_color;
            s32 button_color;
            s32 id;
            f32 alpha;

            box = D_E1_001E7808;
            alpha = 1.0f - (f32)((MessageBoxP *)mode_freeze_info)->timer / (f32)scale_game_frame_count(30);
            DrawUIFrame(0x64, 0x12C, 0x60, 0x1A0, (s32)(alpha * 80.0f));
            text_color = FastTweenColor(freeze_text_fade_color_start, freeze_text_fade_color_end, alpha);
            button_color = FastTweenColor(0x20FFFF, 0x8020FFFF, alpha);
            switch (((MessageBoxP *)mode_freeze_info)->choice) {
                case 0:
                case 1:
                    FontPrintCenter(0xCA, 0x118, button_color, msg_string(0x524E), -1);
                    FontPrintCenter(0x135, 0x118, button_color, msg_string(0x524B), -1);
                    id = 0x522A;
                    break;
                case 2:
                    FontPrintCenter(0xCA, 0x118, button_color, msg_string(0x524E), -1);
                    FontPrintCenter(0x135, 0x118, button_color, msg_string(0x524B), -1);
                    id = 0x522B;
                    break;
                case 3:
                    FontPrintCenter(0x100, 0x118, button_color, msg_string(0x524A), -1);
                    id = 0x522C;
                    break;
                default:
                    id = 0;
                    break;
            }
            if (id != 0) {
                ((void (*)(struct TextRegionP *, long, char *, s32, s32, u8 *))FontPrintWindow)(
                    &box, text_color, msg_string(id), -1, GetEffectTex(1), normal_font_metrics);
            }
            break;
        }
        case 0: {
            char line[0x40];
            s32 pulse;
            s32 c1;
            s32 c2;
            s32 c3;
            s32 best;
            s32 record;
            s32 unit;
            s32 rem;
            s32 per;
            s32 min;
            s32 sec;
            s32 hund;
            s32 sel;
            f32 scale;
            f32 f;

            pulse = FastTweenColor(freeze_box_pulse_color_a, freeze_box_pulse_color_b,
                                   FastSin((f32)(game_frame_counter % freeze_box_pulse_period) /
                                               IntToFloat(freeze_box_pulse_period) * 6.28318f -
                                           3.14159f) *
                                           0.5f +
                                       0.5f);
            scale = (f32)((MessageBoxP *)mode_freeze_info)->unk20 * 0.125f;
            if (scale > 1.0f) {
                scale = 1.0f;
            } else if (scale < 0.1f) {
                scale = 0.1f;
            }
            c1 = freeze_box_text_color;
            c2 = freeze_box_record_color;
            c3 = freeze_box_no_record_color;
            if (((MessageBoxP *)mode_freeze_info)->unk24 != 0) {
                f = (f32)((MessageBoxP *)mode_freeze_info)->unk24 * 0.125f;
                if (f > 1.0f) {
                    f = 1.0f;
                } else if (f < 0.0f) {
                    f = 0.0f;
                }
                c1 = FastTweenColor(freeze_box_text_color, freeze_box_text_flash_color, f);
                c2 = FastTweenColor(freeze_box_record_color, freeze_box_record_flash_color, f);
                c3 = FastTweenColor(freeze_box_no_record_color, freeze_box_no_record_flash_color, f);
            }
            if (((Hero *)g_hero)->race_lap < 3) {
                draw_outlined_rect(
                    freeze_short_box_center_y - truncate_float_to_s32((f32)freeze_short_box_half_height * scale),
                    freeze_short_box_center_y + truncate_float_to_s32((f32)freeze_short_box_half_height * scale),
                    freeze_short_box_center_x - truncate_float_to_s32((f32)freeze_short_box_half_width * scale),
                    freeze_short_box_center_x + truncate_float_to_s32((f32)freeze_short_box_half_width * scale), pulse);
                FontPrintCenter(0x100, freeze_short_box_text_y, c1, msg_string(0x4F6E), -1);
                FontPrintCenter(0x100, freeze_short_box_text_y + 0x18, c1, msg_string(0x5249), -1);
                FontPrintCenter(0x100, freeze_short_box_text_y + 0x30, c1, msg_string(0x5248), -1);
            } else {
                draw_outlined_rect(
                    freeze_long_box_center_y - truncate_float_to_s32((f32)freeze_long_box_half_height * scale),
                    freeze_long_box_center_y + truncate_float_to_s32((f32)freeze_long_box_half_height * scale),
                    freeze_long_box_center_x - truncate_float_to_s32((f32)freeze_long_box_half_width * scale),
                    freeze_long_box_center_x + truncate_float_to_s32((f32)freeze_long_box_half_width * scale), pulse);
                sel = ((Hero *)g_hero)->race_position - 1;
                if (!(sel < 5)) {
                    sel = 4;
                }
                sprintf(line, freeze_rank_line_format, msg_string(sel + 0x5224), msg_string(0x5240));
                if (((Hero *)g_hero)->race_position == 1) {
                    FontPrintCenter(0x100, freeze_long_box_text_y, c2, line, -1);
                } else {
                    FontPrintCenter(0x100, freeze_long_box_text_y, c3, line, -1);
                }
                best = race_best_times[(g_current_level_index ^ 0x10) ? 0 : 1];
                if (best == ((Hero *)g_hero)->race_time_frames) {
                    unit = pal_mode ? 3000 : 3600;
                    per = unit / 60;
                    min = best / unit;
                    rem = best - min * unit;
                    sec = rem / per;
                    hund = (rem - sec * per) * 100 / per;
                    sprintf(line, freeze_text_line_format, msg_string(0x50A3));
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x26, c2, line, -1);
                    sprintf(line, freeze_race_time_format, min, sec, hund);
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x3A, c2, line, -1);
                } else {
                    sprintf(line, freeze_text_line_format, msg_string(0x50A2));
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x30, c3, line, -1);
                }
                record = race_best_records[(g_current_level_index ^ 0x10) ? 0 : 1];
                if (record != 0 && record == ((Hero *)g_hero)->race_score) {
                    sprintf(line, freeze_text_line_format, msg_string(0x50A5));
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x56, c2, line, -1);
                    sprintf(line, freeze_record_value_format, ((Hero *)g_hero)->race_score);
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x6A, c2, line, -1);
                } else {
                    sprintf(line, freeze_text_line_format, msg_string(0x50A4));
                    FontPrintCenter(0x100, freeze_long_box_text_y + 0x60, c3, line, -1);
                }
                FontPrintCenter(0x100, freeze_long_box_text_y + 0x90, c1, msg_string(0x5249), -1);
                FontPrintCenter(0x100, freeze_long_box_text_y + 0xA8, c1, msg_string(0x5248), -1);
            }
            break;
        }
        case 2:
            draw_outlined_rect(0x64, 0xA0, 0xB0, 0x150,
                               FastTweenColor(freeze_box_pulse_color_a, freeze_box_pulse_color_b,
                                              FastSin((f32)(game_frame_counter % freeze_box_pulse_period) /
                                                          IntToFloat(freeze_box_pulse_period) * 6.28318f -
                                                      3.14159f) *
                                                      0.5f +
                                                  0.5f));
            FontPrintCenter(0x100, 0x7A, 0x8000C0C0, ((MessageBoxP *)mode_freeze_info)->title, -1);
            break;
        default: {
            s32 pulse;

            pulse = FastTweenColor(freeze_box_pulse_color_a, freeze_box_pulse_color_b,
                                   FastSin((f32)(game_frame_counter % freeze_box_pulse_period) /
                                               IntToFloat(freeze_box_pulse_period) * 6.28318f -
                                           3.14159f) *
                                           0.5f +
                                       0.5f);
            draw_outlined_rect(0x50, freeze_box_line2_y + 0x1A, 0xB0, 0x150, pulse);
            if (((MessageBoxP *)mode_freeze_info)->title != 0) {
                FontPrintCenter(0x100, 0x5A, 0x8000C0C0, ((MessageBoxP *)mode_freeze_info)->title, -1);
            }
            if (((MessageBoxP *)mode_freeze_info)->line1 != 0) {
                FontPrintCenter(0x100, freeze_box_line1_y, 0x80FFA888, ((MessageBoxP *)mode_freeze_info)->line1, -1);
            }
            if (((MessageBoxP *)mode_freeze_info)->line2 != 0) {
                FontPrintCenter(0x100, freeze_box_line2_y, 0x80FFA888, ((MessageBoxP *)mode_freeze_info)->line2, -1);
            }
            break;
        }
    }
    DoGifPaging();
}
