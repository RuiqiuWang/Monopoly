#include <assert.h>
#include <stddef.h>

#include "property.h"

static Player make_player(int id, int position, int money)
{
    Player player = {0};
    player.id = id;
    player.position = position;
    player.money = money;
    player.status = PLAYER_NORMAL;
    player.active = 1;
    return player;
}

static void test_buy_land(void)
{
    static const int positions[] = {1, 29, 36};
    static const int costs[] = {200, 500, 300};

    for (size_t i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i) {
        Map map;
        Player player;
        map_init(&map);
        player = make_player(1, positions[i], costs[i]);
        assert(try_buy_land(&map, &player));
        assert(buy_land(&map, &player) == 1);
        assert(player.money == 0);
        assert(map_get_property_owner(&map, (size_t)positions[i]) == 1);
        assert(map_get_property_level(&map, (size_t)positions[i]) == 0);

        assert(!try_buy_land(&map, &player));
        assert(buy_land(&map, &player) == 0);
    }

    {
        Map map;
        Player player;
        map_init(&map);
        player = make_player(1, 1, 199);
        assert(!try_buy_land(&map, &player));
        assert(buy_land(&map, &player) == 0 && player.money == 199);
        player.position = 0;
        assert(!try_buy_land(&map, &player));
        player.position = -1;
        assert(!try_buy_land(&map, &player));
        player.position = 1;
        player.active = 0;
        assert(!try_buy_land(&map, &player));
        player.active = 1;
        player.id = 0;
        assert(!try_buy_land(&map, &player));
        assert(!try_buy_land(NULL, &player));
        assert(!try_buy_land(&map, NULL));
    }
}

static void test_upgrade_property(void)
{
    Map map;
    Player player;

    map_init(&map);
    player = make_player(1, 1, 800);
    assert(map_set_property(&map, 1, 1, 0));
    for (unsigned int level = 1; level <= MAP_MAX_PROPERTY_LEVEL; ++level) {
        assert(try_upgrade_property(&map, &player));
        assert(upgrade_property(&map, &player) == 1);
        assert(map_get_property_level(&map, 1) == level);
    }
    assert(player.money == 200);
    assert(!try_upgrade_property(&map, &player));
    assert(upgrade_property(&map, &player) == 0);

    map_init(&map);
    player = make_player(1, 29, 499);
    assert(map_set_property(&map, 29, 1, 0));
    assert(!try_upgrade_property(&map, &player));
    player.money = 500;
    assert(upgrade_property(&map, &player) == 1 && player.money == 0);

    map_init(&map);
    player = make_player(1, 1, 1000);
    assert(map_set_property(&map, 1, 2, 0));
    assert(!try_upgrade_property(&map, &player));
    player.active = 0;
    assert(!try_upgrade_property(&map, &player));
    player.active = 1;
    player.id = 0;
    assert(!try_upgrade_property(&map, &player));
    assert(!try_upgrade_property(NULL, &player));
    assert(!try_upgrade_property(&map, NULL));
}

static void test_collect_toll(void)
{
    Map map;
    Player owner;
    Player visitor;

    for (unsigned int level = 0; level <= MAP_MAX_PROPERTY_LEVEL; ++level) {
        int toll = 200 * ((int)level + 1) / 2;
        map_init(&map);
        assert(map_set_property(&map, 1, 1, level));
        owner = make_player(1, 0, 1000);
        visitor = make_player(2, 1, 1000);
        assert(property_investment_value(&map, 1) == 200 * ((int)level + 1));
        assert(property_toll(&map, 1) == toll);
        assert(try_collect_toll(&map, &visitor, &owner));
        assert(collect_toll(&map, &visitor, &owner) == 1);
        assert(visitor.money == 1000 - toll);
        assert(owner.money == 1000 + toll);
    }

    map_init(&map);
    assert(map_set_property(&map, 1, 1, 0));
    owner = make_player(1, 0, 1000);
    visitor = make_player(2, 1, 99);
    assert(collect_toll(&map, &visitor, &owner) == 1);
    assert(visitor.money == -1 && owner.money == 1100);

    visitor = make_player(2, 1, 1000);
    visitor.god_of_wealth_rounds = 1;
    assert(!try_collect_toll(&map, &visitor, &owner));
    visitor.god_of_wealth_rounds = 0;
    owner.active = 0;
    assert(!try_collect_toll(&map, &visitor, &owner));
    owner.active = 1;
    owner.id = 3;
    assert(!try_collect_toll(&map, &visitor, &owner));
    owner.id = 1;
    visitor.id = 1;
    assert(!try_collect_toll(&map, &visitor, &owner));
    visitor.id = 0;
    assert(!try_collect_toll(&map, &visitor, &owner));
    assert(!try_collect_toll(NULL, &visitor, &owner));
    assert(!try_collect_toll(&map, NULL, &owner));
    assert(!try_collect_toll(&map, &visitor, NULL));
}

static void test_sell_property(void)
{
    Map map;
    Player player;

    for (unsigned int level = 0; level <= MAP_MAX_PROPERTY_LEVEL; ++level) {
        int sale = 200 * ((int)level + 1) * 2;
        map_init(&map);
        assert(map_set_property(&map, 1, 1, level));
        player = make_player(1, 0, 1000);
        assert(property_sale_price(&map, 1) == sale);
        assert(try_sell_property(&map, &player, 1));
        assert(sell_property(&map, &player, 1) == 1);
        assert(player.money == 1000 + sale);
        assert(map_get_property_owner(&map, 1) == MAP_PROPERTY_UNOWNED);
        assert(map_get_property_level(&map, 1) == 0);
        assert(sell_property(&map, &player, 1) == 0);
    }

    map_init(&map);
    player = make_player(1, 0, 1000);
    assert(map_set_property(&map, 1, 2, 0));
    assert(!try_sell_property(&map, &player, 1));
    assert(!try_sell_property(&map, &player, -1));
    assert(!try_sell_property(&map, &player, MAP_BLOCK_COUNT));
    assert(!try_sell_property(&map, &player, 0));
    player.active = 0;
    assert(!try_sell_property(&map, &player, 1));
    player.active = 1;
    player.id = 0;
    assert(!try_sell_property(&map, &player, 1));
    assert(!try_sell_property(NULL, &player, 1));
    assert(!try_sell_property(&map, NULL, 1));
    assert(property_investment_value(NULL, 1) == 0);
    assert(property_investment_value(&map, -1) == 0);
}

int main(void)
{
    test_buy_land();
    test_upgrade_property();
    test_collect_toll();
    test_sell_property();
    return 0;
}
