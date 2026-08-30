#ifndef MONOPOLY_ITEM_EFFECT_H
#define MONOPOLY_ITEM_EFFECT_H

#include <stdbool.h>
#include "map.h"
#include "movement.h"
#include "player.h"

#define ITEM_EFFECT_NO_POSITION (-1)
#define ITEM_EFFECT_HOSPITAL_ROUNDS 3

typedef enum {
    ITEM_EFFECT_MOVE_OK = 0,
    ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER = 1,
    ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL = 2,
    ITEM_EFFECT_MOVE_INVALID_ARGUMENT = -1,
    ITEM_EFFECT_MOVE_INVALID_POSITION = -2,
    ITEM_EFFECT_MOVE_INVALID_STEP = -3,
    ITEM_EFFECT_MOVE_HOSPITAL_NOT_FOUND = -4,
    ITEM_EFFECT_MOVE_FAILED = -5
} ItemEffectMoveResult;

typedef struct {
    int requested_steps;
    int travelled_steps;
    int final_position;
    int trigger_position;
    bool stopped_by_barrier;
    bool hit_bomb;
    bool skip_landing_event;
} ItemEffectReport;

typedef enum {
    ITEM_EFFECT_TURN_READY = 0,
    ITEM_EFFECT_TURN_SKIPPED = 1,
    ITEM_EFFECT_TURN_INVALID_ARGUMENT = -1
} ItemEffectTurnResult;

ItemEffectMoveResult Move_Player_With_Item_Effects(
    Player *player, Map *map, int step, ItemEffectReport *report);
ItemEffectTurnResult Process_Hospital_Turn(Player *player);

#endif
