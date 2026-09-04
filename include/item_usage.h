#ifndef MONOPOLY_ITEM_USAGE_H
#define MONOPOLY_ITEM_USAGE_H

#include "map.h"
#include "player.h"

typedef enum {
    ITEM_USE_OK = 0,
    ITEM_USE_INVALID_ARGUMENT = -1,
    ITEM_USE_INVALID_POSITION = -2,
    ITEM_USE_DISTANCE_OUT_OF_RANGE = -3,
    ITEM_USE_NOT_OWNED = -4,
    ITEM_USE_TARGET_OCCUPIED = -5
} ItemUseResult;

ItemUseResult Use_Block(Player *player, Map *map, int relative_distance);
ItemUseResult Use_Robot(Player *player, Map *map);

#endif
