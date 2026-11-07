int clear_menu_preview_pair(MenuScreen *arg0) {
    arg0->data.preview.second_moby = (struct Moby *)delete_moby(
        (int)(arg0->data.preview.moby = (struct Moby *)delete_moby((int)arg0->data.preview.moby)));
    return 0;
}
