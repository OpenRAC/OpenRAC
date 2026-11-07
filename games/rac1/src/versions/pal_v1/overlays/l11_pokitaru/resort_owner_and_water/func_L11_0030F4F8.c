extern char D_E1_L11_001DA640[];
void water_tile_update(struct Moby *m) {
    int *pv = (int *)m->pvars;
    int i;
    int n;
    float f;

    switch (m->state) {
        case 0:
            m->update_dist = 0xFF;
            {
                int sl = D_L11_00161F64_pv;
                pv[1] = sl;
                D_L11_00161F64_pv = sl + 1;
                if (sl + 1 >= 0x80) {
                    STUB_printf(D_L11_0021B4C0_pv);
                }
            }
            m->update_dist = 0xFF;
            m->flags |= 1;
            m->scale = ((struct MobyClass_pv *)m->pclass)->scale + ((struct MobyClass_pv *)m->pclass)->scale;
            if (pv[1] == 0) {
                Light_pv *l;
                float a;
                for (i = 19; i >= 0; i--) {
                    D_L11_001DA880_pv[i].unk1E = 0;
                }
                pool_init_tiles(D_L11_001DA880_pv, 20);
                pool_set_scale(1.0f);
                l = &D_L11_001CB280_pv;
                l->unk0 = 16.0f;
                l->unk4 = 16.0f;
                l->unk8 = -8.0f;
                l->unkC = -8.0f;
                l->unk10 = 1.0f;
                l->unk14 = 1.0f;
                l->unk18 = 0.9f;
                l->unk20 = 0.1f;
                a = fast_atan2(D_L11_001808C0_pv[4], D_L11_001808C0_pv[5]);
                l->unk30 = FastCos(a) * 0.57735f;
                l->unk34 = FastSin(a) * 0.57735f;
                l->unk38 = -0.57735f;
                l->unk28 = 0x38;
                l->unk2C = 0x39;
                l->unk3C = 0x1C;
                l->unk3D = 0x40;
                pokitaru_pool_draw_tiles = D_L11_001DA880_pv;
                l11_water_tile_binding_record_base = D_E1_L11_001DA640;
                pokitaru_pool_draw_count = 20;
                pokitaru_pool_draw_fixed_scale = 32768.0f;
                pokitaru_pool_draw_max_255 = 255.0f;
                pokitaru_pool_draw_param_48 = 48.0f;
                pokitaru_pool_draw_cleared_word = 0;
            }
            D_L11_001DA880_pv[pv[1]].x = m->pos.x;
            D_L11_001DA880_pv[pv[1]].y = m->pos.y;
            D_L11_001DA880_pv[pv[1]].z = m->pos.z;
            D_L11_001DA880_pv[pv[1]].unk14 = 0x38;
            D_L11_001DA880_pv[pv[1]].unk18 = 0x39;
            D_L11_001DA880_pv[pv[1]].unk1C = 0x1C;
            D_L11_001DA880_pv[pv[1]].unk1D = 0x44;
            D_L11_001DA880_pv[pv[1]].unk1E = 0xFFFF;
            pool_bind_tile(&D_L11_001DA880_pv[pv[1]], D_E1_L11_001DA640 + pv[1] * 16);
            m->state = 1;
            break;
        case 1:
            n = pv[1];
            D_L11_00161F64_pv = 0;
            populate_water_tile_neighbor_indices(n);
            m->state = 2;
            break;
        case 3:
            if (m->command & 4) {
                pv[2] = 1;
                set_moby_list_state(m->group, 2);
            }
            m->command = 1;
            if (pv[2] != 0) {
                ApproachWithVelocity(&m->pos.z, (float *)pv, 132.36f, g_frame_time_sq * 4.0f, g_frame_time_sq * 4.0f,
                                     g_frame_time * 4.0f);
                Group_CopyZToTableSlots(m);
            }
            if (random_integer_below(200) == 0) {
                float x = D_L11_001DA880_pv[pv[1]].x + random_float_between(-4.0f, 4.0f);
                float y = random_float_between(-4.0f, 4.0f);
                Ent_pv *t = &D_L11_001DA880_pv[pv[1]];
                pool_splash(t, 1, 1, x, t->y + y, 1.0f, -0.03f);
            }
            break;
        case 2:
            if (m->command & 8) {
                pv[2] = 1;
                set_moby_list_state(m->group, 3);
            }
            m->command = 2;
            if (pv[2] != 0) {
                ApproachWithVelocity(&m->pos.z, (float *)pv, 137.0f, g_frame_time_sq * 4.0f, g_frame_time_sq * 4.0f,
                                     g_frame_time * 4.0f);
                Group_CopyZToTableSlots(m);
            }
            if (random_integer_below(200) == 0) {
                float x = D_L11_001DA880_pv[pv[1]].x + random_float_between(-4.0f, 4.0f);
                float y = random_float_between(-4.0f, 4.0f);
                Ent_pv *t = &D_L11_001DA880_pv[pv[1]];
                pool_splash(t, 1, 1, x, t->y + y, 1.0f, -0.03f);
            }
            break;
    }
    if (pv[1] >= 0) {
        f = 0.0f;
        if (check_scaled_vector_bsphere(m, 64.0f) != -1) {
            f = 1.0f;
        }
        if (f == 0.0f) {
            D_L11_001DA880_pv[pv[1]].unk1E = 0;
        } else {
            D_L11_001DA880_pv[pv[1]].unk1E = 0xFFFF;
        }
        if (pv[1] == 0) {
            pool_flush(D_L11_001DA880_pv, 20);
            AddDrawCallback((void (*)(char *))render_level_11_water_grids, (char *)m);
        }
    }
}
