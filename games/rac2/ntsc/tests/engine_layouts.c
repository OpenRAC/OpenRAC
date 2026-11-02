/* Compile-time checks of the measured 32-bit retail memory views on any host. */
#include <stddef.h>
#include "engine.h"

#define CHECK(name, expression) typedef char check_##name[(expression) ? 1 : -1]
CHECK(address_width, sizeof(u32) == 4);
CHECK(impact_size, sizeof(Rac2Impact32) == 0x28);
CHECK(impact_vector, offsetof(Rac2Impact32, vector) == 0x00);
CHECK(impact_source, offsetof(Rac2Impact32, sourceAddress) == 0x10);
CHECK(impact_flags, offsetof(Rac2Impact32, flags) == 0x14);
CHECK(impact_metadata18, offsetof(Rac2Impact32, unk18) == 0x18);
CHECK(impact_metadata19, offsetof(Rac2Impact32, unk19) == 0x19);
CHECK(impact_class, offsetof(Rac2Impact32, objectClass) == 0x1A);
CHECK(impact_strength, offsetof(Rac2Impact32, strength) == 0x1C);
CHECK(impact_word20, offsetof(Rac2Impact32, unk20) == 0x20);
CHECK(impact_word24, offsetof(Rac2Impact32, unk24) == 0x24);
CHECK(moby_prefix_size, sizeof(Rac2MobyCollisionView32) == 0xAC);
CHECK(moby_position, offsetof(Rac2MobyCollisionView32, position) == 0x10);
CHECK(moby_instance_data, offsetof(Rac2MobyCollisionView32, instanceDataAddress) == 0x68);
CHECK(moby_impact_slot, offsetof(Rac2MobyCollisionView32, impactSlot) == 0xA8);
CHECK(moby_class, offsetof(Rac2MobyCollisionView32, objectClass) == 0xAA);
