#include "tool_room.h"
#include "input.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TOOL_ROOM_ITEM_LIMIT 10

static int item_price(ToolRoomItem item)
{
    switch (item) {
    case TOOL_ROOM_BLOCK: return 50;
    case TOOL_ROOM_BOMB: return 30;
    case TOOL_ROOM_ROBOT: return 50;
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

void Enter_Tool_Room_With_Refresh(
    Player *player, InputRefreshCallback refresh, void *context)
{
    char input[32];
    char detail[128];
    const char *message = NULL;
    bool first_frame = true;
    bool purchased_any = false;
    if (player == NULL) return;
    for (;;) {
        ToolRoomItem item;
        ToolRoomBuyResult result;
        if (!first_frame) {
            /* The previous result belongs below the input that produced it;
             * redraw a clean frame before opening the next prompt. */
            if (refresh != NULL) refresh(NULL, context);
            else {
                input_clear_screen();
            }
        }
        first_frame = false;
        message = NULL;
        if (item_count(player) >= TOOL_ROOM_ITEM_LIMIT) {
            puts("道具屋已关闭：道具数量已达到上限。");
            return;
        }
        if (player->points < 30 && !purchased_any) {
            puts("道具屋已关闭：点数不足以购买任何道具。");
            return;
        }
        puts("你已到达道具屋，可以用点数购买道具。每次输入一个编号后按回车确认：");
        puts("  1：路障（50点）——阻挡其他玩家移动一次");
        puts("  2：炸弹（30点）——触发后将玩家送往医院");
        puts("  3：机器娃娃（50点）——清除前方10格内的路障和炸弹");
        puts("输入 1、2 或 3 购买；输入 F 后按回车离开道具屋。\n"
             "(Tool room: 1=barrier(50), 2=bomb(30), 3=robot(50), F=exit)");
        if (!input_read_line("道具屋> ", input, sizeof(input))) return;
        if ((input[0] == 'f' || input[0] == 'F') && input[1] == '\0') {
            puts("已退出道具屋。");
            return;
        }
        if (input[1] != '\0' || input[0] < '1' || input[0] > '3') {
            message = "输入无效（Invalid Input）：请输入1、2、3或F。";
            puts(message);
            continue;
        }
        item = (ToolRoomItem)(input[0] - '1');
        result = Tool_Room_Buy(player, item);
        if (result == TOOL_ROOM_BUY_OK) {
            purchased_any = true;
            message = "道具购买成功。（Item purchased.）";
        } else if (result == TOOL_ROOM_BUY_NOT_ENOUGH_POINTS) {
            snprintf(detail, sizeof(detail),
                     "操作失败：点数不足，购买该道具需要%d点，你当前有%d点。",
                     item_price(item), player->points);
            message = detail;
        }
        else message = "输入无效（Invalid Input）：无法购买该道具。";
        puts(message);
        if (result == TOOL_ROOM_BUY_OK && item_count(player) >= TOOL_ROOM_ITEM_LIMIT) {
            puts("道具屋已关闭：道具数量已达到上限。");
            return;
        }
    }
}

void Enter_Tool_Room(Player *player)
{
    Enter_Tool_Room_With_Refresh(player, NULL, NULL);
}
