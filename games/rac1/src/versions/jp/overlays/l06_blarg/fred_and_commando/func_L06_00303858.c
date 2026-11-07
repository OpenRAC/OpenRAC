void queue_moby_dialogue_line(char *moby, int mode) {
    Pick *data;
    int id;
    int r;
    int i;
    int n = 5;
    data = ((Pick *)((Moby *)moby)->pvars);
    id = -1;
    switch (mode) {
        case 0:
            id = data->a[n];
            break;
        case 1:
            r = random_integer_below(3);
            for (i = 0; i < 4; i++) {
                if (((int *)&blarg_random_line_used)[r] == 0) {
                    ((int *)&blarg_random_line_used)[r] = 1;
                    break;
                }
                r = (r + 1) % 3;
                if (i == 2) {
                    *(int *)&blarg_random_line_used = 0;
                    blarg_random_line_used_1 = 0;
                    blarg_random_line_used_2 = 0;
                }
            }
            id = data->c[r][n];
            break;
        case 2:
            id = data->b[n];
            break;
    }
    if (id != -1)
        queued_dialogue_id = id + 0x4E20;
}
