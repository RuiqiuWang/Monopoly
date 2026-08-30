#ifndef MONOPOLY_JAIL_H
#define MONOPOLY_JAIL_H

#include "map.h"
#include "player.h"

#define JAIL_DETENTION_ROUNDS 2

typedef enum {
    JAIL_CHECK_NOT_TRIGGERED = 0,
    JAIL_CHECK_ENTERED = 1,
    JAIL_CHECK_ALREADY_DETAINED = 2,
    JAIL_CHECK_INVALID_ARGUMENT = -1,
    JAIL_CHECK_INVALID_POSITION = -2
} JailCheckResult;

typedef enum {
    JAIL_TURN_READY = 0,
    JAIL_TURN_SKIPPED = 1,
    JAIL_TURN_INVALID_ARGUMENT = -1
} JailTurnResult;

JailCheckResult Check_Player_in_Jail(Player *player, const Map *map);
JailTurnResult Process_Jail_Turn(Player *player);

#endif
