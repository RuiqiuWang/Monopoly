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

static void tui_print_board(
    char board[TUI_ROWS][TUI_COLS][4],
    PlayerColor board_colors[TUI_ROWS][TUI_COLS],
    const TuiPlayerView *players,
    size_t player_count)
{
    for (int r = 0; r < TUI_ROWS; ++r) {
        for (int c = 0; c < TUI_COLS; ++c) {
            size_t index = tui_index_at_coord(r, c);
            PlayerColor player_color = COLOR_GREEN;
            const char *mark = tui_player_mark(players, player_count, index, &player_color);

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
    fputs("\033[2J\033[H", stdout);
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
    tui_print_board(board, board_colors, NULL, 0);
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
    tui_print_board(board, board_colors, players, player_count);

    puts("");
    puts("Legend: S=Start/Player T=Tool G=Gift M=Magic H=Hospital P=Prison $=Mine 0=Land");

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
