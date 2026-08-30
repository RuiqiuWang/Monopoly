#ifndef MONOPOLY_GIFT_HOUSE_H
#define MONOPOLY_GIFT_HOUSE_H

#include <stdbool.h>
#include "input.h"
#include "player.h"

typedef enum {
    GIFT_HOUSE_MONEY = 1,
    GIFT_HOUSE_POINTS = 2,
    GIFT_HOUSE_GOD_OF_WEALTH = 3
} GiftHouseChoice;

typedef enum {
    GIFT_HOUSE_OK = 0,
    GIFT_HOUSE_INVALID_ARGUMENT = -1,
    GIFT_HOUSE_INVALID_CHOICE = -2
} GiftHouseResult;

GiftHouseResult Gift_House_Apply(Player *player, GiftHouseChoice choice);
bool Gift_House_Prompt(Player *player);
bool Gift_House_Prompt_With_Refresh(
    Player *player, InputRefreshCallback refresh, void *context);

#endif
