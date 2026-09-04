#include "gift_house.h"

#include <stdio.h>
#include "input.h"

#define GIFT_HOUSE_MONEY_REWARD 2000
#define GIFT_HOUSE_POINTS_REWARD 200
#define GIFT_HOUSE_GOD_ROUNDS 5

GiftHouseResult Gift_House_Apply(Player *player, GiftHouseChoice choice)
{
    if (player == NULL) return GIFT_HOUSE_INVALID_ARGUMENT;
    switch (choice) {
    case GIFT_HOUSE_MONEY:
        player->money += GIFT_HOUSE_MONEY_REWARD;
        return GIFT_HOUSE_OK;
    case GIFT_HOUSE_POINTS:
        player->points += GIFT_HOUSE_POINTS_REWARD;
        return GIFT_HOUSE_OK;
    case GIFT_HOUSE_GOD_OF_WEALTH:
        player->god_of_wealth_rounds = GIFT_HOUSE_GOD_ROUNDS;
        return GIFT_HOUSE_OK;
    default:
        return GIFT_HOUSE_INVALID_CHOICE;
    }
}

bool Gift_House_Prompt_With_Choice(
    Player *player, InputRefreshCallback refresh, void *context,
    GiftHouseChoice *applied_choice)
{
    char input[32];
    if (player == NULL) return false;
    puts("你已到达礼品屋，只能选择一件礼品。输入编号后按回车领取：");
    puts("  1：奖金——立即获得2000资金");
    puts("  2：点数——立即获得200点数");
    puts("  3：财神——接下来5回合免交租金");
    puts("输入 1、2 或 3 领取礼品；输入错误会放弃本次机会。\n"
         "礼品屋：1=奖金（2000），2=点数（200），3=财神（5回合）\n"
         "(Gift house: 1=money(2000), 2=points(200), 3=God of Wealth(5 turns))");
    if (!input_read_line("礼品屋> ", input, sizeof(input))) return false;
    if (input[1] != '\0' || input[0] < '1' || input[0] > '3') {
        if (refresh != NULL) refresh("输入无效（Invalid Input）：请输入1、2或3。", context);
        else {
            input_clear_screen();
            puts("输入无效（Invalid Input）：请输入1、2或3。");
        }
        puts("礼品选择无效，本次机会已放弃。（Invalid gift. This opportunity was skipped.）");
        return false;
    }
    GiftHouseChoice choice = (GiftHouseChoice)(input[0] - '0');
    bool applied = Gift_House_Apply(player, choice) == GIFT_HOUSE_OK;
    if (applied && applied_choice != NULL) *applied_choice = choice;
    return applied;
}

bool Gift_House_Prompt_With_Refresh(
    Player *player, InputRefreshCallback refresh, void *context)
{
    return Gift_House_Prompt_With_Choice(player, refresh, context, NULL);
}

bool Gift_House_Prompt(Player *player)
{
    return Gift_House_Prompt_With_Refresh(player, NULL, NULL);
}
