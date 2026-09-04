#include <assert.h>
#include <stddef.h>

#include "item_effect.h"

static int has_item(const Map *map, int position, Item_Type item)
{
    return block_has_flag(map_get_block(map, (size_t)position), (BlockBits)item);
}

static Player normal_player_at(int position)
{
    Player player = {0};
    player.position = position;
    player.status = PLAYER_NORMAL;
    player.active = 1;
    return player;
}

static void test_plain_movement(void)
{
    Map map;
    Player player = normal_player_at(68);
    ItemEffectReport report;
    map_init(&map);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == 3);
    assert(report.requested_steps == 5);
    assert(report.travelled_steps == 5);
    assert(report.final_position == 3);
    assert(report.trigger_position == ITEM_EFFECT_NO_POSITION);
    assert(!report.stopped_by_barrier);
    assert(!report.skip_landing_event);
}

static void test_barrier_on_path_stops_and_clears(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map.blocks[13] |= IS_MINE;
    map_set_item(&map, 13, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 13 && report.travelled_steps == 3);
    assert(report.trigger_position == 13 && report.stopped_by_barrier);
    assert(!report.skip_landing_event);
    assert(!has_item(&map, 13, HAS_OBSTACLE));
    assert(map_block_is_mine(map_get_block(&map, 13)));
}

static void test_barrier_at_endpoint_also_triggers(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 16, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 16 && report.travelled_steps == 6);
    assert(!report.skip_landing_event && !has_item(&map, 16, HAS_OBSTACLE));
}

static void test_wraparound_trigger(void)
{
    Map map;
    Player player = normal_player_at(68);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 1, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 1 && report.travelled_steps == 3);
}

static void test_zero_step_is_a_valid_landing(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 10, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 0, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == 10 && report.travelled_steps == 0);
    assert(!report.skip_landing_event && has_item(&map, 10, HAS_OBSTACLE));
}

static void test_full_laps_do_not_scan_path_items(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 10, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, MAP_BLOCK_COUNT, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == 10 && report.travelled_steps == 0);
    assert(has_item(&map, 10, HAS_OBSTACLE));
}

static void test_invalid_input_does_not_modify_state(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 11, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(NULL, &map, 1, &report) ==
           ITEM_EFFECT_MOVE_INVALID_ARGUMENT);
    assert(Move_Player_With_Item_Effects(&player, NULL, 1, &report) ==
           ITEM_EFFECT_MOVE_INVALID_ARGUMENT);
    assert(Move_Player_With_Item_Effects(&player, &map, -1, &report) ==
           ITEM_EFFECT_MOVE_INVALID_STEP);
    assert(player.position == 10 && has_item(&map, 11, HAS_OBSTACLE));
    player.position = -1;
    assert(Move_Player_With_Item_Effects(&player, &map, 1, &report) ==
           ITEM_EFFECT_MOVE_INVALID_POSITION);
}

static void test_large_step_is_supported(void)
{
    Map map;
    Player player = normal_player_at(3);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 4, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, MAP_BLOCK_COUNT * 2, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == 3 && report.travelled_steps == 0);
    assert(has_item(&map, 4, HAS_OBSTACLE));
}

int main(void)
{
    test_plain_movement();
    test_barrier_on_path_stops_and_clears();
    test_barrier_at_endpoint_also_triggers();
    test_wraparound_trigger();
    test_zero_step_is_a_valid_landing();
    test_full_laps_do_not_scan_path_items();
    test_invalid_input_does_not_modify_state();
    test_large_step_is_supported();
    return 0;
}
