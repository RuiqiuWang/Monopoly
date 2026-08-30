#include <stdbool.h>
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
#include "character_select.h"
#include "command.h"
#include "input.h"
#include "map.h"
#include "mine.h"
#include "movement.h"
#include "tool_room.h"
#include "tui.h"
#include "tutorial.h"

#define MAX_PLAYERS CHARACTER_SELECT_MAX_PLAYERS
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

static int initialize_players(Player players[], const CharacterSelection *selection)
{
    if (players == NULL || selection == NULL) return 0;
    for (int i = 0; i < selection->chosen_count; ++i) {
        char role = selection->chosen[i];
        memset(&players[i], 0, sizeof(players[i]));
        players[i].id = i + 1;
        players[i].name[0] = role;
        players[i].name[1] = '\0';
        players[i].color = CharacterSelect_RoleColor(role);
        players[i].money = INITIAL_PLAYER_MONEY;
        players[i].status = PLAYER_NORMAL;
        players[i].active = 1;
    }
    return selection->chosen_count;
}

static int find_player_index(const Player players[], int player_count, int id)
{
    for (int i = 0; i < player_count; ++i) {
        if (players[i].id == id) return i;
    }
    return -1;
}

static int next_active_player(const Player players[], int player_count, int index)
{
    for (int checked = 0; checked < player_count; ++checked) {
        index = (index + 1) % player_count;
        if (players[index].active) return index;
    }
    return -1;
}

static void append_message(char *message, size_t size, const char *text)
{
    size_t used;
    if (message == NULL || size == 0 || text == NULL) return;
    used = strlen(message);
    if (used < size - 1) snprintf(message + used, size - used, "%s", text);
}

static void render_game_state(
    const Map *map, const Player players[], int player_count,
    const unsigned long arrivals[], int current_index, int focus_index,
    int round, const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];
    for (int i = 0; i < player_count; ++i) {
        views[i].id = players[i].id;
        views[i].name = players[i].name;
        views[i].color = players[i].color;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].arrival_order = arrivals[i];
        views[i].active = players[i].active != 0;
        views[i].current = i == current_index;
        views[i].property_focus = i == focus_index;
        views[i].winner = players[i].is_winner != 0;
    }
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 2.0    Round: %d", round);
    if (current_index >= 0) printf("    Current: %s", players[current_index].name);
    putchar('\n');
    if (message != NULL && message[0] != '\0') printf("%s\n\n", message);
    tui_render_game(map, views, (size_t)player_count);
}

static void reclaim_properties(Map *map, int player_id)
{
    for (size_t i = 0; map != NULL && i < MAP_BLOCK_COUNT; ++i) {
        if (map_get_property_owner(map, i) == player_id) map_clear_property(map, i);
    }
}

static void update_bankruptcy(Map *map, Player *player, char *message, size_t size)
{
    if (player == NULL || !player->active || player->money >= 0) return;
    reclaim_properties(map, player->id);
    player->active = 0;
    player->is_winner = 0;
    append_message(message, size, " Player ");
    append_message(message, size, player->name);
    append_message(message, size, " is bankrupt; properties returned to the bank.");
}

static int mark_winner(Player players[], int player_count)
{
    int active_count = 0;
    int winner = -1;
    for (int i = 0; i < player_count; ++i) {
        players[i].is_winner = 0;
        if (players[i].active) {
            ++active_count;
            winner = i;
        }
    }
    if (active_count == 1) players[winner].is_winner = 1;
    return active_count == 1 ? winner : -1;
}

static void resolve_property(
    Map *map, Player players[], int player_count, Player *player,
    char *message, size_t size)
{
    size_t position;
    int owner_id;
    unsigned int level;
    int price;

    if (map == NULL || player == NULL || player->position < 0) return;
    position = (size_t)player->position;
    if (!map_valid_index(position) ||
        !map_block_is_purchasable(map_get_block(map, position))) return;
    owner_id = map_get_property_owner(map, position);
    level = map_get_property_level(map, position);
    price = (int)map_get_cost(map, position);

    if (owner_id == MAP_PROPERTY_UNOWNED && player->money >= price) {
        if (map_set_property(map, position, player->id, 1)) {
            player->money -= price;
            append_message(message, size, " Property purchased at level 1.");
        }
    } else if (owner_id == player->id && level < MAP_MAX_PROPERTY_LEVEL) {
        int upgrade_price = price / 2;
        if (player->money >= upgrade_price &&
            map_set_property(map, position, player->id, level + 1)) {
            char detail[80];
            player->money -= upgrade_price;
            snprintf(detail, sizeof(detail), " Property upgraded to level %u.", level + 1);
            append_message(message, size, detail);
        }
    } else if (owner_id != MAP_PROPERTY_UNOWNED && owner_id != player->id) {
        int owner = find_player_index(players, player_count, owner_id);
        int rent = price * (int)level / 10;
        if (owner >= 0 && players[owner].active) {
            char detail[96];
            player->money -= rent;
            players[owner].money += rent;
            snprintf(detail, sizeof(detail), " Paid rent %d to %s.", rent, players[owner].name);
            append_message(message, size, detail);
        }
    }
}

static bool sell_property(Map *map, Player *player, int position, char *message, size_t size)
{
    unsigned int level;
    int value;
    if (map == NULL || player == NULL || position < 0 ||
        !map_valid_index((size_t)position) ||
        map_get_property_owner(map, (size_t)position) != player->id) {
        snprintf(message, size, "Player %s does not own property %d.", player->name, position);
        return false;
    }
    level = map_get_property_level(map, (size_t)position);
    value = 2 * (int)map_get_cost(map, (size_t)position) * (int)(level + 1);
    player->money += value;
    map_clear_property(map, (size_t)position);
    snprintf(message, size, "Player %s sold property %d for %d.",
             player->name, position, value);
    return true;
}

static void apply_map_item(Map *map, Player *player, CommandType type, int offset,
                           char *message, size_t size)
{
    int target;
    int inventory = type == COMMAND_BLOCK ? ITEM_BARRIER : ITEM_BOMB;
    if (offset < -10 || offset > 10 || player->items[inventory] <= 0) {
        snprintf(message, size, "Item unavailable or offset out of range.");
        return;
    }
    target = (player->position + offset) % MAP_BLOCK_COUNT;
    if (target < 0) target += MAP_BLOCK_COUNT;
    map_set_item(map, (size_t)target,
                 type == COMMAND_BLOCK ? HAS_OBSTACLE : HAS_BOMB);
    --player->items[inventory];
    snprintf(message, size, "%s placed at %d.",
             type == COMMAND_BLOCK ? "Barrier" : "Bomb", target);
}

static void apply_robot(Map *map, Player *player, char *message, size_t size)
{
    if (player->items[ITEM_ROBOT] <= 0) {
        snprintf(message, size, "Robot unavailable.");
        return;
    }
    --player->items[ITEM_ROBOT];
    for (int distance = 1; distance <= 10; ++distance) {
        map_set_item(map, (size_t)((player->position + distance) % MAP_BLOCK_COUNT), NO_ITEM);
    }
    snprintf(message, size, "Robot cleared items in the next 10 blocks.");
}

static void print_help(void)
{
    puts("Commands: Enter/roll, step <id> <steps>, query [id], sell <position>,");
    puts("          block <offset>, bomb <offset>, robot, help, quit");
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    CharacterSelection selection;
    TutorialState tutorial_state;
    unsigned long arrivals[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;
    int player_count;
    int current_index = 0;
    int focus_index = 0;
    int round = 1;

    configure_console_encoding();
    srand((unsigned int)time(NULL));
    map_init(&map);
    if (!CharacterSelect_Prompt(&selection)) return 1;
    player_count = initialize_players(players, &selection);

    tutorial_state_load(&tutorial_state, MONOPOLY_STATE_FILE);
    if (!tutorial_state.has_run) {
        tutorial_state.has_run = true;
        tutorial_state_save(&tutorial_state, MONOPOLY_STATE_FILE);
        if (tutorial_prompt_first_run()) tutorial_run(&map);
    }
    render_game_state(&map, players, player_count, arrivals, current_index,
                      focus_index, round, "Game initialized. Use help for commands.");

    for (;;) {
        char input[128];
        char message[320] = "";
        Command command;
        CommandResult result;
        int action_index;
        int steps;

        if (!input_read_line("Command (Enter=roll): ", input, sizeof(input))) break;
        if (input[0] == '\0') {
            command.type = COMMAND_ROLL;
        } else {
            result = Parse_Command(input, &command);
            if (result != COMMAND_OK) {
                render_game_state(&map, players, player_count, arrivals, current_index,
                                  focus_index, round, Command_Result_Message(result));
                continue;
            }
        }
        if (command.type == COMMAND_QUIT) break;
        if (command.type == COMMAND_HELP) {
            print_help();
            continue;
        }
        if (command.type == COMMAND_QUERY) {
            int queried = command.player_id > 0
                ? find_player_index(players, player_count, command.player_id)
                : current_index;
            if (queried < 0) puts("Error: player does not exist.");
            else {
                query_assets(&players[queried], &map);
                (void)query_assets_to_json(&players[queried], &map, "player_assets.json");
            }
            continue;
        }
        if (command.type == COMMAND_BLOCK || command.type == COMMAND_BOMB) {
            apply_map_item(&map, &players[current_index], command.type,
                           command.argument, message, sizeof(message));
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }
        if (command.type == COMMAND_ROBOT) {
            apply_robot(&map, &players[current_index], message, sizeof(message));
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }
        if (command.type == COMMAND_SELL) {
            sell_property(&map, &players[current_index], command.argument,
                          message, sizeof(message));
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }

        action_index = command.type == COMMAND_ROLL
            ? current_index
            : find_player_index(players, player_count, command.player_id);
        steps = command.type == COMMAND_ROLL ? Get_Random_Step() : command.steps;
        if (action_index < 0 || !players[action_index].active) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, "Error: player does not exist or is inactive.");
            continue;
        }
        if (Move_Player(&players[action_index], steps) != PLAYER_MOVE_OK) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, "Error: movement failed.");
            continue;
        }
        arrivals[action_index] = ++arrival_counter;
        snprintf(message, sizeof(message), "Player %s moved %d steps to %d.",
                 players[action_index].name, steps, players[action_index].position);
        Check_Player_in_Mine(&players[action_index], &map);
        if (map_block_is_tool_room(
                map_get_block(&map, (size_t)players[action_index].position))) {
            Enter_Tool_Room(&players[action_index]);
        }
        resolve_property(&map, players, player_count, &players[action_index],
                         message, sizeof(message));
        update_bankruptcy(&map, &players[action_index], message, sizeof(message));
        focus_index = action_index;

        {
            int winner = mark_winner(players, player_count);
            if (winner >= 0) {
                append_message(message, sizeof(message), " Winner: ");
                append_message(message, sizeof(message), players[winner].name);
                current_index = winner;
                render_game_state(&map, players, player_count, arrivals, current_index,
                                  focus_index, round, message);
                break;
            }
        }

        {
            int next = next_active_player(players, player_count, action_index);
            if (next < 0) break;
            if (next <= action_index) ++round;
            current_index = next;
        }
        render_game_state(&map, players, player_count, arrivals, current_index,
                          focus_index, round, message);
    }
    puts("Game ended.");
    return 0;
}
