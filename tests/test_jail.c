#include <assert.h>
#include <stddef.h>

#include "jail.h"
#include "movement.h"

static Player normal_player_at(int position)
{
    Player player = {0};
    player.id = 1;
    player.position = position;
    player.status = PLAYER_NORMAL;
    player.active = 1;
    return player;
}

static int find_jail(const Map *map)
{
    for (int position = 0; position < MAP_BLOCK_COUNT; ++position) {
        if (map_block_is_jail(map_get_block(map, (size_t)position))) return position;
    }
    return -1;
}

int main(void)
{
    Map map;
    Player player;
    Player hospital;
    int jail_position;

    map_init(&map);
    jail_position = find_jail(&map);
    assert(jail_position >= 1);

    player = normal_player_at(jail_position - 1);
    assert(Move_Player(&player, 1) == PLAYER_MOVE_OK);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_ENTERED);
    assert(player.status == PLAYER_JAIL && player.status_rounds == 2);

    assert(Process_Jail_Turn(&player) == JAIL_TURN_SKIPPED);
    assert(player.status == PLAYER_JAIL && player.status_rounds == 1);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_ALREADY_DETAINED);
    assert(player.status_rounds == 1);
    assert(Process_Jail_Turn(&player) == JAIL_TURN_SKIPPED);
    assert(player.status == PLAYER_NORMAL && player.status_rounds == 0);
    assert(Process_Jail_Turn(&player) == JAIL_TURN_READY);

    player = normal_player_at(jail_position - 1);
    assert(Move_Player(&player, 2) == PLAYER_MOVE_OK);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_NOT_TRIGGERED);
    assert(player.status == PLAYER_NORMAL);

    player = normal_player_at(20);
    map.blocks[jail_position] &= (BlockBits)~IS_JAIL;
    map.blocks[20] |= IS_JAIL;
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_ENTERED);

    hospital = normal_player_at(14);
    hospital.status = PLAYER_HOSPITAL;
    hospital.status_rounds = 3;
    assert(Process_Jail_Turn(&hospital) == JAIL_TURN_READY);
    assert(hospital.status == PLAYER_HOSPITAL && hospital.status_rounds == 3);

    player = normal_player_at(-1);
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_INVALID_POSITION);
    player.position = MAP_BLOCK_COUNT;
    assert(Check_Player_in_Jail(&player, &map) == JAIL_CHECK_INVALID_POSITION);
    assert(Check_Player_in_Jail(NULL, &map) == JAIL_CHECK_INVALID_ARGUMENT);
    assert(Check_Player_in_Jail(&player, NULL) == JAIL_CHECK_INVALID_ARGUMENT);
    assert(Process_Jail_Turn(NULL) == JAIL_TURN_INVALID_ARGUMENT);
    return 0;
}
