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
#include "gift_house.h"
#include "help_query.h"
#include "input.h"
#include "item_effect.h"
#include "item_usage.h"
#include "jail.h"
#include "map.h"
#include "mine.h"
#include "movement.h"
#include "property.h"
#include "tool_room.h"
#include "tui.h"
#include "tutorial.h"

#define MAX_PLAYERS CHARACTER_SELECT_MAX_PLAYERS
#define DEFAULT_PLAYER_MONEY 10000
#define MIN_PLAYER_MONEY 1000
#define MAX_PLAYER_MONEY 50000

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

static int prompt_initial_money(void)
{
    char input[64];
    for (;;) {
        char *end;
        long value;
        if (!input_read_line("Initial money [1000-50000, Enter=10000]: ",
                             input, sizeof(input))) return DEFAULT_PLAYER_MONEY;
        if (input[0] == '\0') return DEFAULT_PLAYER_MONEY;
        value = strtol(input, &end, 10);
        if (*end == '\0' && value >= MIN_PLAYER_MONEY && value <= MAX_PLAYER_MONEY) {
            return (int)value;
        }
        puts("Initial money must be an integer from 1000 to 50000.");
    }
}

static int initialize_players(Player players[], const CharacterSelection *selection,
                              int initial_money)
{
    if (players == NULL || selection == NULL) return 0;
    for (int i = 0; i < selection->chosen_count; ++i) {
        char role = selection->chosen[i];
        memset(&players[i], 0, sizeof(players[i]));
        players[i].id = i + 1;
        players[i].name[0] = role;
        players[i].name[1] = '\0';
        players[i].color = CharacterSelect_RoleColor(role);
        players[i].money = initial_money;
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

static bool prompt_confirmation(const char *prompt)
{
    char input[16];

    if (!input_read_line(prompt, input, sizeof(input))) return false;
    return (input[0] == 'y' || input[0] == 'Y') && input[1] == '\0';
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
    char prompt[96];

    if (map == NULL || player == NULL || player->position < 0) return;
    position = (size_t)player->position;
    if (!map_valid_index(position) ||
        !map_block_is_purchasable(map_get_block(map, position))) return;
    owner_id = map_get_property_owner(map, position);
    level = map_get_property_level(map, position);

    if (owner_id == MAP_PROPERTY_UNOWNED && try_buy_land(map, player)) {
        snprintf(prompt, sizeof(prompt), "Buy property %u for %.0f? [Y/N]: ",
                 (unsigned)position, map_get_cost(map, position));
        if (prompt_confirmation(prompt) && buy_land(map, player)) {
            append_message(message, size, " Property purchased at level 0.");
        } else {
            append_message(message, size, " Property purchase declined.");
        }
    } else if (owner_id == player->id && try_upgrade_property(map, player)) {
        snprintf(prompt, sizeof(prompt),
                 "Upgrade property %u to level %u for %.0f? [Y/N]: ",
                 (unsigned)position, level + 1, map_get_cost(map, position));
        if (prompt_confirmation(prompt) && upgrade_property(map, player)) {
            char detail[80];
            snprintf(detail, sizeof(detail), " Property upgraded to level %u.", level + 1);
            append_message(message, size, detail);
        } else {
            append_message(message, size, " Property upgrade declined.");
        }
    } else if (owner_id != MAP_PROPERTY_UNOWNED && owner_id != player->id) {
        int owner = find_player_index(players, player_count, owner_id);
        if (owner >= 0 && players[owner].active) {
            char detail[96];
            if (player->god_of_wealth_rounds > 0) {
                snprintf(detail, sizeof(detail),
                         " God of Wealth waived rent to %s.", players[owner].name);
            } else if (players[owner].status == PLAYER_HOSPITAL ||
                       players[owner].status == PLAYER_JAIL) {
                snprintf(detail, sizeof(detail),
                         " Rent waived because %s is unavailable.", players[owner].name);
            } else if (collect_toll(map, player, &players[owner])) {
                int rent = property_toll(map, player->position);
                snprintf(detail, sizeof(detail),
                         " Paid rent %d to %s.", rent, players[owner].name);
            } else {
                return;
            }
            append_message(message, size, detail);
        }
    }
}

static bool sell_property_with_message(
    Map *map, Player *player, int position, char *message, size_t size)
{
    int value;
    if (!try_sell_property(map, player, position)) {
        snprintf(message, size, "Player %s does not own property %d.", player->name, position);
        return false;
    }
    value = property_sale_price(map, position);
    if (!sell_property(map, player, position)) return false;
    snprintf(message, size, "Player %s sold property %d for %d.",
             player->name, position, value);
    return true;
}

static void describe_item_result(ItemUseResult result, const char *success,
                                 char *message, size_t size)
{
    const char *text = success;
    if (result == ITEM_USE_DISTANCE_OUT_OF_RANGE) text = "Offset must be between -10 and 10.";
    else if (result == ITEM_USE_NOT_OWNED) text = "Item unavailable.";
    else if (result == ITEM_USE_TARGET_OCCUPIED) text = "Target already contains an item.";
    else if (result != ITEM_USE_OK) text = "Error: item could not be used.";
    snprintf(message, size, "%s", text);
}

static void print_help(void)
{
    fputs(HelpQuery_Text(), stdout);
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
    int initial_money;
    int current_index = 0;
    int focus_index = 0;
    int round = 1;

    configure_console_encoding();
    srand((unsigned int)time(NULL));
    map_init(&map);
    initial_money = prompt_initial_money();
    if (!CharacterSelect_Prompt(&selection)) return 1;
    player_count = initialize_players(players, &selection, initial_money);

    tutorial_state_load(&tutorial_state, MONOPOLY_STATE_FILE);
    if (!tutorial_state.has_run) {
        TutorialChoice choice = tutorial_prompt_first_run();
        bool completed = choice == TUTORIAL_CHOICE_NO;
        if (choice == TUTORIAL_CHOICE_YES) completed = tutorial_run(&map);
        if (completed) {
            tutorial_state.has_run = true;
            tutorial_state_save(&tutorial_state, MONOPOLY_STATE_FILE);
        }
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
        int god_rounds_at_turn_start;
        ItemEffectReport movement_report;
        ItemEffectMoveResult movement_result;

        if (Process_Hospital_Turn(&players[current_index]) ==
            ITEM_EFFECT_TURN_SKIPPED) {
            int skipped = current_index;
            int next;
            snprintf(message, sizeof(message),
                     "Player %s skips this turn in hospital (%d remaining).",
                     players[skipped].name, players[skipped].status_rounds);
            focus_index = skipped;
            next = next_active_player(players, player_count, skipped);
            if (next < 0) break;
            if (next <= skipped) ++round;
            current_index = next;
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, message);
            continue;
        }
        if (Process_Jail_Turn(&players[current_index]) == JAIL_TURN_SKIPPED) {
            int skipped = current_index;
            int next;
            snprintf(message, sizeof(message),
                     "Player %s skips this turn in jail (%d remaining).",
                     players[skipped].name, players[skipped].status_rounds);
            focus_index = skipped;
            next = next_active_player(players, player_count, skipped);
            if (next < 0) break;
            if (next <= skipped) ++round;
            current_index = next;
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, message);
            continue;
        }

        if (!input_read_line("Command (Enter=roll): ", input, sizeof(input))) break;
        if ((input[0] == 'q' || input[0] == 'Q') && input[1] == '\0') break;
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
        if (command.type == COMMAND_RESET) {
            const char *reset_message = tutorial_state_reset(MONOPOLY_STATE_FILE)
                ? "Play record cleared. The tutorial will be offered on next launch."
                : "Error: could not clear the play record.";
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, reset_message);
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
            ItemUseResult item_result = command.type == COMMAND_BLOCK
                ? Use_Block(&players[current_index], &map, command.argument)
                : Use_Bomb(&players[current_index], &map, command.argument);
            int target = (players[current_index].position + command.argument) %
                MAP_BLOCK_COUNT;
            if (target < 0) target += MAP_BLOCK_COUNT;
            if (item_result == ITEM_USE_OK) {
                snprintf(message, sizeof(message), "%s placed at %d.",
                         command.type == COMMAND_BLOCK ? "Barrier" : "Bomb", target);
            } else {
                describe_item_result(item_result, "", message, sizeof(message));
            }
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }
        if (command.type == COMMAND_ROBOT) {
            describe_item_result(Use_Robot(&players[current_index], &map),
                "Robot cleared items in the next 10 blocks.", message, sizeof(message));
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }
        if (command.type == COMMAND_SELL) {
            sell_property_with_message(&map, &players[current_index], command.argument,
                                       message, sizeof(message));
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }

        action_index = current_index;
        steps = command.type == COMMAND_ROLL ? Get_Random_Step() : command.steps;
        god_rounds_at_turn_start = players[action_index].god_of_wealth_rounds;
        if (action_index < 0 || !players[action_index].active) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, "Error: player does not exist or is inactive.");
            continue;
        }
        movement_result = Move_Player_With_Item_Effects(
            &players[action_index], &map, steps, &movement_report);
        if (movement_result < ITEM_EFFECT_MOVE_OK) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, "Error: movement failed.");
            continue;
        }
        arrivals[action_index] = ++arrival_counter;
        snprintf(message, sizeof(message), "Player %s moved %d of %d steps to %d.",
                 players[action_index].name, movement_report.travelled_steps,
                 steps, players[action_index].position);
        if (movement_result == ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER) {
            append_message(message, sizeof(message), " Stopped by a barrier.");
        } else if (movement_result == ITEM_EFFECT_MOVE_SENT_TO_HOSPITAL) {
            append_message(message, sizeof(message), " Hit a bomb and was sent to hospital.");
        }
        if (!movement_report.skip_landing_event) {
            BlockBits landing = map_get_block(
                &map, (size_t)players[action_index].position);
            if (Check_Player_in_Jail(&players[action_index], &map) == JAIL_CHECK_ENTERED) {
                append_message(message, sizeof(message),
                               " Sent to jail for the next two turns.");
            }
            Check_Player_in_Mine(&players[action_index], &map);
            if (map_block_is_tool_room(landing)) Enter_Tool_Room(&players[action_index]);
            if (map_block_is_gift_room(landing)) Gift_House_Prompt(&players[action_index]);
            resolve_property(&map, players, player_count, &players[action_index],
                             message, sizeof(message));
        }
        if (god_rounds_at_turn_start > 0 &&
            players[action_index].god_of_wealth_rounds > 0) {
            --players[action_index].god_of_wealth_rounds;
        }
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
