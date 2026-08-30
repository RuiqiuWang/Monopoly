#include <assert.h>

#include "item_effect.h"

static int has_item(const Map *map, int position, Item_Type item)
{
    return block_has_flag(map_get_block(map, (size_t)position), (BlockBits)item);
}

int main(void)
{
    Map map;
    Player player = {0};
    ItemEffectReport report;

    map_init(&map);
    player.position = 10;
    map_set_item(&map, 13, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 13 && report.travelled_steps == 3);
    assert(!has_item(&map, 13, HAS_OBSTACLE));

    map_set_item(&map, 15, HAS_BOMB);
    assert(Move_Player_With_Item_Effects(&player, &map, 6, &report) ==
           ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL);
    assert(player.position == 14);
    assert(player.status == PLAYER_HOSPITAL && player.status_rounds == 3);
    assert(report.skip_landing_event && !has_item(&map, 15, HAS_BOMB));
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_SKIPPED);
    assert(player.status == PLAYER_NORMAL && player.status_rounds == 0);
    assert(Process_Hospital_Turn(&player) == ITEM_EFFECT_TURN_READY);

    player.position = 69;
    map_set_item(&map, 1, HAS_OBSTACLE);
    assert(Move_Player_With_Item_Effects(&player, &map, 5, &report) ==
           ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER);
    assert(player.position == 1 && report.travelled_steps == 2);
    assert(Move_Player_With_Item_Effects(&player, &map, 0, &report) ==
           ITEM_EFFECT_MOVE_INVALID_STEP);
    return 0;
}
