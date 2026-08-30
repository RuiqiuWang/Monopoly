#ifndef MONOPOLY_MAP_H
#define MONOPOLY_MAP_H

#include <stdbool.h>
#include <stddef.h>

#include "block_bit_utils.h"
#include "movement.h"

typedef enum {
    IS_START       = (1u << 0),
    IS_TOOL_ROOM   = (1u << 1),
    IS_GIFT_ROOM   = (1u << 2),
    IS_MAGIC_ROOM  = (1u << 3),
    IS_MINE        = (1u << 4),
    IS_PURCHASABLE = (1u << 5),
    IS_HOSPITAL    = (1u << 12),
    IS_JAIL        = (1u << 13)
} Block_Type;

typedef enum {
    IS_POOR      = (0u << 6) | (0u << 7),
    IS_MIDDLE    = (1u << 6) | (0u << 7),
    IS_RICH      = (0u << 6) | (1u << 7),
    IS_VERY_RICH = (1u << 6) | (1u << 7)
} Building_Type;

typedef enum {
    IS_PLOT_ONE   = (0u << 8) | (0u << 9),
    IS_PLOT_TWO   = (1u << 8) | (0u << 9),
    IS_PLOT_THREE = (0u << 8) | (1u << 9),
    PLOT_RESERVED = (1u << 8) | (1u << 9)
} Plot_Type;

typedef enum {
    NO_ITEM       = (0u << 10) | (0u << 11),
    HAS_BOMB      = (1u << 10) | (0u << 11),
    HAS_OBSTACLE  = (0u << 10) | (1u << 11),
    ITEM_RESERVED = (1u << 10) | (1u << 11)
} Item_Type;

typedef struct {
    BlockBits blocks[MAP_BLOCK_COUNT];
    double cost[MAP_BLOCK_COUNT];
    int property_owner[MAP_BLOCK_COUNT];
    unsigned char property_level[MAP_BLOCK_COUNT];
} Map;

#define MAP_PROPERTY_UNOWNED 0
#define MAP_MAX_PROPERTY_LEVEL 3

void map_init(Map *map);
bool map_valid_index(size_t index);
BlockBits map_get_block(const Map *map, size_t index);
double map_get_cost(const Map *map, size_t index);
void map_set_item(Map *map, size_t index, Item_Type item);
int map_get_property_owner(const Map *map, size_t index);
unsigned int map_get_property_level(const Map *map, size_t index);
bool map_set_property(Map *map, size_t index, int owner_id, unsigned int level);
bool map_clear_property(Map *map, size_t index);

bool map_block_is_start(BlockBits block);
bool map_block_is_tool_room(BlockBits block);
bool map_block_is_gift_room(BlockBits block);
bool map_block_is_magic_room(BlockBits block);
bool map_block_is_mine(BlockBits block);
bool map_block_is_hospital(BlockBits block);
bool map_block_is_jail(BlockBits block);
bool map_block_is_purchasable(BlockBits block);
bool map_block_is_plot(BlockBits block, Plot_Type plot);

#endif
