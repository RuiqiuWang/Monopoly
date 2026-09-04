#include "fortune.h"

#include <stdlib.h>

static bool block_has_item(BlockBits block)
{
    return block_has_any_flag(
        block, (BlockBits)(HAS_OBSTACLE | HAS_FORTUNE));
}

static bool position_has_player(
    const Player players[], size_t player_count, int position)
{
    for (size_t i = 0; players != NULL && i < player_count; ++i) {
        if (players[i].active && players[i].position == position) return true;
    }
    return false;
}

static int random_respawn_delay(void)
{
    return (rand() % 10) + 1;
}

static void schedule_respawn(FortuneState *state, int completed_turn)
{
    state->position = FORTUNE_NO_POSITION;
    state->remaining_map_turns = 0;
    state->next_spawn_after_turn = completed_turn + random_respawn_delay();
}

static bool spawn_fortune(
    FortuneState *state, Map *map,
    const Player players[], size_t player_count)
{
    int candidate;

    do {
        candidate = rand() % MAP_BLOCK_COUNT;
    } while (map_block_is_tool_room(map_get_block(map, (size_t)candidate)) ||
             map_block_is_gift_room(map_get_block(map, (size_t)candidate)) ||
             block_has_item(map_get_block(map, (size_t)candidate)) ||
             position_has_player(players, player_count, candidate));

    state->position = candidate;
    state->remaining_map_turns = FORTUNE_MAP_LIFETIME_TURNS;
    state->next_spawn_after_turn = 0;
    map_set_item(map, (size_t)candidate, HAS_FORTUNE);
    return true;
}

void Fortune_Init(FortuneState *state)
{
    if (state == NULL) return;
    state->position = FORTUNE_NO_POSITION;
    state->remaining_map_turns = 0;
    state->next_spawn_after_turn = FORTUNE_FIRST_SPAWN_TURN;
}

FortuneTurnResult Fortune_Advance_Turn(
    FortuneState *state, Map *map, const Player players[], size_t player_count,
    int completed_turn)
{
    if (state == NULL || map == NULL) return FORTUNE_TURN_NO_CHANGE;

    if (state->position != FORTUNE_NO_POSITION) {
        --state->remaining_map_turns;
        if (state->remaining_map_turns > 0) return FORTUNE_TURN_NO_CHANGE;
        map_set_item(map, (size_t)state->position, NO_ITEM);
        schedule_respawn(state, completed_turn);
        return FORTUNE_TURN_EXPIRED;
    }

    if (state->next_spawn_after_turn == completed_turn &&
        spawn_fortune(state, map, players, player_count)) {
        return FORTUNE_TURN_SPAWNED;
    }
    return FORTUNE_TURN_NO_CHANGE;
}

bool Fortune_Collect_On_Path(
    FortuneState *state, Map *map, Player *player,
    int start_position, int travelled_steps, int current_turn)
{
    if (state == NULL || map == NULL || player == NULL ||
        state->position == FORTUNE_NO_POSITION || travelled_steps <= 0 ||
        start_position < 0 || start_position >= MAP_BLOCK_COUNT) {
        return false;
    }

    for (int distance = 1; distance <= travelled_steps; ++distance) {
        int position = (start_position + distance) % MAP_BLOCK_COUNT;
        if (position != state->position) continue;
        map_set_item(map, (size_t)state->position, NO_ITEM);
        schedule_respawn(state, current_turn);
        player->god_of_wealth_rounds = FORTUNE_PLAYER_EFFECT_ROUNDS;
        return true;
    }
    return false;
}

void Fortune_End_Player_Turn(Player *player, bool acquired_this_turn)
{
    if (player != NULL && player->god_of_wealth_rounds > 0 &&
        !acquired_this_turn) {
        --player->god_of_wealth_rounds;
    }
}
