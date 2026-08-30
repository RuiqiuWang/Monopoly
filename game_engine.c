#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "assets.h"
#include "command.h"
#include "map.h"
#include "mine.h"
#include "movement.h"
#include "tool_room.h"
#include "tui.h"
#include "tutorial.h"

#define MAX_PLAYERS 4
#define INITIAL_PLAYER_MONEY 1000

static void configure_console_encoding(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stdin), _O_BINARY);
#endif
}

int Get_Random_Step(void)
{
    return (rand() % 6) + 1;
}

static void initialize_players(Player players[])
{
    static const char names[MAX_PLAYERS] = {'A', 'Q', 'S', 'J'};
    static const PlayerColor colors[MAX_PLAYERS] = {
        COLOR_GREEN, COLOR_RED, COLOR_BLUE, COLOR_YELLOW
    };
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        memset(&players[i], 0, sizeof(players[i]));
        players[i].id = i + 1;
        players[i].name[0] = names[i];
        players[i].name[1] = '\0';
        players[i].color = colors[i];
        players[i].money = INITIAL_PLAYER_MONEY;
        players[i].status = PLAYER_NORMAL;
        players[i].active = 1;
    }
}

static int find_player_index(const Player players[], int id)
{
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (players[i].id == id) return i;
    }
    return -1;
}

static void render_game_state(const Map *map, const Player players[],
                              const unsigned long arrivals[], int round,
                              const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        views[i].name = players[i].name;
        views[i].color = players[i].color;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].arrival_order = arrivals[i];
        views[i].active = players[i].active != 0;
    }
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 1.0    Round: %d\n", round);
    if (message != NULL) printf("%s\n\n", message);
    tui_render_game(map, views, MAX_PLAYERS);
}

static void print_help(void)
{
    puts("Commands: step <id> <steps>, roll, query, block <offset>, bomb <offset>, robot, help, quit");
}

static void apply_map_item(Map *map, Player *player, CommandType type, int offset)
{
    int target;
    int inventory = type == COMMAND_BLOCK ? ITEM_BARRIER : ITEM_BOMB;
    if (offset < -10 || offset > 10 || player->items[inventory] <= 0) {
        puts("Item unavailable or offset out of range.");
        return;
    }
    target = (player->position + offset) % MAP_BLOCK_COUNT;
    if (target < 0) target += MAP_BLOCK_COUNT;
    map_set_item(map, (size_t)target,
                 type == COMMAND_BLOCK ? HAS_OBSTACLE : HAS_BOMB);
    --player->items[inventory];
    puts(type == COMMAND_BLOCK ? "Barrier placed." : "Bomb placed.");
}

static void apply_robot(Map *map, Player *player)
{
    if (player->items[ITEM_ROBOT] <= 0) {
        puts("Robot unavailable.");
        return;
    }
    --player->items[ITEM_ROBOT];
    for (int distance = 1; distance <= 10; ++distance) {
        int target = (player->position + distance) % MAP_BLOCK_COUNT;
        map_set_item(map, (size_t)target, NO_ITEM);
    }
    puts("Robot cleared nearby items.");
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    unsigned long arrivals[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;
    TutorialState tutorial_state;
    int round = 1;

    configure_console_encoding();
    srand((unsigned int)time(NULL));
    map_init(&map);
    tutorial_state_load(&tutorial_state, MONOPOLY_STATE_FILE);
    if (!tutorial_state.has_run) {
        tutorial_state.has_run = true;
        tutorial_state_save(&tutorial_state, MONOPOLY_STATE_FILE);
        if (tutorial_prompt_first_run()) tutorial_run(&map);
    }
    initialize_players(players);
    render_game_state(&map, players, arrivals, round, "Game initialized. Use help for commands.");

    for (;;) {
        char input[128];
        Command command;
        CommandResult result;
        printf("\nCommand: ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        result = Parse_Command(input, &command);
        if (result != COMMAND_OK) {
            puts(Command_Result_Message(result));
            continue;
        }
        if (command.type == COMMAND_QUIT) break;
        if (command.type == COMMAND_HELP) {
            print_help();
            continue;
        }
        {
            int current = (round - 1) % MAX_PLAYERS;
            if (command.type == COMMAND_QUERY) {
                int queried = command.player_id > 0
                    ? find_player_index(players, command.player_id) : current;
                if (queried < 0) {
                    puts("Error: player does not exist.");
                    continue;
                }
                query_assets(&players[queried], &map);
                (void)query_assets_to_json(&players[queried], &map, "player_assets.json");
                continue;
            }
            if (command.type == COMMAND_BLOCK || command.type == COMMAND_BOMB) {
                apply_map_item(&map, &players[current], command.type, command.argument);
                continue;
            }
            if (command.type == COMMAND_ROBOT) {
                apply_robot(&map, &players[current]);
                continue;
            }
            if (command.type == COMMAND_SELL) {
                puts("Property selling is unavailable until ownership is added to Map.");
                continue;
            }
            {
                int index = command.type == COMMAND_ROLL
                    ? current : find_player_index(players, command.player_id);
                int steps = command.type == COMMAND_ROLL ? Get_Random_Step() : command.steps;
                PlayerMoveResult move_result;
                if (index < 0 || !players[index].active) {
                    puts("Error: player does not exist or is inactive.");
                    continue;
                }
                move_result = Move_Player(&players[index], steps);
                if (move_result != PLAYER_MOVE_OK) {
                    puts("Error: movement failed.");
                    continue;
                }
                arrivals[index] = ++arrival_counter;
                Check_Player_in_Mine(&players[index], &map);
                if (map_block_is_tool_room(map_get_block(&map, (size_t)players[index].position))) {
                    Enter_Tool_Room(&players[index]);
                }
                printf("Player %s moved %d steps to %d.\n",
                       players[index].name, steps, players[index].position);
                ++round;
            }
        }
    }
    puts("Game ended.");
    return 0;
}
