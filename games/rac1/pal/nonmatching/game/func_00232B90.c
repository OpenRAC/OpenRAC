/*
 * Loads the loading-slide images for a language and the two slides from the disc: reads
 * the archive, decompresses it, then uploads six images to GS memory and writes the
 * three TEX0 values (shared, first slide, second slide) through the pointers.
 */
extern int D_001941D4;   /* archive memory: the archive base is at +0x14 of D_001941C0 */
extern int D_0015EF74;   /* gs texture allocation cursor */
extern int D_0015EF78;   /* gs texture allocation start */
extern int D_0015EF8C;   /* gs texture allocation base */
extern void func_002175C8(void *, int, int);
extern void func_0020C468(int, int);
/* sceGsLoadImage: twelve doublewords (giftag, BITBLTBUF, TRXPOS, TRXREG, TRXDIR, image giftag
 * with their A+D addresses). Retail keeps it at sp+0x20 with 0x60 bytes of room. */
typedef struct {
    u64 q[12];
} LoadImage_32B90 __attribute__((aligned(16)));
extern void func_00122630(void *, int, int, int, int, int, int, int);
extern void func_00122958(void *, int);

void func_00232B90(s32 language_index, s32 first_slide, s32 second_slide,
                   u64 *shared_texture, u64 *first_texture, u64 *second_texture)
{
    s32 texture_bases[6];
    LoadImage_32B90 load_image;
    s32 upload_index;
    s32 *texture_base_output;
    s32 upload_bytes;
    s32 image_address;
    s32 archive_base;
    char *slot;
    u64 texture_bits;
    u64 image_bits;

    slot = (char *)&D_00137C80_t + language_index * 8;
    func_002175C8((void *)(D_001941D4 + 0x100000), *(s32 *)(slot + 0x1388), *(s32 *)(slot + 0x138C));
    func_00120F30(0);
    func_00118D80(0);
    archive_base = D_001941D4;
    func_0020C468(archive_base + 0x100000, archive_base);
    func_00118D80(0);
    archive_base = D_001941D4;
    D_0015EF78 = D_0015EF8C;
    D_0015EF74 = D_0015EF8C;
    texture_base_output = texture_bases;
    for (upload_index = 0; upload_index < 6; upload_index++) {
        if (upload_index == 0) {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 1, 0, 0, 0, 0x10, 0x10);
            upload_bytes = 0x400;
            image_address = (s32)((char *)archive_base + *(s32 *)((char *)archive_base + 4)) + 0x20;
        } else if (upload_index == 1) {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 1, 0x13, 0, 0, 0x40, 0x40);
            upload_bytes = 0x1000;
            image_address = (s32)((char *)archive_base + *(s32 *)((char *)archive_base + 4)) + 0x420;
        } else if (upload_index == 2) {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 1, 0, 0, 0, 0x10, 0x10);
            upload_bytes = 0x400;
            image_address = (s32)((char *)archive_base +
                                  *(s32 *)((char *)archive_base + first_slide * 4 + 8)) + 0x20;
        } else if (upload_index == 3) {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 8, 0x13, 0, 0, 0x200, 0x40);
            upload_bytes = 0x8000;
            image_address = (s32)((char *)archive_base +
                                  *(s32 *)((char *)archive_base + first_slide * 4 + 8)) + 0x420;
        } else if (upload_index == 4) {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 1, 0, 0, 0, 0x10, 0x10);
            upload_bytes = 0x400;
            image_address = (s32)((char *)archive_base +
                                  *(s32 *)((char *)archive_base + second_slide * 4 + 8)) + 0x20;
        } else {
            func_00122630(&load_image, (D_0015EF74 << 8) >> 16, 8, 0x13, 0, 0, 0x200, 0x40);
            upload_bytes = 0x8000;
            image_address = (s32)((char *)archive_base +
                                  *(s32 *)((char *)archive_base + second_slide * 4 + 8)) + 0x420;
        }
        func_00118D80(0);
        func_00122958(&load_image, image_address);
        func_00120858(0, 0);
        *texture_base_output = D_0015EF74 >> 8;
        D_0015EF74 = D_0015EF74 + upload_bytes;
        texture_base_output++;
    }
    texture_bits = ((u64)texture_bases[0] << 37) | (0xB000ULL << 19);
    image_bits = (u64)(texture_bases[1] | 0x19304000);
    *shared_texture = (image_bits | texture_bits) | (1ULL << 63);
    texture_bits = ((u64)texture_bases[2] << 37) | (0xB000ULL << 19);
    *first_texture = (texture_bases[3] | 0x25320000) | texture_bits | (1ULL << 63);
    *second_texture = (((u64)texture_bases[4] << 37) | (0xB000ULL << 19)) |
                      (texture_bases[5] | 0x25320000) | (1ULL << 63);
}
