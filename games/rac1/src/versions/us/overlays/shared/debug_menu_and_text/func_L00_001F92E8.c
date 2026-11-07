void draw_framebuffer_with_timed_text(void) {
    DbgRec *p;
    int n;

    if (skip_frame == 0) {
        queue_framebuffer_upload(slideshow[2]);
        if (0.0f < gFadeLvl) {
            if (1.0f < gFadeLvl) {
                gFadeLvl = 1.0f;
            }
            emit_rgba_draw_packet(0, 0, 0, truncate_float_to_s32(gFadeLvl * 128.0f));
        }
        ResetGsRegisters();
        ((void (*)(int))SetupGifPaging)(0);
        p = timed_text_records;
        n = 0xB6;
        do {
            draw_timed_text(p, ((short *)slideshow)[3]);
            p++;
        } while (--n >= 0);
        DoGifPaging();
    }
}
