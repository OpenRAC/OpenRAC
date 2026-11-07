void update_world_object_animation(void *arg0) {
    struct ModelRec *rec;
    struct Moby *mob;
    int *cp;
    int *sp;
    int i;
    int k;
    int idx;
    int id;
    int off;
    int endp;
    int pc;

    flush_cache(0);
    fast_decompress(render_sequence_t.unk5C, (int)render_sequence_t.rec);
    flush_cache(0);

    rec = render_sequence_t.rec;
    render_sequence_t.unk38 = 0;
    cp = rec->part_off;
    render_sequence_t.unk40 = rec->flags;
    pc = rec->part_count;
    *(unsigned short *)((char *)&render_sequence_t + 0x48) = rec->pad08;
    render_sequence_t.num_parts = rec->num_parts;
    render_sequence_t.unk54 = (int)rec + rec->end_off;
    if (pc < 0x400) {
        render_sequence_t.unk4C = 0;
    } else {
        render_sequence_t.unk4C = (int)rec + pc;
    }

    for (i = 0; i < render_sequence_t.num_parts; i++) {
        off = *cp++;
        sp = (int *)((unsigned char *)rec + off);
        id = sp[0];
        sp = (int *)((unsigned char *)sp + 0xC);
        endp = (int)rec + sp[0];
        sp = (int *)((unsigned char *)sp + 4);
        if (g_game_mode == 6 && i == 0 && id == 0x215) {
            id = cutscene_moby_ids_by_content_variant[level_render_state.slot];
        }
        mob = render_sequence_t.slots[i];
        if (mob == 0) {
            mob = CreateMoby(id);
            idx = ((struct PartList *)mob->pclass)->used;
            ((struct PartList *)mob->pclass)->used = idx + 1;
            mob->seq = idx;
            mob->prev_seq = idx;
            mob->draw_dist = 0x1FF;
            mob->flags |= 6;
            mob->unk72 = 0xFF;
            mob->collision = 0;
            if (((struct ColorSrc *)g_hero.moby) == 0) {
                mob->spawn_frame = 0x38383800000000;
            } else {
                mob->spawn_frame = ((struct ColorSrc *)g_hero.moby)->color;
            }
            if (((struct PartList *)mob->pclass)->flag) {
                mob->unk73 = 0x18;
            }
            render_sequence_t.slots[i] = mob;
        }
        mob->pvars = (u8 *)endp;
        ((struct PartList *)mob->pclass)->entries[mob->seq] = sp;
        for (k = 0; k < ((unsigned char *)sp)[0x10]; k++) {
            int *w = (int *)((unsigned char *)sp + 0x1C);

            w[k] = (int)sp + w[k];
        }
    }
}
