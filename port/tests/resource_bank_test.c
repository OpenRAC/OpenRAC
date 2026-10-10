/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1-pal/hand/game_resource_bank.c"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); exit(1); } } while (0)
_Static_assert(sizeof(BankMaterial)==0x20, "material stride");
_Static_assert(sizeof(BankItem)==0x4C, "item stride");
#if __SIZEOF_POINTER__ == 4
_Static_assert(sizeof(BankGroup)==0x10, "EE group stride");
_Static_assert(offsetof(BankModel,count2)==6, "EE upgrade flag");
_Static_assert(offsetof(BankModel,materials)==0x28, "EE material pointer");
_Static_assert(offsetof(BankModel,raw_value)==0x2C, "EE serialized value");
#endif
int bank_count, bank_current, bank_buffer;
unsigned int bank_texture_base;
int bank_classes[5];
unsigned char* bank_compressed[5];
unsigned char bank_setup[0x30], bank_slots[256];
BankModel* bank_models[256];
unsigned int bank_raw_values[256];
unsigned char bank_textures[16], bank_material_maps[5][16];
short bank_material_indices[5][16];
BankItem bank_items[37];
unsigned char bank_upgraded[37];
unsigned long long bank_upgrade_state[4];
static union { unsigned long long alignment; unsigned char bytes[0x30000]; } buffers;
static BankModel template_model;
static BankMaterial materials[17];
static BankGroup groups[2];
static unsigned long long packets[2][20];
static int events, selected, selected_buffer, requested_class, change_index;
void bank_flush(int mode) {
    CHECK(mode==0 && (events==0 || events==2));
    CHECK(bank_current==selected && bank_buffer==selected_buffer); ++events;
}
int bank_decompress(void* src, void* dst) {
    CHECK(events++==1 && src==bank_compressed[selected]);
    CHECK(dst==buffers.bytes+selected_buffer*0x18000);
    memcpy(dst,&template_model,sizeof(template_model)); return (int)sizeof(template_model);
}
void bank_relocate(BankModel* model, void* textures, void* map, int class_id) {
    CHECK(events++==3 && class_id==requested_class);
    CHECK(model==bank_models[bank_slots[class_id]]);
    CHECK(bank_raw_values[bank_slots[class_id]]==0x12345678u);
    CHECK(textures==bank_textures && map==bank_material_maps[selected]);
    model->raw_value=0;
    /* Material fixups must use post-relocation pointers and current index. */
    model->materials=materials;
    bank_texture_base=0xFEDCBA98;
    if(change_index) bank_current=4;
}
static void reset(void) {
    memset(&buffers,0xCC,sizeof(buffers)); memset(&template_model,0,sizeof(template_model));
    memset(bank_models,0,sizeof(bank_models)); memset(bank_raw_values,0,sizeof(bank_raw_values));
    memset(bank_upgraded,0,sizeof(bank_upgraded)); memset(bank_slots,0,sizeof(bank_slots));
    memset(materials,0xAA,sizeof(materials)); memset(packets,0x55,sizeof(packets));
    bank_count=3; bank_current=-1; bank_buffer=0; bank_texture_base=0;
    for(int i=0;i<5;++i) {
        bank_classes[i]=10+i; bank_compressed[i]=bank_textures+i;
        for(int j=0;j<16;++j) bank_material_indices[i][j]=(short)(j%2?-1:i*100+j);
    }
    for(int i=0;i<37;++i) bank_items[i].class_id=100+i;
    *(unsigned char**)(bank_setup+0x10)=buffers.bytes;
    template_model.raw_value=0x12345678;
    template_model.groups=groups; template_model.last_group=1;
    groups[0].packet=(unsigned char*)packets[0]; groups[0].quadwords=5;
    groups[1].packet=(unsigned char*)packets[1]; groups[1].quadwords=6;
    bank_upgrade_state[0]=0xFEDCBA9876543210ULL;
    bank_upgrade_state[2]=0x0123456789ABCDEFULL;
    events=0; selected=2; selected_buffer=1; requested_class=12; change_index=0;
    bank_slots[12]=255;
}
static void run(int buffer) {
    unsigned char untouched[0x18000];
    memcpy(untouched,buffers.bytes+(1-selected_buffer)*0x18000,sizeof(untouched));
    func_00205270(requested_class,buffer);
    CHECK(events==4 && bank_buffer==selected_buffer);
    CHECK(bank_models[bank_slots[requested_class]]==(BankModel*)(buffers.bytes+selected_buffer*0x18000));
    CHECK(!memcmp(untouched,buffers.bytes+(1-selected_buffer)*0x18000,sizeof(untouched)));
    for(int j=0;j<16;++j) {
        if(j%2) {
            CHECK((unsigned short)materials[j].index==0xAAAA && materials[j].texture_base==0xAAAAAAAAu);
        } else {
            CHECK(materials[j].index==(change_index?400:selected*100)+j);
            CHECK(materials[j].texture_base==0xFEDCBA98);
        }
        for(unsigned i=0;i<sizeof(materials[j].reserved);++i) CHECK(materials[j].reserved[i]==0xAA);
    }
    CHECK(materials[16].texture_base==0xAAAAAAAAu);
}
int main(void) {
    reset(); run(-1); CHECK(bank_current==2);
    /* Already selected: no load, no bank toggle and no table writes. */
    events=0; bank_buffer=7; func_00205270(12,-1); CHECK(!events && bank_buffer==7);
    reset(); bank_buffer=1; selected_buffer=0; run(-1);
    reset(); selected_buffer=0; run(0);
    reset(); bank_buffer=1; run(1);
    reset(); requested_class=10; selected=0; run(-1); CHECK(bank_current==0);
    reset(); change_index=1; run(-1); CHECK(bank_current==4);
    reset(); bank_items[36].class_id=12; bank_upgraded[36]=1; template_model.count2=1;
    run(-1);
    for(int i=0;i<20;++i) {
        CHECK(packets[0][i]==0x5555555555555555ULL);
        CHECK(packets[1][i]==(i==8?bank_upgrade_state[0]:i==10?bank_upgrade_state[2]:0x5555555555555555ULL));
    }
    reset(); bank_items[0].class_id=12; bank_items[1].class_id=12;
    bank_upgraded[1]=1; template_model.count2=1; run(-1);
    CHECK(packets[1][8]==0x5555555555555555ULL); /* first matching item wins */
    reset(); bank_items[0].class_id=12; bank_upgraded[0]=1; run(-1);
    CHECK(packets[1][8]==0x5555555555555555ULL); /* no upgrade group */
    reset(); bank_count=0; selected=0; run(-1); CHECK(bank_current==0);
    reset(); bank_count=2; selected=2; run(-1); CHECK(bank_current==2); /* terminal slot */
    puts("resource bank: OK"); return 0;
}
