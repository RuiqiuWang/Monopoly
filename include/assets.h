#ifndef MONOPOLY_ASSETS_H
#define MONOPOLY_ASSETS_H

#include <stdbool.h>

#include "map.h"
#include "player.h"

void query_assets(const Player *player, const Map *map);
bool query_assets_to_json(const Player *player, const Map *map, const char *filename);

#endif
