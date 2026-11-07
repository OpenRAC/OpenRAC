int memcard_RestoreData(void *src, int slot, int *table) {
    unsigned char *buf = src;
    struct RestoreEntry *tbl = (struct RestoreEntry *)table;
    struct RestoreEntry *e;
    int errors;
    int total;
    int i;
    int n;
    unsigned char *dst;

    if (memcard_TestChecksum((char *)buf) == 0) {
        return 1;
    }
    buf += 8;
    errors = 0;
    total = 8;
    for (i = 0; tbl[i].data != 0; i++) {
        tbl[i].status = 0;
    }
    while (((struct RestoreBlock *)buf)->id != -1) {
        i = 0;
        if (tbl[0].data == 0) {
            goto missing;
        }
        while (tbl[i].id != ((struct RestoreBlock *)buf)->id) {
            i++;
            if (tbl[i].data == 0) {
                goto missing;
            }
        }
        e = (struct RestoreEntry *)(((unsigned int)i << 4) + (unsigned int)tbl);
        if (e->data != 0) {
            int bsize = ((struct RestoreBlock *)buf)->size;

            dst = e->data + slot * e->size;
            if (bsize == e->size) {
                n = e->size;
                e->status = 1;
            } else if (bsize < e->size) {
                n = bsize;
                e->status = -1;
            } else {
                n = e->size;
                e->status = -2;
            }
            if (libc_memcmp(dst, ((struct RestoreBlock *)buf)->data, n) != 0) {
                memcard_restore_mismatch_count++;
            }
            copy_memory_aligned(dst, ((struct RestoreBlock *)buf)->data, n);
            total += ((n + 3) & ~3) + 8;
        } else {
        missing:
            errors++;
        }
        buf += ((((struct RestoreBlock *)buf)->size + 3) & ~3) + 8;
    }
    total += 8;
    if (total != memcard_GetDataSize((int *)tbl)) {
        errors++;
    }
    buf += 8;
    for (i = 0; tbl[i].data != 0 && tbl[i].id != ((struct RestoreBlock *)buf)->id; i++) {
        if (tbl[i].status <= 0) {
            errors++;
        }
    }
    ((struct {
        char pad[0xAC];
        int errors;
        char pad2[8];
    } *)memory_card_state_i)[memory_card_state_i[0x31]]
        .errors = errors;
    return errors;
}
