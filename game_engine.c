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
#include "fortune.h"
#include "gift_house.h"
#include "help_query.h"
#include "input.h"
#include "item_effect.h"
#include "item_usage.h"
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

/* The input area is part of the same frame as the board.  Keeping the last
 * command lets a refreshed frame show exactly what produced its message. */
static char g_last_command[128] = "";

static void remember_command(const char *input)
{
    if (input == NULL) {
        g_last_command[0] = '\0';
        return;
    }
    snprintf(g_last_command, sizeof(g_last_command), "%s", input);
}

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
        if (!input_read_line("初始资金【1000-50000，回车=10000】：",
                             input, sizeof(input))) return DEFAULT_PLAYER_MONEY;
        if (input[0] == '\0') return DEFAULT_PLAYER_MONEY;
        value = strtol(input, &end, 10);
        if (*end == '\0' && value >= MIN_PLAYER_MONEY && value <= MAX_PLAYER_MONEY) {
            return (int)value;
        }
        input_clear_screen();
        puts("输入无效（Invalid Input）");
        puts("初始资金必须是 1000 到 50000 之间的整数。\n"
             "(Initial money must be an integer from 1000 to 50000.)");
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

static void render_game_state(
    const Map *map, const Player players[], int player_count,
    const unsigned long arrivals[], int current_index, int focus_index,
    int round, const char *message);

typedef struct {
    const Map *map;
    const Player *players;
    int player_count;
    const unsigned long *arrivals;
    int current_index;
    int focus_index;
    int round;
} GameView;

static void refresh_game_view(const char *message, void *context)
{
    const GameView *view = context;
    if (view == NULL) return;
    render_game_state(view->map, view->players, view->player_count,
                      view->arrivals, view->current_index, view->focus_index,
                      view->round, message);
}

static bool prompt_confirmation(const char *prompt,
                                InputRefreshCallback refresh, void *context)
{
    char input[16];

    for (;;) {
        if (!input_read_line(prompt, input, sizeof(input))) return false;
        if (input[1] == '\0' &&
            (input[0] == 'y' || input[0] == 'Y')) return true;
        if (input[1] == '\0' &&
            (input[0] == 'n' || input[0] == 'N')) return false;
        if (refresh != NULL) refresh("输入无效（Invalid Input）：请输入 Y 或 N。", context);
        else {
            input_clear_screen();
            puts("输入无效（Invalid Input）：请输入 Y 或 N。");
        }
    }
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
    printf("大富翁游戏 2.0    回合：%d", round);
    if (current_index >= 0) printf("    当前行动：%s", players[current_index].name);
    putchar('\n');
    tui_render_game(map, views, (size_t)player_count);
    puts("");
    if (g_last_command[0] != '\0') {
        printf("命令：%s\n", g_last_command);
    }
    if (message != NULL && message[0] != '\0') {
        printf("提示：%s\n", message);
    }
}

static void reclaim_properties(Map *map, int player_id)
{
    for (size_t i = 0; map != NULL && i < MAP_BLOCK_COUNT; ++i) {
        if (map_get_property_owner(map, i) == player_id) map_clear_property(map, i);
    }
}

static void update_bankruptcy(Map *map, Player *player, char *message, size_t size)
{
    char compatibility[96];
    if (player == NULL || !player->active || player->money >= 0) return;
    reclaim_properties(map, player->id);
    player->active = 0;
    player->is_winner = 0;
    append_message(message, size, "玩家");
    append_message(message, size, player->name);
    append_message(message, size, "已破产，地产已归还系统。");
    snprintf(compatibility, sizeof(compatibility),
             " (Player %s is bankrupt; properties returned to the bank.)",
             player->name);
    append_message(message, size, compatibility);
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
    char *message, size_t size, InputRefreshCallback refresh, void *context)
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
        snprintf(prompt, sizeof(prompt), "是否花费 %.0f 购买 %u 号地产？【Y/N】：",
                 map_get_cost(map, position), (unsigned)position);
        if (prompt_confirmation(prompt, refresh, context) && buy_land(map, player)) {
            append_message(message, size, "已购买地产，等级为 0。（Property purchased at level 0.）");
        } else {
            append_message(message, size, "已放弃购买地产。 Property purchase declined.");
        }
    } else if (owner_id == player->id && try_upgrade_property(map, player)) {
        snprintf(prompt, sizeof(prompt),
                 "是否花费 %.0f 将 %u 号地产升级到 %u 级？【Y/N】：",
                 map_get_cost(map, position), (unsigned)position, level + 1);
        if (prompt_confirmation(prompt, refresh, context) && upgrade_property(map, player)) {
            char detail[80];
            snprintf(detail, sizeof(detail), "地产已升级到 %u 级。（Property upgraded to level %u.）",
                     level + 1, level + 1);
            append_message(message, size, detail);
        } else {
            append_message(message, size, "已放弃升级地产。 Property upgrade declined.");
        }
    } else if (owner_id != MAP_PROPERTY_UNOWNED && owner_id != player->id) {
        int owner = find_player_index(players, player_count, owner_id);
        if (owner >= 0 && players[owner].active) {
            char detail[96];
            if (player->god_of_wealth_rounds > 0) {
                snprintf(detail, sizeof(detail),
                         "财神效果生效，本次免向%s支付租金。", players[owner].name);
            } else if (collect_toll(map, player, &players[owner])) {
                int rent = property_toll(map, player->position);
                snprintf(detail, sizeof(detail),
                         "已向%s支付租金%d。（Paid rent %d to %s.）",
                         players[owner].name, rent, rent, players[owner].name);
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
        snprintf(message, size, "操作失败：玩家%s不拥有%d号地产。", player->name, position);
        return false;
    }
    value = property_sale_price(map, position);
    if (!sell_property(map, player, position)) return false;
    snprintf(message, size, "玩家%s已出售%d号地产，获得%d。 (Player %s sold property %d for %d.)",
             player->name, position, value, player->name, position, value);
    return true;
}

static void describe_item_result(ItemUseResult result, const char *success,
                                 char *message, size_t size)
{
    const char *text = success;
    if (result == ITEM_USE_DISTANCE_OUT_OF_RANGE) text = "操作失败：距离必须在 -10 到 10 之间。";
    else if (result == ITEM_USE_NOT_OWNED) text = "操作失败：你没有这个道具。（Item unavailable.）";
    else if (result == ITEM_USE_TARGET_OCCUPIED) text = "操作失败：目标位置已有道具。";
    else if (result != ITEM_USE_OK) text = "操作失败：道具无法使用。";
    snprintf(message, size, "%s", text);
}

static const char *movement_result_message(ItemEffectMoveResult result)
{
    switch (result) {
    case ITEM_EFFECT_MOVE_INVALID_ARGUMENT:
        return "移动失败：玩家或地图数据无效。";
    case ITEM_EFFECT_MOVE_INVALID_POSITION:
        return "移动失败：玩家位置无效。";
    case ITEM_EFFECT_MOVE_INVALID_STEP:
        return "输入无效（Invalid Input）：步数不能为负数。";
    case ITEM_EFFECT_MOVE_FAILED:
    default:
        return "移动失败：无法完成移动。";
    }
}

static void print_help(void)
{
    fputs(HelpQuery_Text(), stdout);
}

static const char *localized_command_error(CommandResult result)
{
    switch (result) {
    case COMMAND_UNKNOWN: return "错误：未知命令，请输入 help 查看可用命令。";
    case COMMAND_MISSING_ARGUMENT: return "错误：缺少必要参数。";
    case COMMAND_INVALID_NUMBER: return "错误：参数必须是整数。";
    case COMMAND_OUT_OF_RANGE: return "错误：数值参数超出允许范围。";
    case COMMAND_EXTRA_ARGUMENT: return "错误：命令包含多余参数。";
    case COMMAND_INVALID_ARGUMENT: return "错误：命令输入无效。";
    default: return "错误：命令执行失败。";
    }
}

int main(void)
{
    Map map;
    Player players[MAX_PLAYERS];
    CharacterSelection selection;
    TutorialState tutorial_state;
    FortuneState fortune;
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
    Fortune_Init(&fortune);
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
                      focus_index, round, "游戏已开始。输入 help 查看帮助。");

    for (;;) {
        char input[128];
        char message[320] = "";
        Command command;
        CommandResult result;
        int action_index;
        int steps;
        int start_position;
        bool fortune_acquired_this_turn = false;
        ItemEffectReport movement_report;
        ItemEffectMoveResult movement_result;

        if (!input_read_line("命令：", input, sizeof(input))) break;
        remember_command(input);
        result = Parse_Command(input, &command);
        if (result != COMMAND_OK) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, localized_command_error(result));
            continue;
        }
        if (command.type == COMMAND_QUIT) break;
        if (command.type == COMMAND_HELP) {
            print_help();
            continue;
        }
        if (command.type == COMMAND_RESET) {
            const char *reset_message = tutorial_state_reset(MONOPOLY_STATE_FILE)
                ? "游戏记录已清除，下次启动时将再次显示新手教程。 (Play record cleared.)"
                : "操作失败：无法清除游戏记录。";
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, reset_message);
            continue;
        }
        if (command.type == COMMAND_QUERY) {
            query_assets(&players[current_index], &map);
            (void)query_assets_to_json(
                &players[current_index], &map, "player_assets.json");
            continue;
        }
        if (command.type == COMMAND_BLOCK) {
            ItemUseResult item_result =
                Use_Block(&players[current_index], &map, command.argument);
            int target = (players[current_index].position + command.argument) %
                MAP_BLOCK_COUNT;
            if (target < 0) target += MAP_BLOCK_COUNT;
            if (item_result == ITEM_USE_OK) {
                snprintf(message, sizeof(message),
                         "路障已放置在%d号位置。 (Barrier placed at %d.)",
                         target, target);
            } else {
                describe_item_result(item_result, "", message, sizeof(message));
            }
            render_game_state(&map, players, player_count, arrivals, current_index,
                              current_index, round, message);
            continue;
        }
        if (command.type == COMMAND_ROBOT) {
            describe_item_result(Use_Robot(&players[current_index], &map),
                "机器娃娃已清除前方10格内的路障。", message, sizeof(message));
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
        if (action_index < 0 || !players[action_index].active) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round, "操作失败：玩家不存在或已退出游戏。");
            continue;
        }
        if (command.type == COMMAND_STEP && steps < 0) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round,
                              "输入无效（Invalid Input）：步数不能为负数。");
            continue;
        }
        start_position = players[action_index].position;
        movement_result = Move_Player_With_Item_Effects(
            &players[action_index], &map, steps, &movement_report);
        if (movement_result < ITEM_EFFECT_MOVE_OK) {
            render_game_state(&map, players, player_count, arrivals, current_index,
                              focus_index, round,
                              movement_result_message(movement_result));
            continue;
        }
        if (movement_report.travelled_steps > 0) {
            arrivals[action_index] = ++arrival_counter;
        }
        snprintf(message, sizeof(message), "玩家%s移动了%d/%d步，到达%d号位置。 (Player %s moved %d of %d steps to %d.)",
                 players[action_index].name, movement_report.travelled_steps,
                 steps, players[action_index].position, players[action_index].name,
                 movement_report.travelled_steps, steps, players[action_index].position);
        if (movement_result == ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER) {
            append_message(message, sizeof(message), "遇到路障，移动停止。 Stopped by a barrier.");
        }
        fortune_acquired_this_turn = Fortune_Collect_On_Path(
            &fortune, &map, &players[action_index], start_position,
            movement_report.travelled_steps, round);
        if (fortune_acquired_this_turn) {
            append_message(message, sizeof(message),
                           " 路过财神并成功领取，本回合起免交租金。 (Fortune collected.)");
        }
        if (!movement_report.skip_landing_event) {
            BlockBits landing = map_get_block(
                &map, (size_t)players[action_index].position);
            GameView view = {
                &map, players, player_count, arrivals,
                action_index, action_index, round
            };
            Check_Player_in_Mine(&players[action_index], &map);
            /* Draw the moved state before entering a landing prompt.  Retry
             * callbacks redraw this same game page with the latest message. */
            render_game_state(&map, players, player_count, arrivals,
                              action_index, action_index, round, message);
            if (map_block_is_tool_room(landing)) {
                Enter_Tool_Room_With_Refresh(&players[action_index],
                                             refresh_game_view, &view);
            }
            if (map_block_is_gift_room(landing)) {
                GiftHouseChoice gift_choice = (GiftHouseChoice)0;
                if (Gift_House_Prompt_With_Choice(
                        &players[action_index], refresh_game_view, &view,
                        &gift_choice) &&
                    gift_choice == GIFT_HOUSE_GOD_OF_WEALTH) {
                    fortune_acquired_this_turn = true;
                }
            }
            resolve_property(&map, players, player_count, &players[action_index],
                             message, sizeof(message), refresh_game_view, &view);
        }
        Fortune_End_Player_Turn(
            &players[action_index], fortune_acquired_this_turn);
        update_bankruptcy(&map, &players[action_index], message, sizeof(message));
        focus_index = action_index;

        {
            int winner = mark_winner(players, player_count);
            if (winner >= 0) {
                append_message(message, sizeof(message), "获胜者： Winner: ");
                append_message(message, sizeof(message), players[winner].name);
                append_message(message, sizeof(message), " (WINNER)");
                current_index = winner;
                render_game_state(&map, players, player_count, arrivals, current_index,
                                  focus_index, round, message);
                break;
            }
        }

        {
            int next = next_active_player(players, player_count, action_index);
            FortuneTurnResult fortune_result;
            if (next < 0) break;
            fortune_result = Fortune_Advance_Turn(
                &fortune, &map, players, (size_t)player_count, round);
            ++round;
            if (fortune_result == FORTUNE_TURN_SPAWNED) {
                char detail[96];
                snprintf(detail, sizeof(detail),
                         " 财神已随机出现在%d号位置。 (Fortune appeared at %d.)",
                         fortune.position, fortune.position);
                append_message(message, sizeof(message), detail);
            } else if (fortune_result == FORTUNE_TURN_EXPIRED) {
                append_message(message, sizeof(message),
                               " 财神已消失，将在未来10回合内随机再次出现。");
            }
            current_index = next;
        }
        render_game_state(&map, players, player_count, arrivals, current_index,
                          focus_index, round, message);
    }
    puts("游戏结束。");
    return 0;
}
