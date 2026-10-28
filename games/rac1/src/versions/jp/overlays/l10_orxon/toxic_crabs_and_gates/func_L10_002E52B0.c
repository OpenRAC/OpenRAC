void update_toxic_crab_hit_reactions_and_targeting(void *mv) {
    Moby *m = mv;
    V_2E52B0 vA;
    V_2E52B0 vB;
    V_2E52B0 W;
    V_2E52B0 X;
    V_2E52B0 dst;
    int A;
    float B;
    float fx;
    D_2E52B0 *d = (D_2E52B0 *)m->pvars;
    Hit_2E52B0 *r;
    Moby *q;
    int v21;

    if (((Hero *)(hero_globals_base_r + 0xE1D))->items[0].item_id == 0x15) {
        if (((D_2E52B0 *)((Hero *)(hero_globals_base_r + 0xE1D))->items[0].moby->pvars)->f10 == m) {
            activate_waiting_moby_group_members_r(m);
            if (m->state == 1) m->state = 2;
        }
    }
    if (d->f38 != 0) {
        activate_waiting_moby_group_members_r(m);
        d->f1AC = truncate_float_to_s32_r(multiply_global_factor_r(random_float_between_r(180.0f, 240.0f)));
        d->f38 = 0;
    }
    fast_dec_timer_r(&d->f1AC);
    A = 1;
    B = 0.0f;
    r = MobyGetHitMessage_r(m, 0xB30000, 0);
    v21 = ComputeHitResponse_r(m, r, &d->f20, 0, &A, &B, 0, 4);
    FastDecTimer_r(&d->h26);
    if ((unsigned int)A >= 2 && m->state != 0xB && m->state != 0xC) {
        if (g_current_level_index_m == 10 && r != 0 && r->f20 != 0 && r->f20->oclass == 0x31A) {
            if (d->f1AC != 0 && ((Flags_2E52B0 *)(menu_item_enabled_115_r + 1))->b11 == 0) {
                ((Flags_2E52B0 *)(menu_item_enabled_115_r + 1))->b11 = 1;
                PlayBankSound_r(1, 0, 0);
                ShowBanner_r(0x542D, -1);
            }
        }
        if (d->h26 != 0 && r->b28 == 0) {
            v21 = 11;
            if (B < 3.0f) B = 0.0f;
        }
        if (((Hero *)(hero_globals_base_r + 0xE1D))->state.current == 32
            || (((Hero *)(hero_globals_base_r + 0xE1D))->state.prev == 32
                && ((Hero *)(hero_globals_base_r + 0xE1D))->state_timer < scale_ticks_r(30))) {
            if (2.0f < B) B = 2.0f;
        }
        if (r->h2A == m->oclass) {
            B = 0.0f;
            v21 = 11;
        }
        if ((unsigned int)(m->prev_seq - 6) < 2) {
            if (((Hero *)(hero_globals_base_r + 0xE1D))->vehicle_mode == 1
                && r->f20 == ((Hero *)(hero_globals_base_r + 0xE1D))->moby) {
                B = 0.0f;
                v21 = 11;
            }
        }
        if (r->h2A == 0x47) {
            *(u128_2E52B0 *)&vA = *(u128_2E52B0 *)(hero_globals_base_r + 0xE9D);
            *(u128_2E52B0 *)&vB = *(u128_2E52B0 *)((V_2E52B0 *)&m->pos);
            vA.z += 1.0f;
            vB.z += 1.0f;
            if (coll_line_r(&vA, &vB, 2, m, 0) != 0) {
                v21 = 11;
                B = 0.0f;
            }
        } else if ((q = r->f20) != 0 && q->oclass != 0x359) {
            *(u128_2E52B0 *)&vA = *(u128_2E52B0 *)((V_2E52B0 *)&q->pos);
            *(u128_2E52B0 *)&vB = *(u128_2E52B0 *)((V_2E52B0 *)&m->pos);
            vB.z += 1.0f;
            if (r->f20->oclass == 0xBA || r->f20->oclass == 0) vA.z += 0.25f;
            if (coll_line_r(&vA, &vB, 0x12, m, 0) != 0) {
                FastVecSub_r(&dst, &vB, &vA);
                *(u128_2E52B0 *)&W = *(u128_2E52B0 *)&dst;
                vec_scale_to_length_r(&W, &W, 1.0f);
                vec_scale_to_length_r(&X, &orxon_ground_hit_normal_r, 1.0f);
                if (0.5f < FastAbsF_r(FastVecDot_r(&W, &X))) {
                    B = 0.0f;
                    v21 = 11;
                }
            }
        }
        d->f20 -= B;
        if (d->f20 <= 0.0f) v21 = 1;
        activate_waiting_moby_group_members_r(m);
        d->f90 = truncate_float_to_s32_r(768.0f);
        d->f98 = 0.75f;
        d->f94 = 9;
        d->bAD = 0;
        fx = fast_atan2_r(r->pos.x, r->pos.y);
        switch (v21) {
        case 3:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            m->state = 10;
            d->h26 = scale_ticks_r(60);
            d->f80 = D_L10_00161EAC_2E52B0 * g_frame_time_sq_r;
            trooper_set_knockback_r(d->a70, 0.5f, 0.5f);
            d->fC0 = 8.0f;
            d->fC4 = 10.0f;
            *(u128_2E52B0 *)&vA = *(u128_2E52B0 *)&r->pos;
            HeadingFromVector_r(&vA, &fx, &d->f88, &d->f8C);
            InitKnockback_r(m, d->a70, 9, 1, 0, fx);
            d->b67 = 0xFA;
            RerollEffectCounter_r(m, d->a60);
            break;
        case 4:
        case 5: {
            float s;
            m->state = 10;
            d->h26 = scale_ticks_r(60);
            d->f80 = D_L10_00161EAC_2E52B0 * g_frame_time_sq_r;
            s = ((Hero *)(hero_globals_base_r + 0xE1D))->vehicle_mode == 1 ? 1.0f : 3.0f;
            trooper_set_knockback_r(d->a70, s, 0.5f);
            d->fC0 = 3.5f;
            d->fC4 = 7.0f;
            *(u128_2E52B0 *)&vA = *(u128_2E52B0 *)&r->pos;
            HeadingFromVector_r(&vA, &fx, &d->f88, &d->f8C);
            InitKnockback_r(m, d->a70, 9, 1, 0, fast_atan2_r(r->pos.x, r->pos.y));
            d->b67 = 0xFA;
            RerollEffectCounter_r(m, d->a60);
            break;
        }
        case 1:
        case 2:
            m->state = 11;
            m->flags &= 0xEFFF;
            break;
        case 0:
        case 11:
            break;
        }
    }
    m->hit_slot = 0xFF;
    StepColorFade_r(m, d->a60);
    if ((unsigned char)(m->state - 10) < 3) {
        d->f1D4 = 0;
        return;
    }
    if (d->f1AC != 0) {
        d->f1A0 = D_L10_00161EC0_2E52B0 + 6.0f;
    } else if (m->state == 1) {
        if (m == ((Moby *)((Hero *)(hero_globals_base_r + 0xE1D))->unk240) || m == ((Moby *)((Hero *)(hero_globals_base_r + 0xE1D))->coll_hit_moby)) {
            d->f1A0 = 6.0f;
        } else {
            d->f1A0 = 0.0f;
        }
        if (((Hero *)(hero_globals_base_r + 0xE1D))->vehicle_mode == 1) activate_waiting_moby_group_members_r(m);
    } else {
        d->f1A0 = D_L10_00161EC0_2E52B0;
    }
    if (d->f1AC != 0) {
        FindNearestTargetMoby_r(m, &d->a120, d->f1A0);
        if (d->f1A0 < vec_distance_xy_r(&d->a170, &d->a120) || 3.0f < FastAbsF_r(m->pos.z - d->a120.z)) {
            d->f164 = 2;
        }
    } else if (d->f1A0 < vec_distance_xy_r(&d->a170, &d->a120) || 3.0f < FastAbsF_r(m->pos.z - d->a120.z)) {
        FindNearestTargetMoby_r(m, &d->a120, d->f1A0);
        d->f164 = 2;
    } else {
        FindNearestEligibleMoby_r(d->f1A0, m, &d->a120, 0, 0, orxon_paths_r[d->f1B0]->nodes,
            orxon_paths_r[d->f1B0]->count);
    }
    if (m->state != 1 && d->f164 == 2) {
        d->f1D4++;
        if (scale_ticks_r(300) < d->f1D4) {
            if (1.0f < FastVecDist_r(&m->pos, &d->a170)) {
                m->state = 8;
                if (m->prev_seq != 4) MobyBlendAnimation_r(m, 4, 0, scale_ticks_r(12));
            } else if (((Hero *)(hero_globals_base_r + 0xE1D))->vehicle_mode != 1) {
                m->state = 1;
                if (m->prev_seq != 0) MobyBlendAnimation_r(m, 0, 0, scale_ticks_r(20));
            }
        }
    } else {
        d->f1D4 = 0;
    }
    if (d->f160 == 0) d->f160 = ((Hero *)(hero_globals_base_r + 0xE1D))->moby;
}
