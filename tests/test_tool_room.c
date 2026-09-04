#include <assert.h>
#include <stddef.h>

#include "tool_room.h"

int main(void)
{
    Player player = {0};
    player.points = 100;
    assert(Tool_Room_Buy(&player, TOOL_ROOM_BLOCK) == TOOL_ROOM_BUY_OK);
    assert(player.points == 50 && player.items[ITEM_BARRIER] == 1);
    assert(Tool_Room_Buy(&player, TOOL_ROOM_ROBOT) == TOOL_ROOM_BUY_OK);
    assert(player.points == 20 && player.items[ITEM_ROBOT] == 1);

    /* A second purchase must be rejected without overdrawing the balance. */
    player.points = 50;
    player.items[ITEM_BARRIER] = 0;
    player.items[ITEM_ROBOT] = 0;
    assert(Tool_Room_Buy(&player, TOOL_ROOM_ROBOT) == TOOL_ROOM_BUY_OK);
    assert(player.points == 20);
    assert(Tool_Room_Buy(&player, TOOL_ROOM_BLOCK) == TOOL_ROOM_BUY_NOT_ENOUGH_POINTS);
    assert(player.points == 20);

    player.items[ITEM_BARRIER] = 9;
    player.items[ITEM_ROBOT] = 1;
    assert(Tool_Room_Buy(&player, TOOL_ROOM_ROBOT) == TOOL_ROOM_BUY_ITEM_LIMIT_REACHED);
    assert(Tool_Room_Buy(NULL, TOOL_ROOM_BLOCK) == TOOL_ROOM_BUY_INVALID_PLAYER);
    return 0;
}
