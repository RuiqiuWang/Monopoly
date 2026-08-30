#include "mine.h"

#include <stddef.h>

static const int kMinePoints[] = {60, 80, 40, 100, 80, 20};

void Check_Player_in_Mine(Player *player, const Map *map)
{
    size_t position;

    if (player == NULL || map == NULL || !player->active || player->position < 0) {
        return;
    }
    position = (size_t)player->position;
    if (!map_valid_index(position) || !map_block_is_mine(map_get_block(map, position))) {
        return;
    }
    /* The map has six contiguous mine cells at the end of the board. */
    if (position < MAP_BLOCK_COUNT - (sizeof(kMinePoints) / sizeof(kMinePoints[0]))) {
        return;
    }
    player->points += kMinePoints[position -
        (MAP_BLOCK_COUNT - (sizeof(kMinePoints) / sizeof(kMinePoints[0])))];
}
