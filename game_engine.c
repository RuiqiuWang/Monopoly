#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "map.h"
#include "movement.h"
#include "tui.h"

#define MAX_PLAYERS 4
#define INITIAL_PLAYER_MONEY 1000
#define MAX_ROUNDS 12

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
    TuiPlayerView views[MAX_PLAYERS])
{
    for (int i = 0; i < player_count; ++i) {
        views[i].name = players[i].name;
        views[i].color = players[i].color;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].arrival_order = arrival_order[i];
        views[i].active = players[i].active != 0;
    }
}

static void render_game_state(
    const Map *map,
    const Player players[],
    int player_count,
    const unsigned long arrival_order[MAX_PLAYERS],
    int round,
    const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];

    build_tui_players(players, player_count, arrival_order, views);
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 1.0    Round: %d\n", round);
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
    unsigned long arrival_order[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;

    srand((unsigned int)time(NULL));
    player_count = read_player_count();
    initialize_players(players, player_count);
    map_init(&map);

    render_game_state(&map, players, player_count, arrival_order, round, "游戏初始化完成。");

    for (int outer_round = 0; outer_round < MAX_ROUNDS; ++outer_round) {
        for (int i = 0; i < player_count; ++i) {
            char message[160];
            PlayerMoveResult move_result;
            int step;

            if (!players[i].active) {
                continue;
            }

            step = Get_Random_Step();
            move_result = Move_Player(&players[i], step);
            arrival_order[i] = ++arrival_counter;
            if (move_result != PLAYER_MOVE_OK) {
                snprintf(message,
                         sizeof(message),
                         "玩家 %s 掷出 %d，但移动失败。",
                         players[i].name,
                         step);
            } else {
                Check_Player_in_Mine(&players[i]);
                snprintf(message,
                         sizeof(message),
                         "玩家 %s 掷出 %d，当前位置 %d。",
                         players[i].name,
                         step,
                         players[i].position);
            }

            render_game_state(&map, players, player_count, arrival_order, round, message);
        }
        ++round;
    }

    puts("\n演示结束。");
    return 0;
}
