#ifndef GADGETS_H
#define GADGETS_H

#include "common.h"

/*
 * RaC1 Gadget and Weapon Enum (reused by RaC1 Save-Import system in RaC2).
 * Recovered from Aug 8 2002 proto string mining (@creepnt 2026).
 */
typedef enum RaC1GadgetId {
    GADGET_BOMB_GLOVE       = 0,
    GADGET_PYROCITOR        = 1,
    GADGET_BLASTER          = 2,
    GADGET_GLOVE_OF_DOOM    = 3,
    GADGET_SUCK_CANNON      = 4,
    GADGET_SWINGSHOT        = 5,
    GADGET_HYDRODISPLACER   = 6,
    GADGET_SONIC_SUMMONER   = 7,
    GADGET_RYNO             = 8,
    GADGET_WALLOPER         = 9,
    GADGET_VISIBOMB         = 10,
    GADGET_DECOY_GLOVE      = 11,
    GADGET_TESLA_CLAW       = 12,
    GADGET_TAUNTER          = 13,
    GADGET_TRESPASSER       = 14,
    GADGET_METAL_DETECTOR   = 15,
    GADGET_MAGNEBOOTS       = 16,
    GADGET_GRIND_BOOTS      = 17,
    GADGET_HOVERBOARD       = 18,
    GADGET_HELI_PACK        = 19,
    GADGET_THRUSTER_PACK    = 20,
    GADGET_HYDRO_PACK       = 21,
    GADGET_O2_MASK          = 22,
    GADGET_PILOTS_HELMET    = 23,
    GADGET_MORPH_O_RAY      = 24,
    GADGET_CODEBOT          = 25,
    GADGET_HOLOGUISE        = 26,
    GADGET_PDA              = 27,
    GADGET_PERSUADER        = 28
} RaC1GadgetId;

#endif /* GADGETS_H */
