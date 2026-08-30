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

bool Gift_House_Prompt(Player *player)
{
    char input[32];
    if (player == NULL) return false;
    puts("Gift house: 1=money(2000), 2=points(200), 3=God of Wealth(5 turns)");
    if (!input_read_line("Gift house> ", input, sizeof(input))) return false;
    if (input[1] != '\0' || input[0] < '1' || input[0] > '3') {
        puts("Invalid gift. This opportunity was skipped.");
        return false;
    }
    return Gift_House_Apply(player, (GiftHouseChoice)(input[0] - '0')) == GIFT_HOUSE_OK;
}
