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

bool Gift_House_Prompt_With_Refresh(
    Player *player, InputRefreshCallback refresh, void *context)
{
    char input[32];
    if (player == NULL) return false;
    puts("礼品屋：1=奖金（2000），2=点数（200），3=财神（5回合）\n"
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
    return Gift_House_Apply(player, (GiftHouseChoice)(input[0] - '0')) == GIFT_HOUSE_OK;
}

bool Gift_House_Prompt(Player *player)
{
    return Gift_House_Prompt_With_Refresh(player, NULL, NULL);
}
