#include "jail.h"

#include <stddef.h>

JailCheckResult Check_Player_in_Jail(Player *player, const Map *map)
{
    BlockBits block;

    if (player == NULL || map == NULL) return JAIL_CHECK_INVALID_ARGUMENT;
    if (player->position < 0 || player->position >= MAP_BLOCK_COUNT) {
        return JAIL_CHECK_INVALID_POSITION;
    }

    block = map_get_block(map, (size_t)player->position);
    if (!map_block_is_jail(block)) return JAIL_CHECK_NOT_TRIGGERED;
    if (player->status == PLAYER_JAIL && player->status_rounds > 0) {
        return JAIL_CHECK_ALREADY_DETAINED;
    }

    player->status = PLAYER_JAIL;
    player->status_rounds = JAIL_DETENTION_ROUNDS;
    return JAIL_CHECK_ENTERED;
}

JailTurnResult Process_Jail_Turn(Player *player)
{
    if (player == NULL) return JAIL_TURN_INVALID_ARGUMENT;
    if (player->status != PLAYER_JAIL) return JAIL_TURN_READY;
    if (player->status_rounds <= 0) {
        player->status_rounds = 0;
        player->status = PLAYER_NORMAL;
        return JAIL_TURN_READY;
    }

    --player->status_rounds;
    if (player->status_rounds == 0) player->status = PLAYER_NORMAL;
    return JAIL_TURN_SKIPPED;
}
