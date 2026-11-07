extern int D_0018CC14;
extern int D_00161174 SDATA(D_00161174);
extern void func_J_00239ED0(unsigned char *);
extern void func_J_001F78F0(int, float, float, char *, int, float);

void update_scrolling_status_message(void) {
    char *v;
    char *w;
    draw_framebuffer_rect(0, 0, 0x200, 0x80, 0x200, 0x80, 0);
    if ((game_frame_counter & 0x1F) < 0x10) {
        D_0018CC14 = D_00161174 & 0xFFFFFF;
    } else {
        D_0018CC14 = D_00161174;
    }
    v = vendor_state;
    if (libc_strlen(*(char **)(v + 0x2C)) - *(int *)(v + 0x44) / 20 > 0) {
        *(int *)(v + 0x44) += 2;
    } else {
        func_J_00239ED0((unsigned char *)msg_string(scrolling_status_message_ids[random_integer_below(0x18)]));
    }
    w = vendor_state;
    func_J_001F78F0(D_00161174, -*(int *)(w + 0x44), 7.0f, *(char **)(w + 0x2C), -1, 1.25f);
    draw_framebuffer_rect(0, 0, 4, 0x40, 0x200, 0x80, 0);
    draw_framebuffer_rect(0xE2, 0, 0xE6, 0x40, 0x200, 0x80, 0);
}
