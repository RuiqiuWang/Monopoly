#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "assets.h"

int main(void)
{
    Player player = {0};
    Map map;
    FILE *file;
    player.id = 1;
    player.name[0] = 'A';
    player.money = 900;
    player.points = 20;
    player.active = 1;
    player.status = PLAYER_NORMAL;
    player.god_of_wealth_rounds = 4;
    map_init(&map);
    assert(query_assets_to_json(&player, &map, "assets_test.json"));
    file = fopen("assets_test.json", "r");
    assert(file != NULL);
    {
        char content[1024];
        size_t length = fread(content, 1, sizeof(content) - 1, file);
        content[length] = '\0';
        assert(strstr(content, "\"god_of_wealth_rounds\": 4") != NULL);
    }
    fclose(file);
    remove("assets_test.json");
    assert(!query_assets_to_json(NULL, &map, "assets_test.json"));
    return 0;
}
