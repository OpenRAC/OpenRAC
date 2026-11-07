void memcard_get_name(void *arg0) {
    unsigned char *a = (unsigned char *)arg0;
    unsigned char region = a[0x12];
    int i;

    if (region == 'E') {
        memcard_save_dir_name[2] = region;
    }
    for (i = 3; i < 7; i++) {
        memcard_save_dir_name[i] = a[i + 0xD];
    }
    for (i = 8; i < 0xB; i++) {
        memcard_save_dir_name[i] = a[i + 0xD];
    }
    for (i = 0xB; i < 0xD; i++) {
        memcard_save_dir_name[i] = a[i + 0xE];
    }
    libc_strncpy(memcard_dir_search_path, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_a, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_b, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_c, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_icon_path_c + 0x14, memcard_save_dir_name, 0xD);
    libc_strncpy(memcard_slot_path_format, memcard_save_dir_name, 0xD);
}
