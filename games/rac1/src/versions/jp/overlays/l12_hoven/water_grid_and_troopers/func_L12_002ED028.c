void process_helicopter_hits_and_reactivation(char *m, int flag) {
    char *d = *(char **)(m + 0x78);
    float a[4];
    float b[4];
    unsigned char *q;
    if (MobyGetHitMessage(m, 0xA30000, 0) != 0) {
        if (*(int *)hero_current_state != 0x32) {
            helicopter_hit_count += 1;
            if (helicopter_hit_count >= 5) {
                q = (unsigned char *)gSkillPoints;
                if (q[0x15] == 0) {
                    q[0x15] = 1;
                    allocate_voice_for_bank_entry(1, 0, 0);
                    queue_dialogue_message(0x542D, -1);
                }
            }
        }
        qcopy(a, m + 0x10);
        clear_u64_value(b);
        spawn_randomized_bolt_drop(m, 0x1F, 0x4A, 2, -1);
        *(int *)(d + 0xA4) = scale_ticks(500);
        spawn_explosion_v(m, b, a, 0.0f, 0.0f, 0x14, 3, 4, 4.0f, 2.0f, 100000.0f, -1, 3.0f, 15.0f, 1, 1, -1, 0);
    }
    *(unsigned char *)(m + 0xA4) = 0xFF;
    if (planetbuster_engaged != 0 && flag != 0) {
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) = (*(unsigned short *)(m + 0x34) | 0x41) & 0xEFFF;
    }
    if (*(int *)(d + 0xA4) == 0 && (*(unsigned short *)(m + 0x34) & 1) == 0)
        return;
    if (fast_dec_timer((int *)(d + 0xA4)) != 0 && flag != 0 && planetbuster_engaged == 0) {
        *(int *)(m + 0x94) = *(int *)(*(char **)(m + 0x24) + 0x10);
        *(unsigned short *)(m + 0x34) = (*(unsigned short *)(m + 0x34) & 0xFFBE) | 0x1000;
    } else {
        *(int *)(m + 0x94) = 0;
        *(unsigned short *)(m + 0x34) = (*(unsigned short *)(m + 0x34) | 0x41) & 0xEFFF;
    }
}
