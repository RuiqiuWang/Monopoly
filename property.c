#include "property.h"

#include <stddef.h>

static bool player_position(const Map *map, const Player *player, size_t *position)
{
    if (map == NULL || player == NULL || position == NULL ||
        player->position < 0 || !map_valid_index((size_t)player->position)) {
        return false;
    }
    *position = (size_t)player->position;
    return true;
}

int property_investment_value(const Map *map, int position)
{
    unsigned int level;

    if (map == NULL || position < 0 || !map_valid_index((size_t)position) ||
        !map_block_is_purchasable(map_get_block(map, (size_t)position)) ||
        map_get_property_owner(map, (size_t)position) == MAP_PROPERTY_UNOWNED) {
        return 0;
    }
    level = map_get_property_level(map, (size_t)position);
    return (int)map_get_cost(map, (size_t)position) * ((int)level + 1);
}

int property_toll(const Map *map, int position)
{
    return property_investment_value(map, position) / 2;
}

int property_sale_price(const Map *map, int position)
{
    return property_investment_value(map, position) * 2;
}

bool try_buy_land(const Map *map, const Player *player)
{
    size_t position;

    if (!player_position(map, player, &position) || !player->active ||
        player->id <= MAP_PROPERTY_UNOWNED ||
        !map_block_is_purchasable(map_get_block(map, position)) ||
        map_get_property_owner(map, position) != MAP_PROPERTY_UNOWNED) {
        return false;
    }
    return player->money >= (int)map_get_cost(map, position);
}

int buy_land(Map *map, Player *player)
{
    size_t position;
    int price;

    if (!try_buy_land(map, player) ||
        !player_position(map, player, &position)) return 0;
    price = (int)map_get_cost(map, position);
    if (!map_set_property(map, position, player->id, 0)) return 0;
    player->money -= price;
    return 1;
}

bool try_upgrade_property(const Map *map, const Player *player)
{
    size_t position;

    if (!player_position(map, player, &position) || !player->active ||
        player->id <= MAP_PROPERTY_UNOWNED ||
        !map_block_is_purchasable(map_get_block(map, position)) ||
        map_get_property_owner(map, position) != player->id ||
        map_get_property_level(map, position) >= MAP_MAX_PROPERTY_LEVEL) {
        return false;
    }
    return player->money >= (int)map_get_cost(map, position);
}

int upgrade_property(Map *map, Player *player)
{
    size_t position;
    unsigned int level;
    int price;

    if (!try_upgrade_property(map, player) ||
        !player_position(map, player, &position)) return 0;
    level = map_get_property_level(map, position);
    price = (int)map_get_cost(map, position);
    if (!map_set_property(map, position, player->id, level + 1)) return 0;
    player->money -= price;
    return 1;
}

bool try_collect_toll(const Map *map, const Player *player, const Player *owner)
{
    size_t position;
    int owner_id;

    if (!player_position(map, player, &position) || owner == NULL ||
        !player->active || !owner->active ||
        player->id <= MAP_PROPERTY_UNOWNED ||
        owner->id <= MAP_PROPERTY_UNOWNED ||
        !map_block_is_purchasable(map_get_block(map, position))) {
        return false;
    }
    owner_id = map_get_property_owner(map, position);
    if (owner_id == MAP_PROPERTY_UNOWNED || owner_id == player->id ||
        owner_id != owner->id || player->god_of_wealth_rounds > 0 ||
        owner->status == PLAYER_HOSPITAL || owner->status == PLAYER_JAIL) {
        return false;
    }
    return property_toll(map, player->position) > 0;
}

int collect_toll(const Map *map, Player *player, Player *owner)
{
    int toll;

    if (!try_collect_toll(map, player, owner)) return 0;
    toll = property_toll(map, player->position);
    player->money -= toll;
    owner->money += toll;
    return 1;
}

bool try_sell_property(const Map *map, const Player *player, int position)
{
    if (map == NULL || player == NULL || !player->active || position < 0 ||
        player->id <= MAP_PROPERTY_UNOWNED ||
        !map_valid_index((size_t)position) ||
        !map_block_is_purchasable(map_get_block(map, (size_t)position))) {
        return false;
    }
    return map_get_property_owner(map, (size_t)position) == player->id;
}

int sell_property(Map *map, Player *player, int position)
{
    int price;

    if (!try_sell_property(map, player, position)) return 0;
    price = property_sale_price(map, position);
    if (price <= 0 || !map_clear_property(map, (size_t)position)) return 0;
    player->money += price;
    return 1;
}
