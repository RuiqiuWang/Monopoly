#include "assets.h"

#include <stdio.h>

void query_assets(const Player *player, const Map *map)
{
    (void)map;
    if (player == NULL) return;
    printf("Player %s (id=%d)\n", player->name, player->id);
    printf("money=%d points=%d position=%d\n", player->money, player->points, player->position);
    printf("items: barrier=%d robot=%d bomb=%d\n",
           player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB]);
    printf("status=%s remaining_rounds=%d\n",
           status_to_string(player->status), player->status_rounds);
    puts("properties: not implemented by the current map model");
}

bool query_assets_to_json(const Player *player, const Map *map, const char *filename)
{
    FILE *file;
    (void)map;
    if (player == NULL || filename == NULL) return false;
    file = fopen(filename, "w");
    if (file == NULL) return false;
    fprintf(file,
            "{\n  \"id\": %d,\n  \"name\": \"%s\",\n"
            "  \"position\": %d,\n  \"money\": %d,\n  \"points\": %d,\n"
            "  \"items\": {\"barrier\": %d, \"robot\": %d, \"bomb\": %d},\n"
            "  \"status\": \"%s\",\n  \"status_rounds\": %d,\n  \"active\": %s\n}\n",
            player->id, player->name, player->position, player->money, player->points,
            player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB],
            status_to_string(player->status), player->status_rounds,
            player->active ? "true" : "false");
    if (ferror(file) != 0 || fclose(file) != 0) return false;
    return true;
}
