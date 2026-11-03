#ifndef STRUCTS_USAGE_STATS_H
#define STRUCTS_USAGE_STATS_H

#include "common.h"

/* How often a move or item was used. */
struct MoveUsage {
    u16 count;
    u16 best_minutes;
    int level_mask;
};

/* usage_stats: one record per move or item. */
struct MoveUsageTable {
    struct MoveUsage e[0x40];
};

typedef struct MoveUsage MoveUsage;
typedef struct MoveUsageTable MoveUsageTable;

#endif /* STRUCTS_USAGE_STATS_H */
