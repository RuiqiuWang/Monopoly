#include "tui.h"

#include <stdio.h>
#include <string.h>

#define TUI_ROWS 18
#define TUI_COLS 19

typedef struct {
    int row;
    int col;
} TuiCoord;

static const TuiCoord kMapCoords[MAP_BLOCK_COUNT] = {
    {0, 0},
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8},
    {0, 9}, {0, 10}, {0, 11}, {0, 12}, {0, 13}, {0, 14}, {0, 15},
    {0, 16}, {0, 17}, {0, 18},

    {1, 18}, {2, 18}, {3, 18}, {4, 18}, {5, 18}, {6, 18}, {7, 18},
    {8, 18}, {9, 18}, {10, 18}, {11, 18}, {12, 18}, {13, 18},
    {14, 18}, {15, 18}, {16, 18}, {17, 18},

    {17, 17}, {17, 16}, {17, 15}, {17, 14}, {17, 13}, {17, 12},
    {17, 11}, {17, 10}, {17, 9}, {17, 8}, {17, 7}, {17, 6}, {17, 5},
    {17, 4}, {17, 3}, {17, 2}, {17, 1}, {17, 0},

    {16, 0}, {15, 0}, {14, 0}, {13, 0}, {12, 0}, {11, 0}, {10, 0},
    {9, 0}, {8, 0}, {7, 0}, {6, 0}, {5, 0}, {4, 0}, {3, 0}, {2, 0},
    {1, 0}
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
        return "J";
    }
    if (map_block_is_mine(block)) {
        return "$";
    }
    if (map_block_is_purchasable(block)) {
        if (map_block_is_plot(block, IS_PLOT_ONE)) {
            return "1";
        }
        if (map_block_is_plot(block, IS_PLOT_TWO)) {
            return "2";
        }
        if (map_block_is_plot(block, IS_PLOT_THREE)) {
            return "3";
        }
        return "P";
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
        strcpy(board[coord.row][coord.col], tui_block_symbol(map_get_block(map, i)));
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
    size_t index)
{
    const TuiPlayerView *first_player = NULL;
    int occupied = 0;

    if (players == NULL || index >= MAP_BLOCK_COUNT) {
        return NULL;
    }

    for (size_t i = 0; i < player_count; ++i) {
        if (!players[i].active || players[i].position != (int)index) {
            continue;
        }
        if (first_player == NULL) {
            first_player = &players[i];
        }
        ++occupied;
    }

    if (occupied == 0) {
        return NULL;
    }
    if (occupied > 1) {
        return "*";
    }
    if (first_player->name != NULL && first_player->name[0] != '\0') {
        static char player_mark[2];
        player_mark[0] = first_player->name[0];
        player_mark[1] = '\0';
        return player_mark;
    }
    return "@";
}

static void tui_print_board(
    char board[TUI_ROWS][TUI_COLS][4],
    const TuiPlayerView *players,
    size_t player_count)
{
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            size_t index = tui_index_at_coord(r, c);
            const char *mark = tui_player_mark(players, player_count, index);

            if (mark != NULL) {
                snprintf(board[r][c], sizeof(board[r][c]), "%s", mark);
            }
        }
    }

    puts("+---------------------------------------------------------------------------------------+");
    for (int r = 0; r < TUI_ROWS; ++r) {
        fputs("|", stdout);
        for (int c = 0; c < TUI_COLS; ++c) {
            printf(" %-2s", board[r][c]);
        }
        puts(" |");
    }
    puts("+---------------------------------------------------------------------------------------+");
}

void tui_clear_screen(void)
{
    fputs("\033[2J\033[H", stdout);
}

void tui_render_map(const Map *map)
{
    char board[TUI_ROWS][TUI_COLS][4];

    tui_build_board(board, map);
    tui_print_board(board, NULL, 0);
}

void tui_render_game(const Map *map, const TuiPlayerView *players, size_t player_count)
{
    char board[TUI_ROWS][TUI_COLS][4];

    tui_build_board(board, map);
    tui_print_board(board, players, player_count);

    puts("");
    puts("Legend: S=Start T=Tool G=Gift M=Magic H=Hospital J=Jail $=Mine 1/2/3=Land");

    if (players == NULL || player_count == 0) {
        return;
    }

    puts("Players:");
    for (size_t i = 0; i < player_count; ++i) {
        if (!players[i].active) {
            continue;
        }
        printf("  %s  money=%d  pos=%d\n",
               players[i].name != NULL ? players[i].name : "(null)",
               players[i].money,
               players[i].position);
    }
}
