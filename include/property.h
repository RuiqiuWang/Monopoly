#ifndef MONOPOLY_PROPERTY_H
#define MONOPOLY_PROPERTY_H

#include <stdbool.h>

#include "map.h"
#include "player.h"

bool try_buy_land(const Map *map, const Player *player);
int buy_land(Map *map, Player *player);

bool try_upgrade_property(const Map *map, const Player *player);
int upgrade_property(Map *map, Player *player);

/* owner is the resolved owner of the property at player's current position. */
bool try_collect_toll(const Map *map, const Player *player, const Player *owner);
int collect_toll(const Map *map, Player *player, Player *owner);

bool try_sell_property(const Map *map, const Player *player, int position);
int sell_property(Map *map, Player *player, int position);

int property_investment_value(const Map *map, int position);
int property_toll(const Map *map, int position);
int property_sale_price(const Map *map, int position);

#endif
