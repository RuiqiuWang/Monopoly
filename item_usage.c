#include "item_usage.h"

#define ITEM_USAGE_MIN_DISTANCE (-10)
#define ITEM_USAGE_MAX_DISTANCE 10
#define ROBOT_SCAN_DISTANCE 10

static int valid_player_position(const Player *player)
{
    return player->position >= 0 && player->position < MAP_BLOCK_COUNT;
}

static int target_position(int position, int distance)
{
    int target = (position + distance) % MAP_BLOCK_COUNT;
    return target < 0 ? target + MAP_BLOCK_COUNT : target;
}

static int block_has_item(BlockBits block)
{
    return block_has_any_flag(block, (BlockBits)(HAS_BOMB | HAS_OBSTACLE));
}

static ItemUseResult place_item(Player *player, Map *map, int distance,
                                int inventory, Item_Type item)
{
    int target;
    if (player == NULL || map == NULL) return ITEM_USE_INVALID_ARGUMENT;
    if (!valid_player_position(player)) return ITEM_USE_INVALID_POSITION;
    if (distance < ITEM_USAGE_MIN_DISTANCE || distance > ITEM_USAGE_MAX_DISTANCE) {
        return ITEM_USE_DISTANCE_OUT_OF_RANGE;
    }
    if (player->items[inventory] <= 0) return ITEM_USE_NOT_OWNED;
    target = target_position(player->position, distance);
    if (block_has_item(map_get_block(map, (size_t)target))) {
        return ITEM_USE_TARGET_OCCUPIED;
    }
    map_set_item(map, (size_t)target, item);
    --player->items[inventory];
    return ITEM_USE_OK;
}

ItemUseResult Use_Block(Player *player, Map *map, int relative_distance)
{
    return place_item(player, map, relative_distance, ITEM_BARRIER, HAS_OBSTACLE);
}

ItemUseResult Use_Bomb(Player *player, Map *map, int relative_distance)
{
    return place_item(player, map, relative_distance, ITEM_BOMB, HAS_BOMB);
}

ItemUseResult Use_Robot(Player *player, Map *map)
{
    int distance;
    if (player == NULL || map == NULL) return ITEM_USE_INVALID_ARGUMENT;
    if (!valid_player_position(player)) return ITEM_USE_INVALID_POSITION;
    if (player->items[ITEM_ROBOT] <= 0) return ITEM_USE_NOT_OWNED;
    for (distance = 1; distance <= ROBOT_SCAN_DISTANCE; ++distance) {
        map_set_item(map, (size_t)target_position(player->position, distance), NO_ITEM);
    }
    --player->items[ITEM_ROBOT];
    return ITEM_USE_OK;
}
