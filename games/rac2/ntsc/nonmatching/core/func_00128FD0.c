/* sol-high, v0857: USA v1.01 boot, sdk -O2 -G2 -ffunction-sections.
 * 240/236 bytes, 0/1 places. Supersedes b0264/v0269: initializing result
 * before the bitfield assignments fixes all upper-mask scheduling and
 * registers. Updating first_size with the packet's integer address fixes
 * both operands and allocation of the second-copy destination. The first
 * 0xBC bytes now agree, including resolved call relocations.
 * Remaining: the int argument produces dsll32/dsra32 at +0xBC in place
 * of retail's full-width daddu, moving the final call and epilogue by 4.
 * A Mode128F48 by-value declaration for func_001286C8 makes all 236 bytes
 * match (1/1), but is NOT retained: it changes the existing 32-bit command
 * interface without independent ABI evidence. RAC1's analogous by-value
 * mode call alone does not establish this callee's type. A private callee
 * control taking a bitfield/int union adds two truncation instructions
 * (232 vs the baseline's 224 bytes), so that layout does not qualify it.
 * Packing the bitfields changes construction to 32 bits (216 bytes), also
 * nonmatching. Reopen with independent command-ABI evidence or a supported
 * explanation for omitting the 32-bit truncation. No linked build run.
 * Declarations below are complete; no special flags beyond this SDK unit.
 */
#include "common.h"

typedef union {
    struct {
        unsigned long command : 14;
        unsigned long request : 2;
        unsigned long input : 8;
        unsigned long output : 8;
    } fields;
    int word;
} Pad2Command;
extern void *func_001153D4(void *, const void *, int);
extern int func_001286C8(int, int, int *, unsigned char *);

int func_00128FD0(int port, int second_size, void *second, int first_size, void *first) {
    Pad2Command command;
    struct Pad2Packet { int sizes[2]; unsigned char buffer[0x28]; } packet;
    int result;
    result = 0x28;
    command.fields.command = 0xB;
    command.fields.request = 1;
    command.fields.input = 3;
    command.fields.output = 1;
    packet.sizes[1] = first_size;
    func_001153D4(packet.buffer, first, first_size);
    packet.sizes[0] = second_size;
    first_size += (int)&packet;
    func_001153D4((void *)(first_size + 8), second, second_size);
    return func_001286C8(port, command.word, &result, (unsigned char *)&packet);
}
