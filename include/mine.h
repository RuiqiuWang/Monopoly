#ifndef MONOPOLY_MINE_H
#define MONOPOLY_MINE_H

#include "map.h"
#include "player.h"

/* Apply the point reward for the block on which the player landed. */
void Check_Player_in_Mine(Player *player, const Map *map);

#endif
