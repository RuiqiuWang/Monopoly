#include <assert.h>
#include <limits.h>
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
    assert(!report.stopped_by_barrier && !report.hit_bomb);
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

static void test_bomb_on_path_sends_player_to_hospital(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map.blocks[12] |= IS_PURCHASABLE;
    map_set_item(&map, 12, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(player.position == 14 && player.status == PLAYER_HOSPITAL);
    assert(player.status_rounds == 3 && report.travelled_steps == 2);
    assert(report.trigger_position == 12 && report.final_position == 14);
    assert(report.hit_bomb && report.skip_landing_event);
    assert(!has_item(&map, 12, HAS_BOMB));
    assert(map_block_is_purchasable(map_get_block(&map, 12)));
}

static void test_bomb_at_endpoint_also_triggers(void)
{
    Map map;
    Player player = normal_player_at(20);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 26, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(report.travelled_steps == 6 && report.trigger_position == 26);
    assert(player.position == 14);
}

static void test_first_trigger_on_path_wins(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 12, HAS_OBSTACLE);
    map_set_item(&map, 14, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 8, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 12 && !has_item(&map, 12, HAS_OBSTACLE));
    assert(has_item(&map, 14, HAS_BOMB));
    player = normal_player_at(10);
    map_init(&map);
    map_set_item(&map, 12, HAS_BOMB);
    map_set_item(&map, 14, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 8, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(!has_item(&map, 12, HAS_BOMB));
    assert(has_item(&map, 14, HAS_OBSTACLE));
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

static void test_item_under_starting_player_is_not_immediate(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 10, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 1, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == 11 && has_item(&map, 10, HAS_BOMB));
}

static void test_starting_tile_triggers_after_full_lap(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 10, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, MAP_BLOCK_COUNT, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 10 && report.travelled_steps == MAP_BLOCK_COUNT);
    assert(report.trigger_position == 10 && !has_item(&map, 10, HAS_OBSTACLE));
}

static void test_hospital_item_does_not_chain(void)
{
    Map map;
    Player player = normal_player_at(5);
    ItemEffectReport report;
    map_init(&map);
    map_set_item(&map, 7, HAS_BOMB);
    map_set_item(&map, 14, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(player.position == 14 && !has_item(&map, 7, HAS_BOMB));
    assert(has_item(&map, 14, HAS_BOMB));
}

static void test_three_hospital_turns_are_skipped(void)
{
    Player player = normal_player_at(14);
    player.status = PLAYER_HOSPITAL;
    player.status_rounds = 3;
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(player.status_rounds == 2 && player.status == PLAYER_HOSPITAL);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(player.status_rounds == 1 && player.status == PLAYER_HOSPITAL);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(player.status_rounds == 0 && player.status == PLAYER_NORMAL);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_READY);
    assert(player.position == 14);
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
    assert(Move_Player_With_Item_Effects(&player, &map, 0, &report) ==
           ITEM_EFFECT_MOVE_INVALID_STEP);
    assert(player.position == 10 && has_item(&map, 11, HAS_OBSTACLE));
    player.position = -1;
    assert(Move_Player_With_Item_Effects(&player, &map, 1, &report) ==
           ITEM_EFFECT_MOVE_INVALID_POSITION);
    assert(Process_Hospital_Turn(NULL) == ITEM_EFFECT_TURN_INVALID_ARGUMENT);
}

static void test_missing_hospital_is_atomic(void)
{
    Map map;
    Player player = normal_player_at(10);
    ItemEffectReport report;
    map_init(&map);
    map.blocks[14] &= (BlockBits)~IS_HOSPITAL;
    map_set_item(&map, 12, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_HOSPITAL_NOT_FOUND);
    assert(player.position == 10 && player.status == PLAYER_NORMAL);
    assert(has_item(&map, 12, HAS_BOMB));
}

static void test_large_step_is_supported(void)
{
    Map map;
    Player player = normal_player_at(3);
    ItemEffectReport report;
    int expected = (3 + (INT_MAX % MAP_BLOCK_COUNT)) % MAP_BLOCK_COUNT;
    map_init(&map);
    assert(Move_Player_With_Item_Effects(&player, &map, INT_MAX, &report) ==
           ITEM_EFFECT_MOVE_OK);
    assert(player.position == expected && report.travelled_steps == INT_MAX);
}

int main(void)
{
    test_plain_movement();
    test_barrier_on_path_stops_and_clears();
    test_barrier_at_endpoint_also_triggers();
    test_bomb_on_path_sends_player_to_hospital();
    test_bomb_at_endpoint_also_triggers();
    test_first_trigger_on_path_wins();
    test_wraparound_trigger();
    test_item_under_starting_player_is_not_immediate();
    test_starting_tile_triggers_after_full_lap();
    test_hospital_item_does_not_chain();
    test_three_hospital_turns_are_skipped();
    test_invalid_input_does_not_modify_state();
    test_missing_hospital_is_atomic();
    test_large_step_is_supported();
    return 0;
}
