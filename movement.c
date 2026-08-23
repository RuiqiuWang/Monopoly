#include "movement.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

PlayerMoveResult Move_Player(Player *player, int step)
{
    int normalized_step;

    if (player == NULL) {
        return PLAYER_MOVE_INVALID_ARGUMENT;
    }
    if (player->position >= MAP_BLOCK_COUNT) {
        return PLAYER_MOVE_INVALID_POSITION;
    }
    if (step <= 0) {
        return PLAYER_MOVE_INVALID_STEP;
    }

    /* Reduce first so a very large step cannot overflow position arithmetic. */
    normalized_step = step % MAP_BLOCK_COUNT;
    player->position = (player->position + normalized_step) % MAP_BLOCK_COUNT;

    return PLAYER_MOVE_OK;
}

StepParseResult Parse_Step(const char *input, int *step)
{
    uint64_t value = 0;
    uint64_t digit;
    const unsigned char *cursor;

    if (input == NULL || step == NULL) {
        return STEP_PARSE_INVALID_ARGUMENT;
    }
    if (*input == '\0') {
        return STEP_PARSE_INVALID_STEP;
    }

    /* Deliberately do not accept whitespace, signs, decimal points, or suffixes. */
    for (cursor = (const unsigned char *)input; *cursor != '\0'; ++cursor) {
        if (*cursor < (unsigned char)'0' || *cursor > (unsigned char)'9') {
            return STEP_PARSE_INVALID_CHARACTER;
        }
        digit = (uint64_t)(*cursor - (unsigned char)'0');
        if (value > ((uint64_t)INT_MAX - digit) / 10U) {
            return STEP_PARSE_OVERFLOW;
        }
        value = value * 10U + digit;
    }

    if (value == 0) {
        return STEP_PARSE_INVALID_STEP;
    }

    *step = (int)value;
    return STEP_PARSE_OK;
}
