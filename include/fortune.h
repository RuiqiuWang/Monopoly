#ifndef MONOPOLY_FORTUNE_H
#define MONOPOLY_FORTUNE_H

#include <stdbool.h>
#include <stddef.h>

#include "map.h"
#include "player.h"

#define FORTUNE_FIRST_SPAWN_TURN 10
#define FORTUNE_MAP_LIFETIME_TURNS 5
#define FORTUNE_PLAYER_EFFECT_ROUNDS 5
#define FORTUNE_NO_POSITION (-1)

typedef enum {
    FORTUNE_TURN_NO_CHANGE,
    FORTUNE_TURN_SPAWNED,
    FORTUNE_TURN_EXPIRED
} FortuneTurnResult;

typedef struct {
    int position;
    int remaining_map_turns;
    int next_spawn_after_turn;
} FortuneState;

void Fortune_Init(FortuneState *state);
FortuneTurnResult Fortune_Advance_Turn(
    FortuneState *state, Map *map, const Player players[], size_t player_count,
    int completed_turn);
bool Fortune_Collect_On_Path(
    FortuneState *state, Map *map, Player *player,
    int start_position, int travelled_steps, int current_turn);
void Fortune_End_Player_Turn(Player *player, bool acquired_this_turn);

#endif
