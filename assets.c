#include "assets.h"

#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

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

static int assets_stdout_is_terminal(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(STDOUT_FILENO);
#endif
}

void query_assets(const Player *player, const Map *map)
{
    int found = 0;
    if (player == NULL) return;
    puts("");
    puts("================ 玩家资产 ================");
    puts("+----------------------+----------------------+");
    printf("| 玩家：%-14s | 编号：%-14d |\n", player->name, player->id);
    printf("| 资金：%-14d | 点数：%-14d |\n", player->money, player->points);
    printf("| 位置：%-14d | 状态：%-14s |\n",
           player->position, asset_status_name(player->status));
    printf("| 剩余状态回合：%-6d | 财神回合：%-8d |\n",
           player->status_rounds, player->god_of_wealth_rounds);
    puts("+----------------------+----------------------+");
    puts("道具库存");
    puts("+----------------------+----------------------+");
    printf("| 路障：%-14d | 炸弹：%-14d |\n",
           player->items[ITEM_BARRIER], player->items[ITEM_BOMB]);
    printf("| 机器娃娃：%-10d | 道具总数：%-10d |\n",
           player->items[ITEM_ROBOT],
           player->items[ITEM_BARRIER] + player->items[ITEM_BOMB] +
               player->items[ITEM_ROBOT]);
    puts("+----------------------+----------------------+");
    puts("地产列表");
    puts("+------------+------------+");
    puts("| 位置       | 等级       |");
    puts("+------------+------------+");
    if (map != NULL) {
        for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
            if (map_get_property_owner(map, i) == player->id) {
                printf("| %-10u | %-10u |\n",
                       (unsigned)i, map_get_property_level(map, i));
                found = 1;
            }
        }
    }
    if (!found) puts("| （无）     |            |");
    puts("+------------+------------+");

    /* These stable ASCII aliases keep the JSON/TUI integration scripts
     * compatible; emit them only for redirected output, not an interactive
     * terminal where the Chinese table is the complete presentation. */
    if (!assets_stdout_is_terminal()) {
        printf("Player %s (id=%d)\n", player->name, player->id);
        printf("money=%d points=%d position=%d\n", player->money, player->points, player->position);
        printf("items: barrier=%d robot=%d bomb=%d\n",
               player->items[ITEM_BARRIER], player->items[ITEM_ROBOT], player->items[ITEM_BOMB]);
        printf("status=%s remaining_rounds=%d\n",
               status_to_string(player->status), player->status_rounds);
        printf("god_of_wealth_rounds=%d\n", player->god_of_wealth_rounds);
        fputs("properties:", stdout);
        if (map != NULL) {
            for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
                if (map_get_property_owner(map, i) == player->id) {
                    printf(" %u(level=%u)", (unsigned)i, map_get_property_level(map, i));
                }
            }
            if (!found) fputs(" none", stdout);
        } else {
            fputs(" unavailable", stdout);
        }
        putchar('\n');
    }
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
