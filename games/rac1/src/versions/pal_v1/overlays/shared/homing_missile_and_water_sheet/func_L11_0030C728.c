extern V4_30C728 D_E1_L11_001627C0 SDATA(D_E1_L11_001627C0);
void ocean_pokitaru_update(Moby *moby) {
    OceanVars *d = (OceanVars *)moby->pvars;
    float src[4];
    float out[4];
    int k;

    if (moby->state == 0) {
        moby->state = 1;
        moby->update_dist = 0xFF;
        for (k = 0; k < 2; k++) {
            wave_offset_r[k].u = 0.0f;
            wave_offset_r[k].v = 0.0f;
        }
        qcopy(D_E1_L11_001627C0, camera_pos_b358_r);
        pokitaru_ocean_base_height = d->level;
        return;
    }
    FastVecSub(src, ((Cam_30C728 *)pokitaru_player_camera_r)->pos, D_E1_L11_001627C0);
    qcopy(D_E1_L11_001627C0, ((Cam_30C728 *)pokitaru_player_camera_r)->pos);
    if (!(d->limit < ((Cam_30C728 *)pokitaru_player_camera_r)->pos[2])) {
        return;
    }
    for (k = 0; k < 2; k++) {
        FastVecScale(out, src, (D_L11_00161E88_30C728[k] + D_L11_00161E88_30C728[k]) / (d->scale * 5.0f));
        if (1.0f < out[0] || out[0] < 1.0f) {
            out[0] = out[0] - IntToFloat(truncate_float_to_s32_r2(out[0]));
        }
        if (1.0f < out[1] || out[1] < 1.0f) {
            out[1] = out[1] - IntToFloat(truncate_float_to_s32_r2(out[1]));
        }
        wave_offset_r[k].u = wave_offset_r[k].u + out[0];
        wave_offset_r[k].v = wave_offset_r[k].v + out[1];
        if (1.0f < wave_offset_r[k].u) {
            wave_offset_r[k].u = wave_offset_r[k].u - 1.0f;
        }
        if (wave_offset_r[k].u < -1.0f) {
            wave_offset_r[k].u = wave_offset_r[k].u + 1.0f;
        }
        if (1.0f < wave_offset_r[k].v) {
            wave_offset_r[k].v = wave_offset_r[k].v - 1.0f;
        }
        if (wave_offset_r[k].v < -1.0f) {
            wave_offset_r[k].v = wave_offset_r[k].v + 1.0f;
        }
    }
    pokitaru_ocean_height_r =
        pokitaru_ocean_base_height + FastSin(IntToFloat(level_frame_r % 360) * 0.017444445f - 3.14f) * 0.25f;
    for (k = 0; k < 2; k++) {
        float sum = wave_offset_r[k].u + pokitaru_wave_scroll_speed[k].u * frame_delta_time;
        wave_offset_r[k].u = sum;
        if (1.0f < sum) {
            wave_offset_r[k].u = sum - 1.0f;
        }
        if (wave_offset_r[k].u < -1.0f) {
            wave_offset_r[k].u = wave_offset_r[k].u + 1.0f;
        }
        sum = wave_offset_r[k].v + pokitaru_wave_scroll_speed[k].v * frame_delta_time;
        wave_offset_r[k].v = sum;
        if (1.0f < sum) {
            wave_offset_r[k].v = sum - 1.0f;
        }
        if (wave_offset_r[k].v < -1.0f) {
            wave_offset_r[k].v = wave_offset_r[k].v + 1.0f;
        }
    }
    AddDrawCallback_r((void *)draw_pokitaru_ocean_r, moby);
}
