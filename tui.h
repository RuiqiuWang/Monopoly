#ifndef MONOPOLY_TUI_H
#define MONOPOLY_TUI_H

#include <stdbool.h>
#include <stddef.h>

#include "map.h"

typedef struct {
    const char *name;
    int money;
    int position;
    bool active;
} TuiPlayerView;

void tui_clear_screen(void);
void tui_render_map(const Map *map);
void tui_render_game(const Map *map, const TuiPlayerView *players, size_t player_count);

#endif
