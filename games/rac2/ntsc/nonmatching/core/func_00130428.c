/* NEAR MISS - sol-high, 28 October 2026, task v0659.
   USA v1.01; SDK -O2 -G2 -ffunction-sections; 680/680 bytes, 0/1 places.
   Final check.py first difference: +0xC8. There are 22 non-call instruction-
   position differences, all in setup from +0xC8 through +0x11C; instructions
   after that region and both epilogues agree with retail.

   Sequence-header reference-image resize: update macroblock counts, and when
   aligned output dimensions change, rebuild three frame buffers and nine
   plane descriptors. Based on the earlier transcription of Lombyte's public
   reconstruction src/assembly/sdk/library/sequenceheader.c (MIT),
   func_0012C4C8. The corresponding RAC1 entry is func_0012C608.

   Improvement over the retained +0xB4 near miss: define the final plane pointer
   separately on the width-change and height-change paths and join at resize.
   This reproduces both comparison delay slots (addiu v0,fp,0x528) and keeps
   ld ra in the shared epilogue. The remaining spill of this pointer occurs
   immediately at the join, rather than after the first multiplication;
   subsequent setup scheduling differs until the first allocator-reset call.
   The diagnostic sched2 dump puts the pointer definition/spill in its own
   basic block before the setup block, constraining further scheduling.

   Moving the product expression earlier or adding a separate local pointer
   copy gives identical bytes. Reversing the product grouping changes multiply
   operands without improving setup. A six-pointer local array gives 636 bytes
   and different allocation; duplicating dimension stores across the paths
   regresses to +0x4. Stop on the repeated identical results. Next useful input:
   evidence for the original common-block boundary or pointer representation.

   Task v0659's suggested sibling func_00122B00 is unrelated software double
   multiplication, despite also being 680 bytes; its C is not a template here.
   No old orch/main compiler options are used. Required declarations follow.
*/
extern void func_0012FB70(void *);
extern int func_0012FB80(void *, void *, unsigned int, unsigned int);
extern void func_001306D0(unsigned int *, unsigned int *, unsigned int *, unsigned int *,
                          unsigned int *, unsigned int *, unsigned int *, unsigned int *,
                          unsigned int *, int, int, int, int, int);
extern int func_001302E0(void *, int, int);

void func_00130428(void *arg0) {
    unsigned char *sp30;
    unsigned char *sp34;
    unsigned char *sp38;
    unsigned char *sp3C;
    unsigned char *sp40;
    unsigned char *var_2_53;
    int chroma_height;
    int height;
    int width;
    int temp_6_16;
    int var_2_40;
    unsigned int temp_16_77;
    int temp_22_48;
    int temp_23_51;
    int temp_18_75;
    unsigned char *temp_17_63;
    unsigned char *temp_19_67;
    unsigned char *temp_20_71;
    unsigned char *temp_21_73;
    unsigned char *mpeg;

    mpeg = *(unsigned char **)((unsigned char *)arg0 + 0x40);
    temp_6_16 = *(int *)(mpeg + 0x848);
    if (temp_6_16 == 0) {
        *(int *)(mpeg + 0x174) = 3;
        *(int *)(mpeg + 0x13C) = 1;
        *(int *)(mpeg + 0x140) = 1;
        *(int *)(mpeg + 0x188) = 1;
        *(int *)(mpeg + 0x17C) = 1;
        *(int *)(mpeg + 0x144) = 5;
    }
    *(int *)(mpeg + 0x12C) = (*(int *)(mpeg + 0x124) + 0xF) >> 4;
    if (temp_6_16 != 0) {
        if (*(int *)(mpeg + 0x13C) == 0) {
            var_2_40 = ((*(int *)(mpeg + 0x128) + 0x1F) >> 5) * 2;
        } else {
            goto block_6;
        }
    } else {
    block_6:
        var_2_40 = (*(int *)(mpeg + 0x128) + 0xF) >> 4;
    }
    *(int *)(mpeg + 0x130) = var_2_40;
    temp_22_48 = var_2_40 << 4;
    temp_23_51 = *(int *)(mpeg + 0x12C) << 4;
    if (temp_23_51 != *(int *)arg0) {
        var_2_53 = mpeg + 0x528;
        goto resize;
    }
    if (temp_22_48 == *(int *)((unsigned char *)arg0 + 4)) return;
    var_2_53 = mpeg + 0x528;
resize:
    *(int *)((unsigned char *)arg0 + 0x0) = temp_23_51;
    *(int *)((unsigned char *)arg0 + 0x4) = temp_22_48;
    temp_17_63 = mpeg + 0x108;
    sp30 = mpeg + 0x320;
    temp_19_67 = mpeg + 0x1E8;
    sp34 = mpeg + 0x388;
    temp_20_71 = mpeg + 0x250;
    temp_21_73 = mpeg + 0x2B8;
    sp38 = mpeg + 0x3F0;
    temp_16_77 = ((0x180U * (unsigned int)temp_23_51) * (unsigned int)temp_22_48) >> 8;
    sp3C = mpeg + 0x458;
    sp40 = mpeg + 0x4C0;
    width = temp_23_51;
    height = temp_22_48;
    func_0012FB70(temp_17_63);
    *(int *)(mpeg + 0xFC) = func_0012FB80(mpeg, temp_17_63, temp_16_77, 0x40);
    *(int *)(mpeg + 0x100) = func_0012FB80(mpeg, temp_17_63, temp_16_77, 0x40);
    *(int *)(mpeg + 0x104) = func_0012FB80(mpeg, temp_17_63, temp_16_77, 0x40);
    func_001306D0(temp_19_67, temp_20_71, temp_21_73, sp30, sp34, sp38, sp3C, sp40, var_2_53,
                  *(int *)(mpeg + 0xFC), *(int *)(mpeg + 0x100), *(int *)(mpeg + 0x104), width,
                  height);
    func_001302E0(temp_19_67, width, height);
    func_001302E0(temp_20_71, width, height);
    func_001302E0(temp_21_73, width, height);
    temp_18_75 = height >> 1;
    chroma_height = temp_18_75;
    func_001302E0(sp30, width, chroma_height);
    func_001302E0(sp34, width, chroma_height);
    func_001302E0(sp38, width, chroma_height);
    func_001302E0(sp3C, width, chroma_height);
    func_001302E0(sp40, width, chroma_height);
    func_001302E0(var_2_53, width, chroma_height);
}
