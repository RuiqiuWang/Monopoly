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
        if (players[i].id == id) {
            return i;
        }
    }
    return -1;
}

static void build_tui_players(const Player players[],
                              const unsigned long arrivals[], int current_index,
                              int property_focus_index,
                              TuiPlayerView views[MAX_PLAYERS])
{
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        views[i].name = players[i].name;
        views[i].color = players[i].color;
        views[i].money = players[i].money;
        views[i].position = players[i].position;
        views[i].arrival_order = arrivals[i];
        views[i].active = players[i].active != 0;
        views[i].current = i == current_index;
        views[i].property_focus = i == property_focus_index;
        views[i].id = players[i].id;
        views[i].winner = players[i].is_winner != 0;
    }
}

static void append_message(char *message, size_t message_size, const char *text)
{
    size_t used;
    if (message == NULL || message_size == 0 || text == NULL) return;
    used = strlen(message);
    if (used < message_size - 1) {
        snprintf(message + used, message_size - used, "%s", text);
    }
}

static void reclaim_player_properties(Map *map, int player_id)
{
    if (map == NULL) return;
    for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
        if (map_get_property_owner(map, i) == player_id) {
            map_clear_property(map, i);
        }
    }
}

static bool bankrupt_player(Map *map, Player *player, char *message, size_t message_size)
{
    if (player == NULL || !player->active || player->money >= 0) return false;
    reclaim_player_properties(map, player->id);
    player->active = 0;
    player->is_winner = 0;
    append_message(message, message_size, " 玩家 ");
    append_message(message, message_size, player->name);
    append_message(message, message_size, " 资金低于 0，已破产；其地产已归还系统。");
    return true;
}

static int count_active_players(const Player players[])
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (players[i].active) ++count;
    }
    return count;
}

static void resolve_landing(Map *map, Player players[], Player *player,
                            char *message, size_t message_size)
{
    size_t position;
    BlockBits block;
    int owner_id;
    unsigned int level;
    int price;

    if (map == NULL || player == NULL || message == NULL || message_size == 0 ||
        player->position < 0 || player->position >= MAP_BLOCK_COUNT) return;
    position = (size_t)player->position;
    block = map_get_block(map, position);
    if (!map_block_is_purchasable(block)) {
        append_message(message, message_size, " 落在不可购买地块。");
        return;
    }
    owner_id = map_get_property_owner(map, position);
    level = map_get_property_level(map, position);
    price = (int)map_get_cost(map, position);
    if (owner_id == MAP_PROPERTY_UNOWNED && player->money >= price) {
        if (map_set_property(map, position, player->id, 1)) {
            player->money -= price;
            append_message(message, message_size, " 买下了这块地产（等级 1）。");
        }
    } else if (owner_id == player->id && level < MAP_MAX_PROPERTY_LEVEL) {
        int upgrade_price = price / 2;
        if (upgrade_price < 1) upgrade_price = 1;
        if (player->money >= upgrade_price &&
            map_set_property(map, position, player->id, level + 1)) {
            player->money -= upgrade_price;
            snprintf(message + strlen(message), message_size - strlen(message),
                     " 将地产升级到等级 %u。", level + 1);
        }
    } else if (owner_id != MAP_PROPERTY_UNOWNED && owner_id != player->id) {
        int rent = price * (int)level / 10;
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (players[i].id == owner_id && players[i].active) {
                player->money -= rent;
                players[i].money += rent;
                snprintf(message + strlen(message), message_size - strlen(message),
                         " 向玩家 %s 支付租金 %d。", players[i].name, rent);
                break;
            }
        }
    }
}

static void render_game_state(const Map *map, const Player players[],
                              const unsigned long arrivals[], int current_index,
                              int property_focus_index, int round,
                              const char *message)
{
    TuiPlayerView views[MAX_PLAYERS];
    build_tui_players(players, arrivals, current_index, property_focus_index, views);
    tui_clear_screen();
    printf("MONOPOLY GAME ENGINE 1.0    Round: %d    当前行动者: %s\n",
           round, views[current_index].name);
    if (message != NULL) printf("%s\n\n", message);
    tui_render_game(map, views, MAX_PLAYERS);
}

static void print_help(void)
{
    puts("Commands: Enter/roll, step <id> <steps>, query, block <offset>, "
         "bomb <offset>, robot, help, quit");
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
}

static void apply_robot(Map *map, Player *player)
{
    if (player->items[ITEM_ROBOT] <= 0) return;
    --player->items[ITEM_ROBOT];
    for (int distance = 1; distance <= 10; ++distance) {
        int target = (player->position + distance) % MAP_BLOCK_COUNT;
        map_set_item(map, (size_t)target, NO_ITEM);
    }
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    unsigned long arrivals[MAX_PLAYERS] = {1, 2, 3, 4};
    unsigned long arrival_counter = 4;
    TutorialState tutorial_state;
    int current_index = 0;
    int property_focus_index = 0;
    int round = 1;
    int have_action = 0;

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
    render_game_state(&map, players, arrivals, current_index, property_focus_index,
                      round, "游戏初始化完成。");

    for (;;) {
        char input[128];
        char message[256];
        Command command;
        CommandResult command_result;
        int action_index = current_index;
        int step;

        printf("\n按 Enter%s，输入 q/quit 后 Enter 退出: ",
               have_action ? " 让下一位玩家行动" : " 让当前玩家行动");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        if ((input[0] == 'q' || input[0] == 'Q') &&
            (input[1] == '\n' || input[1] == '\r' || input[1] == '\0')) {
            break;
        }

        if (input[0] == '\n' || input[0] == '\r' || input[0] == '\0') {
            step = Get_Random_Step();
        } else {
            command_result = Parse_Command(input, &command);
            if (command_result != COMMAND_OK) {
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round,
                                  Command_Result_Message(command_result));
                continue;
            }
            if (command.type == COMMAND_QUIT) break;
            if (command.type == COMMAND_HELP) {
                print_help();
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round, "帮助已显示。");
                continue;
            }
            if (command.type == COMMAND_QUERY) {
                int queried = command.player_id > 0
                    ? find_player_index(players, command.player_id) : current_index;
                if (queried >= 0) {
                    query_assets(&players[queried], &map);
                    (void)query_assets_to_json(&players[queried], &map, "player_assets.json");
                }
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round, "玩家信息查询完成。");
                continue;
            }
            if (command.type == COMMAND_BLOCK || command.type == COMMAND_BOMB) {
                apply_map_item(&map, &players[current_index], command.type, command.argument);
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round, "地图道具状态已更新。");
                continue;
            }
            if (command.type == COMMAND_ROBOT) {
                apply_robot(&map, &players[current_index]);
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round, "机器人操作完成。");
                continue;
            }
            if (command.type == COMMAND_STEP) {
                action_index = find_player_index(players, command.player_id);
                step = command.steps;
            } else if (command.type == COMMAND_ROLL) {
                step = Get_Random_Step();
            } else {
                render_game_state(&map, players, arrivals, current_index,
                                  property_focus_index, round, "当前命令不执行移动。");
                continue;
            }
        }

        if (action_index < 0 || action_index >= MAX_PLAYERS ||
            !players[action_index].active) {
            render_game_state(&map, players, arrivals, current_index,
                              property_focus_index, round,
                              "错误：玩家不存在或已经破产。");
            continue;
        }
        message[0] = '\0';
        {
            PlayerMoveResult move_result = Move_Player(&players[action_index], step);
            if (move_result != PLAYER_MOVE_OK) {
                snprintf(message, sizeof(message), "玩家 %s 移动失败。",
                         players[action_index].name);
            } else {
                arrivals[action_index] = ++arrival_counter;
                Check_Player_in_Mine(&players[action_index], &map);
                if (map_block_is_tool_room(map_get_block(&map,
                        (size_t)players[action_index].position))) {
                    Enter_Tool_Room(&players[action_index]);
                }
                snprintf(message, sizeof(message), "玩家 %s 移动 %d 步，当前位置 %d。",
                         players[action_index].name, step,
                         players[action_index].position);
                resolve_landing(&map, players, &players[action_index],
                                message, sizeof(message));
            }
        }

        property_focus_index = action_index;
        have_action = 1;
        bankrupt_player(&map, &players[action_index], message, sizeof(message));
        if (count_active_players(players) <= 1) {
            int winner_index = -1;
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                players[i].is_winner = 0;
                if (players[i].active) {
                    winner_index = i;
                    players[i].is_winner = 1;
                }
            }
            if (winner_index >= 0) {
                append_message(message, sizeof(message), " 仅剩一名未破产玩家，");
                append_message(message, sizeof(message), players[winner_index].name);
                append_message(message, sizeof(message), " 获胜！");
                current_index = winner_index;
                property_focus_index = winner_index;
            } else {
                append_message(message, sizeof(message), " 所有玩家均已破产，游戏结束。");
            }
            render_game_state(&map, players, arrivals, current_index,
                              property_focus_index, round, message);
            break;
        }

        current_index = (action_index + 1) % MAX_PLAYERS;
        if (current_index == 0) ++round;
        while (!players[current_index].active) {
            current_index = (current_index + 1) % MAX_PLAYERS;
        }
        render_game_state(&map, players, arrivals, current_index,
                          property_focus_index, round, message);
    }

    tui_clear_screen();
    puts("MONOPOLY 已退出。");
    return 0;
}
