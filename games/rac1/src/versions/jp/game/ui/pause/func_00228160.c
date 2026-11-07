void update_skill_point_and_gold_weapon_menu_entries(void) {
    int a = count_skill_points() >= 15;
    int b = count_skill_points() >= 30;
    int c = count_gold_weapons() >= 10;

    {
        char *g = reward_menu_items;
        *(short *)(g + 0x3E) = a ? 3 : 2;
    }
    {
        char *g = reward_menu_items;
        *(short *)(g + 0x4A) = b ? 3 : 2;
    }
    {
        char *g = reward_menu_items;
        *(short *)(g + 0x56) = c ? 10 : 2;
    }
    {
        char *g = reward_menu_items;
        *(short *)(g + 0x62) = c ? 3 : 2;
    }
    {
        char *h = reward_menu_page;
        *(int *)(h + 0x14) = a ? 0x4FD4 : 0x4FDA;
        *(int *)(h + 0x18) = b ? 0x4FD5 : 0x4FDB;
        *(int *)(h + 0x1C) = c ? 0x4FD8 : 0x4FDC;
        *(int *)(h + 0x20) = c ? 0x4FD9 : 0x4FDC;
    }
    {
        char *g = reward_menu_items;
        *(short *)(g + 0x60) = 0;
    }
}
