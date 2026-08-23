#ifndef MONOPOLY_MOVEMENT_H
#define MONOPOLY_MOVEMENT_H

#include "player.h"

#define MAP_BLOCK_COUNT 70

typedef enum {
    PLAYER_MOVE_OK = 0,
    PLAYER_MOVE_INVALID_ARGUMENT = -1,
    PLAYER_MOVE_INVALID_POSITION = -2,
    PLAYER_MOVE_INVALID_STEP = -3
} PlayerMoveResult;

typedef enum {
    STEP_PARSE_OK = 0,
    STEP_PARSE_INVALID_ARGUMENT = -1,
    STEP_PARSE_INVALID_CHARACTER = -2,
    STEP_PARSE_INVALID_STEP = -3,
    STEP_PARSE_OVERFLOW = -4
} StepParseResult;

/* Move a player forward and wrap around the circular 70-block map. */
PlayerMoveResult Move_Player(Player *player, int step);

/* Accept only a non-zero, decimal digit string that fits int. */
StepParseResult Parse_Step(const char *input, int *step);

#endif
