#include <assert.h>
#include <stddef.h>

#include "gift_house.h"

int main(void)
{
    Player player = {0};
    player.money = 100;
    player.points = 50;
    player.status = PLAYER_HOSPITAL;
    player.status_rounds = 2;

    assert(Gift_House_Apply(&player, GIFT_HOUSE_MONEY) == GIFT_HOUSE_OK);
    assert(player.money == 2100);
    assert(Gift_House_Apply(&player, GIFT_HOUSE_POINTS) == GIFT_HOUSE_OK);
    assert(player.points == 250);
    assert(Gift_House_Apply(&player, GIFT_HOUSE_GOD_OF_WEALTH) == GIFT_HOUSE_OK);
    assert(player.god_of_wealth_rounds == 5);
    assert(player.status == PLAYER_HOSPITAL && player.status_rounds == 2);
    assert(Gift_House_Apply(&player, (GiftHouseChoice)0) ==
           GIFT_HOUSE_INVALID_CHOICE);
    assert(Gift_House_Apply(NULL, GIFT_HOUSE_MONEY) ==
           GIFT_HOUSE_INVALID_ARGUMENT);
    return 0;
}
