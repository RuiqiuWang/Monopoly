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
    (void)someone;
}

int Get_Random_Step(void)
{
    return rand() % 7;
}

static int read_player_count(void)
{
    int player_count = 0;

    while (player_count < 1 || player_count > MAX_PLAYERS) {
        printf("请输入玩家数量（1-%d）：", MAX_PLAYERS);
        fflush(stdout);

        if (scanf("%d", &player_count) != 1) {
            int input;

            while ((input = getchar()) != '\n' && input != EOF) {
                /* discard invalid input */
            }
            player_count = 0;
            puts("请输入 1 到 4 之间的数字。");
        }
    }

    while (getchar() != '\n') {
        /* discard the rest of the input line */
    }
    return player_count;
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
    TuiPlayerView views[MAX_PLAYERS])
{
    for (int i = 0; i < player_count; ++i) {
        views[i].name = players[i].name;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].active = players[i].active != 0;
    }
}

static void render_game_state(
    const Map *map,
    const Player players[],
    int player_count,
    int round,
    const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];

    build_tui_players(players, player_count, views);
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 1.0    Round: %d\n", round);
    if (message != NULL) {
        printf("%s\n\n", message);
    } else {
        putchar('\n');
    }
    tui_render_game(map, views, (size_t)player_count);
}

static int wait_for_turn(void)
{
    char input[16];

    printf("\n按回车开始下一步，输入 q 退出：");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }
    return input[0] != 'q' && input[0] != 'Q';
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    int player_count;
    int round = 1;

    srand((unsigned int)time(NULL));
    player_count = read_player_count();
    initialize_players(players, player_count);
    map_init(&map);

    render_game_state(&map, players, player_count, round, "游戏初始化完成。");

    for (;;) {
        for (int i = 0; i < player_count; ++i) {
            char message[160];
            PlayerMoveResult move_result;
            int step;

            if (!players[i].active) {
                continue;
            }

            if (!wait_for_turn()) {
                puts("\n游戏结束。");
                return 0;
            }

            step = Get_Random_Step();
            move_result = Move_Player(&players[i], step);
            if (move_result != PLAYER_MOVE_OK && step != 0) {
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

            render_game_state(&map, players, player_count, round, message);
        }
        ++round;
    }
}
