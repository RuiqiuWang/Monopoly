#include "tui.h"

#include <stdio.h>
#include <string.h>

#define TUI_ROWS 8
#define TUI_COLS 29
#define ANSI_RESET "\033[0m"

typedef struct {
    int row;
    int col;
} TuiCoord;

static const TuiCoord kMapCoords[MAP_BLOCK_COUNT] = {
    {0, 0},
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8},
    {0, 9}, {0, 10}, {0, 11}, {0, 12}, {0, 13}, {0, 14}, {0, 15},
    {0, 16}, {0, 17}, {0, 18}, {0, 19}, {0, 20}, {0, 21}, {0, 22},
    {0, 23}, {0, 24}, {0, 25}, {0, 26}, {0, 27}, {0, 28},

    {1, 28}, {2, 28}, {3, 28}, {4, 28}, {5, 28}, {6, 28},

    {7, 28},
    {7, 27}, {7, 26}, {7, 25}, {7, 24}, {7, 23}, {7, 22}, {7, 21},
    {7, 20}, {7, 19}, {7, 18}, {7, 17}, {7, 16}, {7, 15}, {7, 14},
    {7, 13}, {7, 12}, {7, 11}, {7, 10}, {7, 9}, {7, 8}, {7, 7},
    {7, 6}, {7, 5}, {7, 4}, {7, 3}, {7, 2}, {7, 1}, {7, 0},

    {6, 0}, {5, 0}, {4, 0}, {3, 0}, {2, 0}, {1, 0}
};

static const char *tui_block_symbol(BlockBits block)
{
    if (map_block_is_start(block)) {
        return "S";
    }
    if (map_block_is_tool_room(block)) {
        return "T";
    }
    if (map_block_is_gift_room(block)) {
        return "G";
    }
    if (map_block_is_magic_room(block)) {
        return "M";
    }
    if (map_block_is_hospital(block)) {
        return "H";
    }
    if (map_block_is_jail(block)) {
        return "P";
    }
    if (map_block_is_mine(block)) {
        return "$";
    }
    if (map_block_is_purchasable(block)) {
        return "0";
    }
    return " ";
}

static void tui_build_board(char board[TUI_ROWS][TUI_COLS][4], const Map *map)
{
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            strcpy(board[r][c], " ");
        }
    }

    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
        const TuiCoord coord = kMapCoords[i];
        BlockBits block = map_get_block(map, i);

        strcpy(board[coord.row][coord.col], tui_block_symbol(block));
        if (map_block_is_purchasable(block) &&
            map_get_property_owner(map, i) != MAP_PROPERTY_UNOWNED) {
            /* Level 0 is an owned empty plot; levels 1-3 are the
             * progressively upgraded buildings shown on the board. */
            snprintf(board[coord.row][coord.col], 4, "%u",
                     map_get_property_level(map, i));
        }
    }
}

static size_t tui_index_at_coord(int row, int col)
{
    for (size_t i = 0; i < MAP_BLOCK_COUNT; ++i) {
        if (kMapCoords[i].row == row && kMapCoords[i].col == col) {
            return i;
        }
    }
    return MAP_BLOCK_COUNT;
}

static const char *tui_player_mark(
    const TuiPlayerView *players,
    size_t player_count,
    size_t index,
    PlayerColor *player_color)
{
    const TuiPlayerView *first_player = NULL;
    unsigned long latest_arrival = 0;

    if (players == NULL || index >= MAP_BLOCK_COUNT) {
        return NULL;
    }

    for (size_t i = 0; i < player_count; ++i) {
        if (!players[i].active || players[i].position != (int)index) {
            continue;
        }
        if (first_player == NULL || players[i].arrival_order >= latest_arrival) {
            first_player = &players[i];
            latest_arrival = players[i].arrival_order;
        }
    }

    if (first_player == NULL) {
        return NULL;
    }

    if (player_color != NULL) {
        *player_color = first_player->color;
    }
    if (first_player->name != NULL && first_player->name[0] != '\0') {
        static char player_mark[2];
        player_mark[0] = first_player->name[0];
        player_mark[1] = '\0';
        return player_mark;
    }
    return "@";
}

static PlayerColor tui_property_owner_color(
    const Map *map, const TuiPlayerView *players, size_t player_count,
    size_t index)
{
    int owner_id;

    if (map == NULL || players == NULL || index >= MAP_BLOCK_COUNT ||
        !map_block_is_purchasable(map_get_block(map, index))) {
        return (PlayerColor)-1;
    }
    owner_id = map_get_property_owner(map, index);
    if (owner_id == MAP_PROPERTY_UNOWNED) return (PlayerColor)-1;
    for (size_t i = 0; i < player_count; ++i) {
        if (players[i].id == owner_id) return players[i].color;
    }
    return (PlayerColor)-1;
}

static void tui_print_board(
    char board[TUI_ROWS][TUI_COLS][4],
    PlayerColor board_colors[TUI_ROWS][TUI_COLS],
    const Map *map,
    const TuiPlayerView *players,
    size_t player_count)
{
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            size_t index = tui_index_at_coord(r, c);
            PlayerColor property_color = tui_property_owner_color(
                map, players, player_count, index);
            PlayerColor player_color = COLOR_GREEN;
            const char *mark = tui_player_mark(players, player_count, index, &player_color);

            if (property_color >= COLOR_GREEN && property_color <= COLOR_YELLOW) {
                board_colors[r][c] = property_color;
            }

            if (mark != NULL) {
                snprintf(board[r][c], sizeof(board[r][c]), "%s", mark);
                board_colors[r][c] = player_color;
            }
        }
    }

    puts("+---------------------------------------------------------------------------------------+");
    for (int r = 0; r < TUI_ROWS; ++r) {
        fputs("|", stdout);
        for (int c = 0; c < TUI_COLS; ++c) {
            if (board_colors[r][c] == COLOR_GREEN) {
                fputs("\033[32m", stdout);
            } else if (board_colors[r][c] == COLOR_RED) {
                fputs("\033[31m", stdout);
            } else if (board_colors[r][c] == COLOR_BLUE) {
                fputs("\033[34m", stdout);
            } else if (board_colors[r][c] == COLOR_YELLOW) {
                fputs("\033[33m", stdout);
            }
            printf(" %-2s", board[r][c]);
            if (board_colors[r][c] >= COLOR_GREEN && board_colors[r][c] <= COLOR_YELLOW) {
                fputs(ANSI_RESET, stdout);
            }
        }
        puts(" |");
    }
    puts("+---------------------------------------------------------------------------------------+");
}

void tui_clear_screen(void)
{
    fputs("\033[H\033[2J", stdout);
    fflush(stdout);
}

static const char *tui_block_name(BlockBits block)
{
    if (map_block_is_start(block)) return "起点";
    if (map_block_is_tool_room(block)) return "道具屋";
    if (map_block_is_gift_room(block)) return "礼品屋";
    if (map_block_is_magic_room(block)) return "魔法屋";
    if (map_block_is_hospital(block)) return "医院";
    if (map_block_is_jail(block)) return "监狱";
    if (map_block_is_mine(block)) return "矿地";
    if (map_block_is_purchasable(block)) return "地产";
    return "空地";
}

static const char *tui_plot_name(BlockBits block)
{
    if (map_block_is_plot(block, IS_PLOT_ONE)) return "一区";
    if (map_block_is_plot(block, IS_PLOT_TWO)) return "二区";
    if (map_block_is_plot(block, IS_PLOT_THREE)) return "三区";
    return "特殊区域";
}

static const TuiPlayerView *tui_focus_player(
    const TuiPlayerView *players, size_t player_count)
{
    if (players == NULL) return NULL;
    for (size_t i = 0; i < player_count; ++i) {
        if (players[i].property_focus) return &players[i];
    }
    for (size_t i = 0; i < player_count; ++i) {
        if (players[i].current) return &players[i];
    }
    return NULL;
}

static const TuiPlayerView *tui_find_player(
    const TuiPlayerView *players, size_t player_count, int id)
{
    for (size_t i = 0; players != NULL && i < player_count; ++i) {
        if (players[i].id == id) return &players[i];
    }
    return NULL;
}

static void tui_render_property(
    const Map *map, const TuiPlayerView *players, size_t player_count)
{
    const TuiPlayerView *focus = tui_focus_player(players, player_count);
    BlockBits block;

    puts("");
    puts("当前地产信息：");
    if (map == NULL || focus == NULL || focus->position < 0 ||
        focus->position >= MAP_BLOCK_COUNT) {
        puts("  暂无地产信息。");
        return;
    }
    block = map_get_block(map, (size_t)focus->position);
    printf("  玩家=%s 位置=%d 类型=%s\n",
           focus->name != NULL ? focus->name : "?", focus->position,
           tui_block_name(block));
    if (map_block_is_purchasable(block)) {
        int owner_id = map_get_property_owner(map, (size_t)focus->position);
        const TuiPlayerView *owner = tui_find_player(players, player_count, owner_id);
        printf("  价格=%.0f 区域=%s 归属=%s 等级=%u/%d\n",
               map_get_cost(map, (size_t)focus->position), tui_plot_name(block),
               owner != NULL && owner->name != NULL ? owner->name : "无主",
               map_get_property_level(map, (size_t)focus->position),
               MAP_MAX_PROPERTY_LEVEL);
        printf("  owner=%s level=%u/%d\n",
               owner != NULL && owner->name != NULL ? owner->name : "unowned",
               map_get_property_level(map, (size_t)focus->position),
               MAP_MAX_PROPERTY_LEVEL);
    }
}

void tui_render_map(const Map *map)
{
    char board[TUI_ROWS][TUI_COLS][4];
    PlayerColor board_colors[TUI_ROWS][TUI_COLS];

    tui_build_board(board, map);
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            board_colors[r][c] = (PlayerColor)-1;
        }
    }
    tui_print_board(board, board_colors, map, NULL, 0);
}

void tui_render_game(const Map *map, const TuiPlayerView *players, size_t player_count)
{
    char board[TUI_ROWS][TUI_COLS][4];
    PlayerColor board_colors[TUI_ROWS][TUI_COLS];

    tui_build_board(board, map);
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            board_colors[r][c] = (PlayerColor)-1;
        }
    }
    tui_print_board(board, board_colors, map, players, player_count);

    puts("");
    puts("图例：S=起点 T=道具屋 G=礼品屋 M=魔法屋 H=医院 P=监狱 $=矿地 0=未购买 1=茅屋 2=洋房 3=摩天楼");

    if (players == NULL || player_count == 0) {
        return;
    }

    tui_render_property(map, players, player_count);

    puts("");
    puts("玩家（> 表示当前行动，胜者表示最终赢家）：");
    for (size_t i = 0; i < player_count; ++i) {
        printf("%c %s  资金=%d  位置=%d%s%s\n",
               players[i].current ? '>' : ' ',
               players[i].name != NULL ? players[i].name : "(null)",
               players[i].money,
               players[i].position,
               players[i].active ? "" : "  已破产",
               players[i].winner ? "  获胜者" : "");
    }
}
