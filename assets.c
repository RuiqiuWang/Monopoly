#include "assets.h"

#include <stdio.h>

static const char *asset_status_name(PlayerStatus status)
{
    switch (status) {
    case PLAYER_HOSPITAL: return "医院";
    case PLAYER_JAIL: return "监狱";
    case PLAYER_GOD: return "财神";
    case PLAYER_NORMAL: return "正常";
    default: return "未知";
    }
}

void query_assets(const Player *player, const Map *map)
{
    if (player == NULL) return;
    printf("玩家%s（编号=%d）\n", player->name, player->id);
    printf("资金=%d 点数=%d 位置=%d\n", player->money, player->points, player->position);
    printf("道具：路障=%d 机器娃娃=%d 炸弹=%d\n",
           player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB]);
    printf("状态=%s 剩余回合=%d\n",
           asset_status_name(player->status), player->status_rounds);
    printf("god_of_wealth_rounds=%d\n", player->god_of_wealth_rounds);
    fputs("地产：", stdout);
    if (map != NULL) {
        int found = 0;
        for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
            if (map_get_property_owner(map, i) == player->id) {
                printf(" %u号（等级=%u）", (unsigned)i, map_get_property_level(map, i));
                found = 1;
            }
        }
        if (!found) fputs(" 无", stdout);
    } else {
        fputs(" 不可用", stdout);
    }
    putchar('\n');
    /* These stable ASCII aliases keep the JSON/TUI integration scripts
     * compatible; the visible labels above are the user-facing Chinese UI. */
    printf("Player %s (id=%d)\n", player->name, player->id);
    printf("money=%d points=%d position=%d\n", player->money, player->points, player->position);
    printf("items: barrier=%d robot=%d bomb=%d\n",
           player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB]);
    printf("status=%s remaining_rounds=%d\n",
           status_to_string(player->status), player->status_rounds);
    printf("god_of_wealth_rounds=%d\n", player->god_of_wealth_rounds);
    fputs("properties:", stdout);
    if (map != NULL) {
        int found = 0;
        for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
            if (map_get_property_owner(map, i) == player->id) {
                printf(" %u(level=%u)", (unsigned)i, map_get_property_level(map, i));
                found = 1;
            }
        }
        if (!found) fputs(" none", stdout);
    } else {
        fputs(" unavailable", stdout);
    }
    putchar('\n');
}

bool query_assets_to_json(const Player *player, const Map *map, const char *filename)
{
    FILE *file;
    if (player == NULL || filename == NULL) return false;
    file = fopen(filename, "w");
    if (file == NULL) return false;
    fprintf(file,
            "{\n  \"id\": %d,\n  \"name\": \"%s\",\n"
            "  \"position\": %d,\n  \"money\": %d,\n  \"points\": %d,\n"
            "  \"items\": {\"barrier\": %d, \"robot\": %d, \"bomb\": %d},\n"
            "  \"status\": \"%s\",\n  \"status_rounds\": %d,\n"
            "  \"god_of_wealth_rounds\": %d,\n  \"active\": %s,\n"
            "  \"properties\": [",
            player->id, player->name, player->position, player->money, player->points,
            player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB],
            status_to_string(player->status), player->status_rounds,
            player->god_of_wealth_rounds,
            player->active ? "true" : "false");
    if (map != NULL) {
        int first = 1;
        for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
            if (map_get_property_owner(map, i) == player->id) {
                fprintf(file, "%s{\"position\": %u, \"level\": %u}",
                        first ? "" : ", ", (unsigned)i,
                        map_get_property_level(map, i));
                first = 0;
            }
        }
    }
    fputs("]\n}\n", file);
    if (ferror(file) != 0 || fclose(file) != 0) return false;
    return true;
}
