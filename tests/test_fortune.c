#include <assert.h>
#include <stdlib.h>

#include "fortune.h"
#include "property.h"

static Player player_at(int id, int position)
{
    Player player = {0};
    player.id = id;
    player.position = position;
    player.money = 1000;
    player.status = PLAYER_NORMAL;
    player.active = 1;
    return player;
}

static void test_first_spawn_and_expiry(void)
{
    FortuneState fortune;
    Map map;
    Player players[2] = {player_at(1, 0), player_at(2, 1)};

    srand(1);
    map_init(&map);
    Fortune_Init(&fortune);
    for (int turn = 1; turn < 10; ++turn) {
        assert(Fortune_Advance_Turn(&fortune, &map, players, 2, turn) ==
               FORTUNE_TURN_NO_CHANGE);
    }
    assert(Fortune_Advance_Turn(&fortune, &map, players, 2, 10) ==
           FORTUNE_TURN_SPAWNED);
    assert(fortune.position >= 0 && fortune.position < MAP_BLOCK_COUNT);
    assert(fortune.position != players[0].position);
    assert(fortune.position != players[1].position);
    assert(!map_block_is_tool_room(map_get_block(&map, (size_t)fortune.position)));
    assert(!map_block_is_gift_room(map_get_block(&map, (size_t)fortune.position)));
    assert(block_has_flag(map_get_block(&map, (size_t)fortune.position), HAS_FORTUNE));
    assert(fortune.remaining_map_turns == 5);

    for (int turn = 11; turn < 15; ++turn) {
        assert(Fortune_Advance_Turn(&fortune, &map, players, 2, turn) ==
               FORTUNE_TURN_NO_CHANGE);
    }
    assert(Fortune_Advance_Turn(&fortune, &map, players, 2, 15) ==
           FORTUNE_TURN_EXPIRED);
    assert(fortune.position == FORTUNE_NO_POSITION);
    assert(fortune.next_spawn_after_turn >= 16);
    assert(fortune.next_spawn_after_turn <= 25);
}

static void test_collection_is_immediately_rent_free(void)
{
    FortuneState fortune = {10, 5, 0};
    Map map;
    Player owner = player_at(1, 0);
    Player visitor = player_at(2, 11);

    srand(2);
    map_init(&map);
    map_set_item(&map, 10, HAS_FORTUNE);
    assert(map_set_property(&map, 11, owner.id, 0));
    assert(Fortune_Collect_On_Path(&fortune, &map, &visitor, 8, 3, 11));
    assert(visitor.god_of_wealth_rounds == 5);
    assert(!try_collect_toll(&map, &visitor, &owner));
    assert(fortune.position == FORTUNE_NO_POSITION);
    assert(fortune.next_spawn_after_turn >= 12);
    assert(fortune.next_spawn_after_turn <= 21);

    Fortune_End_Player_Turn(&visitor, true);
    assert(visitor.god_of_wealth_rounds == 5);
    for (int turn = 0; turn < 5; ++turn) {
        Fortune_End_Player_Turn(&visitor, false);
    }
    assert(visitor.god_of_wealth_rounds == 0);
}

int main(void)
{
    test_first_spawn_and_expiry();
    test_collection_is_immediately_rent_free();
    return 0;
}
