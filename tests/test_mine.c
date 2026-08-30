#include <assert.h>

#include "mine.h"

int main(void)
{
    Map map;
    Player player = {0};
    const int expected[] = {60, 80, 40, 100, 80, 20};

    map_init(&map);
    player.active = 1;
    for (int i = 0; i < 6; ++i) {
        player.position = 64 + i;
        player.points = 0;
        Check_Player_in_Mine(&player, &map);
        assert(player.points == expected[i]);
    }
    player.position = 10;
    player.points = 0;
    Check_Player_in_Mine(&player, &map);
    assert(player.points == 0);

    /* Bankrupt/inactive players earn no mine points. */
    player.active = 0;
    player.points = 0;
    player.position = 64;
    Check_Player_in_Mine(&player, &map);
    assert(player.points == 0);

    Check_Player_in_Mine(NULL, &map);
    return 0;
}
