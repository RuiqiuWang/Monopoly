#include "map.h"

static const BlockBits kDefaultBlocks[MAP_BLOCK_COUNT] = {
    IS_START,

    /* top: S + 13 zero + H + 13 zero + T */
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_HOSPITAL,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_PURCHASABLE | IS_PLOT_ONE | IS_POOR,
    IS_TOOL_ROOM,

    /* right: 6 zero */
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,

    /* bottom: G + 13 zero + P + 13 zero + M */
    IS_GIFT_ROOM,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_JAIL,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_PURCHASABLE | IS_PLOT_THREE | IS_RICH,
    IS_MAGIC_ROOM,

    /* left: 6 mine */
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE
};

static const double kDefaultCosts[MAP_BLOCK_COUNT] = {
    0.0,

    /* top land */
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0,

    0.0,

    /* right land */
    500.0, 500.0, 500.0, 500.0, 500.0, 500.0,

    0.0,

    /* bottom land */
    300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0,
    300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0, 300.0,
    300.0, 300.0, 300.0, 300.0, 300.0, 300.0,

    0.0,
    0.0,
    0.0,

    /* mine */
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0
};

void map_init(Map *map)
{
    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
        map->blocks[i] = kDefaultBlocks[i];
        map->cost[i] = kDefaultCosts[i];
        map->property_owner[i] = MAP_PROPERTY_UNOWNED;
        map->property_level[i] = 0;
    }
}

bool map_valid_index(size_t index)
{
    return index < MAP_BLOCK_COUNT;
}

BlockBits map_get_block(const Map *map, size_t index)
{
    if (map == NULL || !map_valid_index(index)) {
        return 0;
    }
    return map->blocks[index];
}

double map_get_cost(const Map *map, size_t index)
{
    if (map == NULL || !map_valid_index(index)) {
        return 0.0;
    }
    return map->cost[index];
}

void map_set_item(Map *map, size_t index, Item_Type item)
{
    static const BlockBits item_mask = (BlockBits)((1u << 10) | (1u << 11));

    if (map == NULL || !map_valid_index(index)) {
        return;
    }

    map->blocks[index] &= (BlockBits)~item_mask;
    map->blocks[index] |= (BlockBits)item;
}

int map_get_property_owner(const Map *map, size_t index)
{
    if (map == NULL || !map_valid_index(index)) return MAP_PROPERTY_UNOWNED;
    return map->property_owner[index];
}

unsigned int map_get_property_level(const Map *map, size_t index)
{
    if (map == NULL || !map_valid_index(index)) return 0;
    return map->property_level[index];
}

bool map_set_property(Map *map, size_t index, int owner_id, unsigned int level)
{
    if (map == NULL || !map_valid_index(index) ||
        !map_block_is_purchasable(map->blocks[index]) ||
        owner_id <= MAP_PROPERTY_UNOWNED || level == 0 ||
        level > MAP_MAX_PROPERTY_LEVEL) {
        return false;
    }
    map->property_owner[index] = owner_id;
    map->property_level[index] = (unsigned char)level;
    return true;
}

bool map_clear_property(Map *map, size_t index)
{
    if (map == NULL || !map_valid_index(index) ||
        !map_block_is_purchasable(map->blocks[index])) return false;
    map->property_owner[index] = MAP_PROPERTY_UNOWNED;
    map->property_level[index] = 0;
    return true;
}

bool map_block_is_start(BlockBits block)
{
    return block_has_flag(block, IS_START);
}

bool map_block_is_tool_room(BlockBits block)
{
    return block_has_flag(block, IS_TOOL_ROOM);
}

bool map_block_is_gift_room(BlockBits block)
{
    return block_has_flag(block, IS_GIFT_ROOM);
}

bool map_block_is_magic_room(BlockBits block)
{
    return block_has_flag(block, IS_MAGIC_ROOM);
}

bool map_block_is_mine(BlockBits block)
{
    return block_has_flag(block, IS_MINE);
}

bool map_block_is_hospital(BlockBits block)
{
    return block_has_flag(block, IS_HOSPITAL);
}

bool map_block_is_jail(BlockBits block)
{
    return block_has_flag(block, IS_JAIL);
}

bool map_block_is_purchasable(BlockBits block)
{
    return block_has_flag(block, IS_PURCHASABLE);
}

bool map_block_is_plot(BlockBits block, Plot_Type plot)
{
    static const BlockBits plot_mask = (BlockBits)((1u << 8) | (1u << 9));

    return (block & plot_mask) == (BlockBits)plot;
}
