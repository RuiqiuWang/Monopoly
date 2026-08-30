#include "tool_room.h"

#include <stdio.h>
#include <string.h>

#define TOOL_ROOM_ITEM_LIMIT 10

static int item_price(ToolRoomItem item)
{
    switch (item) {
    case TOOL_ROOM_BLOCK: return 50;
    case TOOL_ROOM_ROBOT: return 30;
    case TOOL_ROOM_BOMB: return 50;
    default: return -1;
    }
}

static int item_count(const Player *player)
{
    return player->items[ITEM_BARRIER] + player->items[ITEM_ROBOT] +
        player->items[ITEM_BOMB];
}

ToolRoomBuyResult Tool_Room_Buy(Player *player, ToolRoomItem item)
{
    int price;
    if (player == NULL) return TOOL_ROOM_BUY_INVALID_PLAYER;
    price = item_price(item);
    if (price < 0) return TOOL_ROOM_BUY_INVALID_ITEM;
    if (item_count(player) >= TOOL_ROOM_ITEM_LIMIT) {
        return TOOL_ROOM_BUY_ITEM_LIMIT_REACHED;
    }
    if (player->points < price) return TOOL_ROOM_BUY_NOT_ENOUGH_POINTS;
    player->points -= price;
    player->items[item] += 1;
    return TOOL_ROOM_BUY_OK;
}

void Enter_Tool_Room(Player *player)
{
    char input[32];
    if (player == NULL) return;
    puts("Tool room: 1=barrier(50), 2=robot(30), 3=bomb(50), F=exit");
    for (;;) {
        ToolRoomItem item;
        ToolRoomBuyResult result;
        if (item_count(player) >= TOOL_ROOM_ITEM_LIMIT || player->points < 30) {
            puts("Tool room closed.");
            return;
        }
        fputs("Tool room> ", stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) return;
        input[strcspn(input, "\r\n")] = '\0';
        if ((input[0] == 'f' || input[0] == 'F') && input[1] == '\0') return;
        if (input[1] != '\0' || input[0] < '1' || input[0] > '3') {
            puts("Invalid item. Choose 1, 2, 3, or F.");
            continue;
        }
        item = (ToolRoomItem)(input[0] - '1');
        result = Tool_Room_Buy(player, item);
        if (result == TOOL_ROOM_BUY_OK) puts("Item purchased.");
        else if (result == TOOL_ROOM_BUY_NOT_ENOUGH_POINTS) puts("Not enough points.");
    }
}
