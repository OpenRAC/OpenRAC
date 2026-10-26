void update_screamer_hit_reactions_and_targeting(struct Moby *moby) {
    extern unsigned char menu_item_enabled_115[];
    extern void MobyBlendAnimation(void *, int, int, int);
    extern int ComputeHitResponse(void *, void *, void *, int, int *, float *, int, int);
    extern void InitKnockback(void *, void *, float, int, int, int);
    extern void RerollEffectCounter(void *, void *);
    extern void StepColorFade(void *, void *);
    extern void spawn_randomized_bolt_drop(void *, int, int, int, int);
    extern int FindNearestEligibleMoby(float, void *, void *, int, int, void *, int);
    extern void matrix_mul_vec3(void *, void *, void *);
    extern void PlayBankSound(int, int, int);
    extern void ShowBanner(int, int);
    MobyData_2E0FF8 *data = (MobyData_2E0FF8 *)moby->pvars;
    Hit_2E0FF8 *r;
    float v[4];
    float w[4];
    int st;
    int r19;
    float ang;
    unsigned char *f;

    if (data->f38 != 0) {
        data->f184 = truncate_float_to_s32(multiply_global_factor(random_float_between(180.0f, 240.0f)));
        data->f38 = 0;
    }
    fast_dec_timer(&data->f184);
    if (moby->state != 11) {
        r = (Hit_2E0FF8 *)MobyGetHitMessage(moby, 0x330000, 0);
        ComputeHitResponse(moby, r, &data->f20, 5, &st, 0, 0, 4);
        if (r != 0) {
            if (data->f20 <= r->f2C) {
                if (hero_inside_conditional_help_volume != 0 && (f = menu_item_enabled_115 + 1, f[0x10]) == 0) {
                    f[0x10] = 1;
                    PlayBankSound(1, 0, 0);
                    ShowBanner(0x53D6, -1);
                }
                data->f20 = 0.0f;
                moby->flags &= 0xEFFF;
                if (moby->prev_seq != 5) MobyBlendAnimation(moby, 5, 0, scale_ticks(10));
                moby->state = 11;
                data->f70 = g_frame_time_sq * 26.0f;
                data->f9D = 3;
                data->f74 = 0.0005f;
                data->f84 = 9;
                data->f78 = g_frame_time * 7.0f;
                data->f7C = g_frame_time * 10.0f;
                if (r->f30 & 1) ang = fast_atan2(r->f10[0], r->f10[1]);
                else ang = fast_atan2(moby->pos.x - r->x, moby->pos.y - r->y);
                InitKnockback(moby, data->f60, ang, 5, 1, 0);
                data->f117 = 0x78;
                RerollEffectCounter(moby, data->f110);
                spawn_randomized_bolt_drop(moby, 1, 3, 0, -1);
            } else {
                data->f20 = data->f20 - r->f2C;
                data->f117 = 0xFA;
                data->f26 = scale_ticks(60);
                RerollEffectCounter(moby, data->f110);
            }
            moby->hit_slot = 0xFF;
        }
    }
    StepColorFade(moby, data->f110);
    if (data->f174 == 2) data->f180 = 42.0f;
    else if (data->f184 != 0) data->f180 = data->f178 + 6.0f;
    else if (moby->state == 1) data->f180 = data->f188;
    else data->f180 = data->f178;

    if (data->f174 == 2) {
        r19 = FindNearestTargetMoby(data->f180, moby, data->fC0);
        if (r19 != 2) {
            Anim_2E0FF8 *bp = &((Anim_2E0FF8 *)orxon_scene_points)[data->f194];
            FastVecSub(v, hero_globals_base + 0xE9D, bp->f30);
            v[3] = 0.0f;
            matrix_mul_vec3(w, v, bp->f40);
            if (w[0] <= -1.0f || 0.0f <= w[0] || w[1] <= -1.0f || 1.0f <= w[1] || w[2] <= -1.0f || 1.0f <= w[2]) data->f104 = 2;
        }
    } else if (moby->state == 1 && data->f18C >= 0) {
        r19 = FindNearestEligibleMoby(data->f180, moby, data->fC0, 0, 0, ((Path_2E0FF8 **)orxon_paths)[data->f18C]->pts, ((Path_2E0FF8 **)orxon_paths)[data->f18C]->count);
    } else if (moby->state != 1 && data->f17C >= 0) {
        r19 = FindNearestEligibleMoby(data->f180, moby, data->fC0, 0, 0, ((Path_2E0FF8 **)orxon_paths)[data->f17C]->pts, ((Path_2E0FF8 **)orxon_paths)[data->f17C]->count);
    } else {
        r19 = FindNearestTargetMoby(data->f180, moby, data->fC0);
    }
    if (r19 != 2) {
        if (data->f180 < vec_distance_xy(data->f130, data->fC0)) data->f104 = 2;
        else if (moby->visible == 0) data->f104 = 2;
    }
    if (data->f100 == 0) data->f100 = *(int *)(hero_globals_base + 0x2E9D);
}
