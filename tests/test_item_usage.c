#include <assert.h>
#include <stddef.h>

#include "item_usage.h"

static int has_item(const Map *map, int position, Item_Type item)
{
    return block_has_flag(map_get_block(map, (size_t)position), (BlockBits)item);
}

static void test_block_placement(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 5;
    player.items[ITEM_BARRIER] = 1;
    assert(Use_Block(&player, &map, 10) == ITEM_USE_OK);
    assert(has_item(&map, 15, HAS_OBSTACLE));
    assert(player.items[ITEM_BARRIER] == 0);
}

static void test_negative_distance_wraps(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 3;
    player.items[ITEM_BARRIER] = 1;
    assert(Use_Block(&player, &map, -5) == ITEM_USE_OK);
    assert(has_item(&map, 68, HAS_OBSTACLE));
    assert(player.items[ITEM_BARRIER] == 0);
}

static void test_occupied_target_is_rejected(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 20;
    player.items[ITEM_BARRIER] = 1;
    map_set_item(&map, 25, HAS_FORTUNE);
    assert(Use_Block(&player, &map, 5) == ITEM_USE_TARGET_OCCUPIED);
    assert(has_item(&map, 25, HAS_FORTUNE));
    assert(player.items[ITEM_BARRIER] == 1);
}

static void test_invalid_distance_does_not_consume_item(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 20;
    player.items[ITEM_BARRIER] = 2;
    assert(Use_Block(&player, &map, -11) == ITEM_USE_DISTANCE_OUT_OF_RANGE);
    assert(Use_Block(&player, &map, 11) == ITEM_USE_DISTANCE_OUT_OF_RANGE);
    assert(player.items[ITEM_BARRIER] == 2);
}

static void test_missing_item_is_rejected(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 20;
    assert(Use_Block(&player, &map, 1) == ITEM_USE_NOT_OWNED);
    assert(Use_Robot(&player, &map) == ITEM_USE_NOT_OWNED);
}

static void test_invalid_arguments_and_position(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.items[ITEM_BARRIER] = 1;
    player.items[ITEM_ROBOT] = 1;
    assert(Use_Block(NULL, &map, 1) == ITEM_USE_INVALID_ARGUMENT);
    assert(Use_Block(&player, NULL, 1) == ITEM_USE_INVALID_ARGUMENT);
    assert(Use_Robot(NULL, &map) == ITEM_USE_INVALID_ARGUMENT);
    assert(Use_Robot(&player, NULL) == ITEM_USE_INVALID_ARGUMENT);
    player.position = -1;
    assert(Use_Block(&player, &map, 1) == ITEM_USE_INVALID_POSITION);
    assert(Use_Robot(&player, &map) == ITEM_USE_INVALID_POSITION);
    player.position = MAP_BLOCK_COUNT;
    assert(Use_Block(&player, &map, 1) == ITEM_USE_INVALID_POSITION);
    assert(Use_Robot(&player, &map) == ITEM_USE_INVALID_POSITION);
}

static void test_robot_clears_forward_ten_steps_only(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 68;
    player.items[ITEM_ROBOT] = 1;
    map_set_item(&map, 68, HAS_FORTUNE);
    map_set_item(&map, 69, HAS_OBSTACLE);
    map_set_item(&map, 0, HAS_FORTUNE);
    map_set_item(&map, 8, HAS_OBSTACLE);
    map_set_item(&map, 9, HAS_FORTUNE);
    assert(Use_Robot(&player, &map) == ITEM_USE_OK);
    assert(has_item(&map, 68, HAS_FORTUNE));
    assert(!has_item(&map, 69, HAS_OBSTACLE));
    assert(has_item(&map, 0, HAS_FORTUNE));
    assert(map_block_is_start(map_get_block(&map, 0)));
    assert(!has_item(&map, 8, HAS_OBSTACLE));
    assert(has_item(&map, 9, HAS_FORTUNE));
    assert(player.items[ITEM_ROBOT] == 0);
}

static void test_robot_is_consumed_when_nothing_is_cleared(void)
{
    Map map;
    Player player = {0};
    map_init(&map);
    player.position = 20;
    player.items[ITEM_ROBOT] = 1;
    assert(Use_Robot(&player, &map) == ITEM_USE_OK);
    assert(player.items[ITEM_ROBOT] == 0);
}

int main(void)
{
    test_block_placement();
    test_negative_distance_wraps();
    test_occupied_target_is_rejected();
    test_invalid_distance_does_not_consume_item();
    test_missing_item_is_rejected();
    test_invalid_arguments_and_position();
    test_robot_clears_forward_ten_steps_only();
    test_robot_is_consumed_when_nothing_is_cleared();
    return 0;
}
