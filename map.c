#include "map.h"

static const BlockBits kDefaultBlocks[MAP_BLOCK_COUNT] = {
    IS_START,

    /* 1-26: 地段 1 */
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

    /* 28-33: 地段 2 */
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,
    IS_PURCHASABLE | IS_PLOT_TWO | IS_MIDDLE,

    IS_GIFT_ROOM,

    /* 35-60: 地段 3 */
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
    IS_HOSPITAL,
    IS_JAIL,

    /* 64-69: 矿地 */
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE,
    IS_MINE
};

static const double kDefaultCosts[MAP_BLOCK_COUNT] = {
    0.0,

    /* 地段 1 */
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0, 200.0,
    200.0, 200.0, 200.0, 200.0, 200.0, 200.0,

    0.0,

    /* 地段 2 */
    500.0, 500.0, 500.0, 500.0, 500.0, 500.0,

    0.0,

    /* 地段 3 */
    900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0,
    900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0, 900.0,
    900.0, 900.0, 900.0, 900.0, 900.0, 900.0,

    0.0,
    0.0,
    0.0,

    /* 矿地 */
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
