/*
 * func_L04_00378838: C left beside its INCLUDE_ASM marker by a sol-low carry worker (v0038); the
 * strict build placed it in level 04 at 272 of 288 bytes. Not matched.
 */
int func_L04_00378838(char *obj) {
    int i, entry;
    int *list;
    for (i = 0; i < 28 && D_00139948[i]; i++) {
        D_L04_0021D7B0[i].kind = 3;
        D_L04_0021D7B0[i].callback = D_L04_0029FEE0;
        D_L04_0021D7B0[i].id = D_L04_002A4420[D_00139948[i]].id;
        D_L04_0021D7B0[i].value = D_L04_002A4420[D_00139948[i]].value;
    }
    list = D_001AA770;
    D_L04_0021D7B0[i].id = 0;
    *(int *)(obj + 0x40) = 0;
    *(unsigned int *)(obj + 0x30) |= 0x8000;
    for (i = 0, entry = list[0]; entry; entry = D_001AA770[++i]) {
        if (D_L04_001C9C60.selected == entry) {
            *(int *)(obj + 0x40) = i;
            break;
        }
    }
    return 0;
}
