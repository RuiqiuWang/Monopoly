#ifndef MONOPOLY_TOOL_ROOM_H
#define MONOPOLY_TOOL_ROOM_H

#include "player.h"
#include "input.h"

typedef enum {
    TOOL_ROOM_BLOCK = ITEM_BARRIER,
    TOOL_ROOM_BOMB = ITEM_BOMB,
    TOOL_ROOM_ROBOT = ITEM_ROBOT
} ToolRoomItem;

typedef enum {
    TOOL_ROOM_BUY_OK = 0,
    TOOL_ROOM_BUY_INVALID_PLAYER = -1,
    TOOL_ROOM_BUY_INVALID_ITEM = -2,
    TOOL_ROOM_BUY_NOT_ENOUGH_POINTS = -3,
    TOOL_ROOM_BUY_ITEM_LIMIT_REACHED = -4
} ToolRoomBuyResult;

ToolRoomBuyResult Tool_Room_Buy(Player *player, ToolRoomItem item);
void Enter_Tool_Room(Player *player);
void Enter_Tool_Room_With_Refresh(
    Player *player, InputRefreshCallback refresh, void *context);

#endif
