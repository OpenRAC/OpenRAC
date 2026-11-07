void Moby1402Update(unsigned char *m) {
    VD *d = ((VD *)((Moby *)m)->pvars);
    switch (((Moby *)m)->state) {
        case 0:
            if (*(int *)&veldin2_lift_raised_flag != 0) {
                g_hero_field_20a5[0xB] = *(unsigned char *)&veldin2_lift_raised_flag;
            }
            {
                int c = veldin2_lift_count;
                unsigned short fl;
                *(int *)((char *)d + 0x1C) = c;
                fl = ((Moby *)m)->flags;
                veldin2_lift_count = c + 1;
                ((Moby *)m)->update_dist = 0xFF;
                ((Moby *)m)->state = 1;
                ((Moby *)m)->flags = fl | 1;
            }
            ((Moby *)m)->draw_dist = 0;
            *(int *)&((Moby *)m)->collision = 0;
            break;
        case 1:
            if (menu_item_enabled_105[9] != 0) {
                d->w8 = (d->w8 + 1) & 1;
            }
            {
                int w = d->w8;
                veldin2_lift_count = 0;
                if (w != 0 && g_hero_field_20a5[0xB] == 0) {
                    ((Moby *)m)->state = 2;
                    ((Moby *)m)->pos.z = d->f0;
                } else {
                    ((Moby *)m)->state = 3;
                    ((Moby *)m)->pos.z = d->f4;
                }
            }
            break;
        case 3:
            if (((Moby *)m)->command & 4) {
                *(int *)&veldin2_lift_raised_flag = 0;
                ((Moby *)m)->state = 2;
                if (d->w8 == 0) {
                    menu_item_enabled_105[9] = 1;
                }
            }
            ApproachWithVelocity_v(d->f4, g_frame_time_sq * 4.0f, g_frame_time_sq * 4.0f, g_frame_time * 4.0f,
                                   (float *)(m + 0x18), (float *)((char *)d + 0x18));
            synchronize_lift_group_height(m);
            ((Moby *)m)->command = 1;
            break;
        case 2:
            if (((Moby *)m)->command & 8) {
                ((Moby *)m)->state = 3;
                *(int *)&veldin2_lift_raised_flag = 1;
            }
            ((Moby *)m)->command = 2;
            synchronize_lift_group_height(m);
            ApproachWithVelocity_v(d->f0, g_frame_time_sq * 4.0f, g_frame_time_sq * 4.0f, g_frame_time * 4.0f,
                                   (float *)(m + 0x18), (float *)((char *)d + 0x18));
            break;
    }
    switch (d->w14) {
        case 1: {
            float t = lift_texture_u_offset_gp + veldin2_lift_uv_scroll_speed * g_frame_time;
            lift_texture_u_offset_gp = t;
            if (t > 1.0f) {
                lift_texture_u_offset_gp = t - 1.0f;
            }
            AddDrawCallback((void (*)(void))draw_lift_25_quad_mesh, m);
            break;
        }
        case 2:
            AddDrawCallback((void (*)(void))draw_lift_38_quad_mesh, m);
            break;
        case 0:
            AddDrawCallback((void (*)(void))draw_lift_22_quad_mesh, m);
            break;
    }
}
