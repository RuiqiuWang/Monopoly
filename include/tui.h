#ifndef MONOPOLY_TUI_H
#define MONOPOLY_TUI_H

#include <stdbool.h>
#include <stddef.h>

#include "map.h"
#include "player.h"

typedef struct {
    const char *name;
    PlayerColor color;
    int money;
    int position;
    unsigned long arrival_order;
    bool active;
    bool current;
    bool property_focus;
    int id;
    bool winner;
} TuiPlayerView;

void tui_clear_screen(void);
void tui_render_map(const Map *map);
void tui_render_game(const Map *map, const TuiPlayerView *players, size_t player_count);

#endif
