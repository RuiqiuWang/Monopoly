#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "map.h"
#include "movement.h"
#include "tui.h"
#include "command.h"
#include "tutorial.h"

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

static int find_player_index(const Player players[], int player_count, int player_id)
{
    for (int i = 0; i < player_count; ++i) {
        if (players[i].id == player_id) return i;
    }
    return -1;
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    int player_count;
    int round = 1;
    unsigned long arrival_order[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;
    TutorialState tutorial_state;

    srand((unsigned int)time(NULL));
    map_init(&map);
    tutorial_state_load(&tutorial_state, MONOPOLY_STATE_FILE);
    if (!tutorial_state.has_run) {
        tutorial_state.has_run = true;
        tutorial_state_save(&tutorial_state, MONOPOLY_STATE_FILE);
        if (tutorial_prompt_first_run()) {
            tutorial_run(&map);
        }
    }
    player_count = read_player_count();
    initialize_players(players, player_count);
    render_game_state(&map, players, player_count, arrival_order, round, "游戏初始化完成。输入 step [id] [number] 移动玩家，输入 quit 退出。");

    for (;;) {
        char input[128];
        char message[160];
        Command command;
        CommandResult command_result;

        printf("\n输入命令：");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        command_result = Parse_Command(input, &command);
        if (command_result != COMMAND_OK) {
            render_game_state(&map, players, player_count, arrival_order, round, Command_Result_Message(command_result));
            continue;
        }
        if (command.type == COMMAND_QUIT) break;

        {
            int index = find_player_index(players, player_count, command.player_id);
            PlayerMoveResult move_result;
            if (index < 0 || !players[index].active) {
                render_game_state(&map, players, player_count, arrival_order, round, "错误：玩家 id 不存在或玩家已退出。");
                continue;
            }
            move_result = Move_Player(&players[index], command.steps);
            if (move_result != PLAYER_MOVE_OK) {
                render_game_state(&map, players, player_count, arrival_order, round, "错误：玩家移动失败。");
                continue;
            }
            arrival_order[index] = ++arrival_counter;
            Check_Player_in_Mine(&players[index]);
            snprintf(message, sizeof(message), "玩家 %s 移动 %d 步，当前位置 %d。", players[index].name, command.steps, players[index].position);
            render_game_state(&map, players, player_count, arrival_order, round, message);
        }
        ++round;
    }

    puts("\n游戏结束。");
    return 0;
}
