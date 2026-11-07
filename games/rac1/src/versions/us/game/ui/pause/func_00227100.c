int func_00227100_r(char *moby, char *m1, char *m2, char **o0, char **o1, char **o2) {
    int flagA = 0;
    int flagB = 0;
    int result = 0;
    char *p;

    if (preview_animation_request_count == 0) {
        return 0;
    }
    {
        if (*(unsigned char *)(moby + 0x70) & 2) {
            flagA = 1;
            if (current_preview_animation_request[0].arg[2] == 0 ||
                --current_preview_animation_request[0].arg[2] == 0) {
                flagB = 1;
            }
        }
        {
            if (menu_system_q[0].pending_buffer != 0) {
                if (cd_read_state_q[0].unk8 == 0) {
                    int n = menu_system_q[0].pending_buffer - 1;
                    int *slot = menu_system_q[0].stream_buffer + n;
                    int buf = *slot;
                    fast_decompress(buf + menu_system_q[0].read_offset, buf);
                    menu_system_q[0].read_offset = 0;
                    clear_menu_stream_buffer_read_active(buf);
                    menu_system_q[0].loaded_animation[n] = preview_animation_requests_q[0].arg[0];
                    preview_animation_requests_q[0].unk00 = 2;
                    ((Cls_q *)moby_class_resources[0])->anim[preview_animation_requests_q[0].arg[0]] = *slot;
                    relocate_asset_entry_pointers(moby_class_resources[0], preview_animation_requests_q[0].arg[0]);
                    menu_system_q[0].pending_buffer = 0;
                }
            }
        }
        if (menu_system_q[0].pending_buffer == 0 && cd_read_state_q[0].unk8 == 0 &&
            preview_animation_request_count > 0 && preview_animation_requests_q[0].unk00 == 0) {
            int idx = menu_system_q[0].read_buffer_index;
            unsigned char *ld = menu_system_q[0].loaded_animation;
            unsigned int want = preview_animation_requests_q[0].arg[0];
            if (ld[idx] == want || ld[idx == 0] == want) {
                preview_animation_requests_q[0].unk00 = 2;
            } else if (want < menu_system_q[0].streamed_animation_base) {
                preview_animation_requests_q[0].unk00 = 3;
            } else {
                int k = want - menu_system_q[0].streamed_animation_base;
                int buf = menu_system_q[0].stream_buffer[idx];
                int sz = disc_table_q[0].e[k].b << 11;
                int addr = buf + get_menu_stream_buffer_size(buf) - sz;
                menu_system_q[0].read_offset = addr - buf;
                if (start_audio_stream_read(addr, disc_table_q[0].e[k].a, disc_table_q[0].e[k].b) == 0) {
                    raise_kernel_trap();
                }
                mark_menu_stream_buffer_read_active(buf);
                menu_system_q[0].pending_buffer = menu_system_q[0].read_buffer_index + 1;
                ld[menu_system_q[0].read_buffer_index] = 0xFF;
                preview_animation_requests_q[0].unk00 = 1;
            }
        }
        if (preview_animation_request_count > 0 && (unsigned int)(preview_animation_requests_q[0].unk00 - 2) < 2) {
            int item = preview_animation_requests_q[0].arg[3];
            if (item != 0 && menu_system_q[0].equipped[0] != item && menu_system_q[0].equipped[2] != item &&
                menu_system_q[0].equipped[1] != item) {
                result = 0;
                advance_preview_animation_queue();
            } else if (preview_animation_requests_q[0].arg[1] == 0) {
                result = 1;
            } else if (preview_animation_requests_q[0].arg[1] == 1 && flagA != 0) {
                result = 1;
            } else if (preview_animation_requests_q[0].arg[1] == 2 && flagB != 0) {
                result = 1;
            } else if (preview_animation_requests_q[0].arg[1] == 3) {
                result = 0;
                advance_preview_animation_queue();
            }
        }
        if (result != 0) {
            current_preview_animation_request[0] = preview_animation_requests_q[0];
            advance_preview_animation_queue();
            clear_preview_resource_bindings();
            load_preview_resource_bindings(current_preview_animation_request[0].arg[11],
                                           current_preview_animation_request[0].arg[12]);
            blend_moby_animation_ex(moby, current_preview_animation_request[0].arg[0], 0, 10, 5);
            if (current_preview_animation_request[0].arg[0] == menu_system_q[0].loaded_animation[0]) {
                menu_system_q[0].read_buffer_index = 1;
            } else if (current_preview_animation_request[0].arg[0] == menu_system_q[0].loaded_animation[1]) {
                menu_system_q[0].read_buffer_index = 0;
            }
            if (item_info_table_r[current_preview_animation_request[0].arg[3]].item_type == 2) {
                if (m2 != 0) {
                    blend_moby_animation_ex(m2, current_preview_animation_request[0].arg[4], 0, 10, 5);
                }
                if (m1 != 0) {
                    MobySetAnimation(m1, 1, 0);
                }
            } else if (m1 != 0) {
                blend_moby_animation_ex(m1, current_preview_animation_request[0].arg[4], 0, 10, 5);
            }
            *o0 = (char *)delete_moby((int)*o0);
            *o1 = (char *)delete_moby((int)*o1);
            *o2 = (char *)delete_moby((int)*o2);
            if (current_preview_animation_request[0].arg[5] != -1) {
                if ((*o0 = create_menu_preview_moby(current_preview_animation_request[0].arg[5])) != 0) {
                    MobySetAnimation(*o0, current_preview_animation_request[0].arg[6], 0);
                    MobyBlendAnimation(*o0, current_preview_animation_request[0].arg[6], 0, 10);
                    *(short *)(*o0 + 0x34) = 0;
                    p = *o0;
                    qcopy(p + 0x10, moby + 0x10);
                    qcopy(p + 0x40, moby + 0x40);
                    if (*(short *)(p + 0xA6) == 0x4A) {
                        *(float *)(p + 0x2C) = *(float *)(p + 0x2C) * 3.0f;
                    }
                    refresh_moby_spatial_bounds(*o0);
                    *(void **)(*o0 + 0x74) = attachment_update_callback;
                }
            }
            if (current_preview_animation_request[0].arg[7] != -1) {
                if ((*o1 = create_menu_preview_moby(current_preview_animation_request[0].arg[7])) != 0) {
                    MobySetAnimation(*o1, current_preview_animation_request[0].arg[8], 0);
                    MobyBlendAnimation(*o1, current_preview_animation_request[0].arg[8], 0, 10);
                    *(short *)(*o1 + 0x34) = 0;
                    p = *o1;
                    qcopy(p + 0x10, moby + 0x10);
                    qcopy(p + 0x40, moby + 0x40);
                    if (*(short *)(p + 0xA6) == 0x4A) {
                        *(float *)(p + 0x2C) = *(float *)(p + 0x2C) * 3.0f;
                    }
                    refresh_moby_spatial_bounds(*o1);
                    *(void **)(*o1 + 0x74) = attachment_update_callback;
                }
            }
            if (current_preview_animation_request[0].arg[9] != -1) {
                if ((*o2 = create_menu_preview_moby(current_preview_animation_request[0].arg[9])) != 0) {
                    MobySetAnimation(*o2, current_preview_animation_request[0].arg[10], 0);
                    MobyBlendAnimation(*o2, current_preview_animation_request[0].arg[10], 0, 10);
                    *(short *)(*o2 + 0x34) = 0;
                    p = *o2;
                    qcopy(p + 0x10, moby + 0x10);
                    qcopy(p + 0x40, moby + 0x40);
                    if (*(short *)(p + 0xA6) == 0x4A) {
                        *(float *)(p + 0x2C) = *(float *)(p + 0x2C) * 3.0f;
                    }
                    refresh_moby_spatial_bounds(*o2);
                    *(void **)(*o2 + 0x74) = attachment_update_callback;
                }
            }
        }
    }
    return 0;
}
