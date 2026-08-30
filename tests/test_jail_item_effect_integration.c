#include <assert.h>

#include "item_effect.h"
#include "jail.h"

static Player normal_player_at(int position)
{
    Player player = {0};
    player.id = 1;
    player.position = position;
    player.status = PLAYER_NORMAL;
    player.active = 1;
    return player;
}

int main(void)
{
    Map map;
    Player player;
    ItemEffectReport report;

    map_init(&map);
    player = normal_player_at(47);
    map_set_item(&map, 49, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 49 && !report.skip_landing_event);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_ENTERED);

    map_init(&map);
    player = normal_player_at(47);
    map_set_item(&map, 49, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(report.skip_landing_event);
    assert(player.position == 14 && player.status == PLAYER_HOSPITAL);
    assert(player.status_rounds == ITEM_EFFECT_HOSPITAL_ROUNDS);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_NOT_TRIGGERED);
    return 0;
}
