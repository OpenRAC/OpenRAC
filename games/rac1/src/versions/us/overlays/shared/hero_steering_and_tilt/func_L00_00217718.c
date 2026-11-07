void set_race_camera(void *a, void *b, int c, int d) {
    char *p = &((struct Hero *)((char *)&hero_globals_base + 0xE1D))->motion.pos;
    char *q;
    qcopy(p, a);
    q = p - 0x80;
    qcopy(*(char **)(q + 0x2080) + 0x10, a);
    qcopy(p + 0x10, b);
    euler_to_matrix(q, p + 0x10);
    euler_rotation_basis(*(char **)(q + 0x2080) + 0xC0, p + 0x10);
    clear_hero_target_and_ground_speeds();
    clear_moby_transform_history(p + 0x15F0);
    *(int *)(q + 0x364) &= -2;
    *(short *)(q + 0x30E) = 1;
    *(int *)(q + 0x364) &= -3;
    *(int *)(q + 0x360) = 0;
    clear_u64_value(p + 0x70);
    clear_u64_value(p + 0x60);
    clear_u64_value(p + 0x80);
    clear_u64_value(p + 0x90);
    clear_u64_value(p + 0xA0);
    clear_u64_value(p + 0xD0);
    clear_u64_value(p + 0x20);
    *(int *)(q + 0x184) = 0;
    *(char *)(q + 0x22CA) = 0;
    reset_pad_history();
    cancel_hero_item_animation();
    reset_hero_easing_records();
    if (c != -1)
        HeroEnterMoveState(c, 1);
    HeroStartSequenceInBank(*(unsigned char *)(*(char **)(q + 0x2080) + 0x53),
                            *(unsigned char *)(*(char **)(q + 0x2080) + 0x51));
    if (*(int *)(q + 0x208C) == 0x11)
        hero_control_mode_11_flag = 1;
    else
        hero_control_mode_11_flag = 0;
    update_hero_ground_contact_and_surface_slopes();
    if (d)
        init_camera_system();
    apply_nearest_environment_sample(*(int *)&((struct Hero *)((char *)&hero_globals_base + 0xE1D))->moby);
}
