#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "map.h"
#include "movement.h"
#include "tui.h"

#define MAX_PLAYERS 4
#define INITIAL_PLAYER_MONEY 1000

void Check_Player_in_Mine(Player *someone)
{
    if (someone == NULL || !someone->active) {
        return;
    }

    if (someone->position >= 64 && someone->position < MAP_BLOCK_COUNT) {
        int bonus = 100 + (someone->position - 64) * 50;
        someone->money += bonus;
    }
}

int Get_Random_Step(void)
{
    return (rand() % 6) + 1;
}

static int read_player_count(void)
{
    return MAX_PLAYERS;
}

static void initialize_players(Player players[], int player_count)
{
    static const char player_names[MAX_PLAYERS] = {'A', 'Q', 'S', 'J'};
    static const PlayerColor player_colors[MAX_PLAYERS] = {
        COLOR_GREEN,
        COLOR_RED,
        COLOR_BLUE,
        COLOR_YELLOW
    };

    for (int i = 0; i < player_count; ++i) {
        memset(&players[i], 0, sizeof(players[i]));
        players[i].id = i + 1;
        players[i].name[0] = player_names[i];
        players[i].name[1] = '\0';
        players[i].color = player_colors[i];
        players[i].position = 0;
        players[i].money = INITIAL_PLAYER_MONEY;
        players[i].status = PLAYER_NORMAL;
        players[i].active = 1;
    }
}

static void build_tui_players(
    const Player players[],
    int player_count,
    const unsigned long arrival_order[MAX_PLAYERS],
    int current_index,
    int property_focus_index,
    TuiPlayerView views[MAX_PLAYERS])
{
    for (int i = 0; i < player_count; ++i) {
        views[i].id = players[i].id;
        views[i].name = players[i].name;
        views[i].color = players[i].color;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].arrival_order = arrival_order[i];
        views[i].active = players[i].active != 0;
        views[i].current = i == current_index;
        views[i].property_focus = i == property_focus_index;
    }
}

/* Resolve the landing in one transaction so the following redraw shows both
 * the new position and the resulting ownership/level immediately. */
static void resolve_landing(Map *map, Player players[], int player_count, Player *player,
                            char *message, size_t message_size)
{
    size_t position;
    BlockBits block;
    int owner_id;
    unsigned int level;
    int price;

    if (map == NULL || player == NULL || message == NULL || message_size == 0 ||
        player->position < 0 || player->position >= MAP_BLOCK_COUNT) {
        return;
    }
    position = (size_t)player->position;
    block = map_get_block(map, position);
    if (!map_block_is_purchasable(block)) {
        snprintf(message + strlen(message), message_size - strlen(message),
                 " 落在不可购买地块。");
        return;
    }

    owner_id = map_get_property_owner(map, position);
    level = map_get_property_level(map, position);
    price = (int)map_get_cost(map, position);
    if (owner_id == MAP_PROPERTY_UNOWNED && player->money >= price) {
        if (map_set_property(map, position, player->id, 1)) {
            player->money -= price;
            snprintf(message + strlen(message), message_size - strlen(message),
                     " 买下了这块地产（等级 1）。");
        }
    } else if (owner_id == player->id && level < MAP_MAX_PROPERTY_LEVEL) {
        int upgrade_price = price / 2;
        if (upgrade_price < 1) {
            upgrade_price = 1;
        }
        if (player->money >= upgrade_price &&
            map_set_property(map, position, player->id, level + 1)) {
            player->money -= upgrade_price;
            snprintf(message + strlen(message), message_size - strlen(message),
                     " 将地产升级到等级 %u。", level + 1);
        }
    } else if (owner_id != MAP_PROPERTY_UNOWNED && owner_id != player->id) {
        int rent = price * (int)level / 10;
        for (int i = 0; i < player_count; ++i) {
            if (players[i].id == owner_id && players[i].active) {
                if (rent > player->money) {
                    rent = player->money;
                }
                player->money -= rent;
                players[i].money += rent;
                snprintf(message + strlen(message), message_size - strlen(message),
                         " 向玩家 %s 支付租金 %d。", players[i].name, rent);
                break;
            }
        }
    }
}

static void render_game_state(
    const Map *map,
    const Player players[],
    int player_count,
    const unsigned long arrival_order[MAX_PLAYERS],
    int current_index,
    int property_focus_index,
    int round,
    const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];

    build_tui_players(players, player_count, arrival_order, current_index,
                      property_focus_index, views);
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 1.0    Round: %d    当前行动者: %s\n",
           round, views[current_index].name);
    if (message != NULL) {
        printf("%s\n\n", message);
    } else {
        putchar('\n');
    }
    tui_render_game(map, views, (size_t)player_count);
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    int player_count;
    int round = 1;
    int current_index = 0;
    int property_focus_index = 0;
    int have_action = 0;
    unsigned long arrival_order[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;

    srand((unsigned int)time(NULL));
    player_count = read_player_count();
    initialize_players(players, player_count);
    map_init(&map);

    render_game_state(&map, players, player_count, arrival_order, current_index,
                      property_focus_index, round, "游戏初始化完成。");

    for (;;) {
        char input[32];
        char message[256];
        PlayerMoveResult move_result;
        int step;

        printf("\n按 Enter%s，输入 q 后 Enter 退出: ",
               have_action ? " 让下一位玩家行动" : " 让当前玩家行动");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        if (input[0] == 'q' || input[0] == 'Q') {
            break;
        }
        if (input[0] != '\n' && input[0] != '\r' && input[0] != '\0') {
            snprintf(message, sizeof(message), "请输入 Enter 执行回合，或输入 q 退出。");
            render_game_state(&map, players, player_count, arrival_order, current_index,
                              property_focus_index, round, message);
            continue;
        }

        while (!players[current_index].active) {
            current_index = (current_index + 1) % player_count;
        }

        step = Get_Random_Step();
        move_result = Move_Player(&players[current_index], step);
        arrival_order[current_index] = ++arrival_counter;
        if (move_result != PLAYER_MOVE_OK) {
            snprintf(message, sizeof(message), "玩家 %s 掷出 %d，但移动失败。",
                     players[current_index].name, step);
        } else {
            Check_Player_in_Mine(&players[current_index]);
            snprintf(message, sizeof(message), "玩家 %s 掷出 %d，当前位置 %d。",
                     players[current_index].name, step, players[current_index].position);
            resolve_landing(&map, players, player_count, &players[current_index],
                            message, sizeof(message));
        }
        property_focus_index = current_index;
        have_action = 1;
        current_index = (current_index + 1) % player_count;
        if (current_index == 0) {
            ++round;
        }
        while (!players[current_index].active) {
            current_index = (current_index + 1) % player_count;
        }
        render_game_state(&map, players, player_count, arrival_order, current_index,
                          property_focus_index, round, message);
    }

    tui_clear_screen();
    puts("MONOPOLY 已退出。");
    return 0;
}
