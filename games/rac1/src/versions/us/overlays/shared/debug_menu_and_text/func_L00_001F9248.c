void draw_dialog_background_and_text(void) {
    if (skip_frame == 0) {
        if (GetDebugOverlayMode() == 0) {
            append_gif_transfer_packet();
            debug_profiler_color = 0x100FF;
            DrawDebugProfiler();
        } else {
            if (GetDebugOverlayMode() == 3)
                draw_pause_menu_pages(1);
        }
        emit_rgba_draw_packet(0, 0, 0, 0x40);
        draw_freeze_message_box();
    }
}
