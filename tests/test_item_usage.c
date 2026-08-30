#include <assert.h>

#include "item_usage.h"

static int has_item(const Map *map, int position, Item_Type item)
{
    return block_has_flag(map_get_block(map, (size_t)position), (BlockBits)item);
}

int main(void)
{
    Map map;
    Player player = {0};

    map_init(&map);
    player.position = 3;
    player.items[ITEM_BARRIER] = 2;
    player.items[ITEM_BOMB] = 1;
    player.items[ITEM_ROBOT] = 1;

    assert(Use_Block(&player, &map, -5) == ITEM_USE_OK);
    assert(has_item(&map, 68, HAS_OBSTACLE));
    assert(player.items[ITEM_BARRIER] == 1);
    assert(Use_Bomb(&player, &map, -5) == ITEM_USE_TARGET_OCCUPIED);
    assert(player.items[ITEM_BOMB] == 1);
    assert(Use_Bomb(&player, &map, 11) == ITEM_USE_DISTANCE_OUT_OF_RANGE);
    assert(Use_Bomb(&player, &map, 0) == ITEM_USE_OK);
    assert(has_item(&map, 3, HAS_BOMB));

    map_set_item(&map, 4, HAS_BOMB);
    map_set_item(&map, 13, HAS_OBSTACLE);
    map_set_item(&map, 14, HAS_BOMB);
    assert(Use_Robot(&player, &map) == ITEM_USE_OK);
    assert(!has_item(&map, 4, HAS_BOMB));
    assert(!has_item(&map, 13, HAS_OBSTACLE));
    assert(has_item(&map, 14, HAS_BOMB));
    assert(Use_Robot(&player, &map) == ITEM_USE_NOT_OWNED);
    assert(Use_Block(NULL, &map, 1) == ITEM_USE_INVALID_ARGUMENT);
    return 0;
}
