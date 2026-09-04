#ifndef MONOPOLY_ITEM_EFFECT_H
#define MONOPOLY_ITEM_EFFECT_H

#include <stdbool.h>
#include "map.h"
#include "movement.h"
#include "player.h"

#define ITEM_EFFECT_NO_POSITION (-1)
typedef enum {
    ITEM_EFFECT_MOVE_OK = 0,
    ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER = 1,
    ITEM_EFFECT_MOVE_INVALID_ARGUMENT = -1,
    ITEM_EFFECT_MOVE_INVALID_POSITION = -2,
    ITEM_EFFECT_MOVE_INVALID_STEP = -3,
    ITEM_EFFECT_MOVE_FAILED = -4
} ItemEffectMoveResult;

typedef struct {
    int requested_steps;
    int travelled_steps;
    int final_position;
    int trigger_position;
    bool stopped_by_barrier;
    bool skip_landing_event;
} ItemEffectReport;

ItemEffectMoveResult Move_Player_With_Item_Effects(
    Player *player, Map *map, int step, ItemEffectReport *report);

#endif
