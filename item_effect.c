#include "item_effect.h"

#include <stddef.h>

static void initialize_report(ItemEffectReport *report, const Player *player, int step)
{
    if (report == NULL) return;
    report->requested_steps = step;
    report->travelled_steps = 0;
    report->final_position = player == NULL ? ITEM_EFFECT_NO_POSITION : player->position;
    report->trigger_position = ITEM_EFFECT_NO_POSITION;
    report->stopped_by_barrier = false;
    report->skip_landing_event = false;
}

static int wrapped_position(int position, int distance)
{
    return (position + distance % MAP_BLOCK_COUNT) % MAP_BLOCK_COUNT;
}

ItemEffectMoveResult Move_Player_With_Item_Effects(
    Player *player, Map *map, int step, ItemEffectReport *report)
{
    int distance;
    int scan_limit;
    int trigger_position = ITEM_EFFECT_NO_POSITION;
    Item_Type trigger = NO_ITEM;
    PlayerMoveResult movement;

    initialize_report(report, player, step);
    if (player == NULL || map == NULL) return ITEM_EFFECT_MOVE_INVALID_ARGUMENT;
    if (player->position < 0 || player->position >= MAP_BLOCK_COUNT) {
        return ITEM_EFFECT_MOVE_INVALID_POSITION;
    }
    if (step < 0) return ITEM_EFFECT_MOVE_INVALID_STEP;

    scan_limit = step % MAP_BLOCK_COUNT;
    for (distance = 1; distance <= scan_limit; ++distance) {
        int position = wrapped_position(player->position, distance);
        BlockBits block = map_get_block(map, (size_t)position);
        if (block_has_flag(block, HAS_OBSTACLE)) trigger = HAS_OBSTACLE;
        if (trigger != NO_ITEM) {
            trigger_position = position;
            break;
        }
    }

    movement = Move_Player(player, trigger == NO_ITEM ? scan_limit : distance);
    if (movement == PLAYER_MOVE_INVALID_ARGUMENT) return ITEM_EFFECT_MOVE_INVALID_ARGUMENT;
    if (movement == PLAYER_MOVE_INVALID_POSITION) return ITEM_EFFECT_MOVE_INVALID_POSITION;
    if (movement == PLAYER_MOVE_INVALID_STEP) return ITEM_EFFECT_MOVE_INVALID_STEP;
    if (movement != PLAYER_MOVE_OK) return ITEM_EFFECT_MOVE_FAILED;

    if (report != NULL) {
        report->travelled_steps = trigger == NO_ITEM ? scan_limit : distance;
        report->final_position = player->position;
        report->trigger_position = trigger_position;
    }
    if (trigger == HAS_OBSTACLE) {
        map_set_item(map, (size_t)trigger_position, NO_ITEM);
        if (report != NULL) report->stopped_by_barrier = true;
        return ITEM_EFFECT_MOVE_STOPPED_BY_BARRIER;
    }
    return ITEM_EFFECT_MOVE_OK;
}
