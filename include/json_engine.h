#ifndef MONOPOLY_JSON_ENGINE_H
#define MONOPOLY_JSON_ENGINE_H

#include <stdio.h>

#define JSON_ENGINE_MAX_USERS 4
#define JSON_ENGINE_MAX_ACTIONS 64
#define JSON_ENGINE_MAX_PROPERTIES 64
#define JSON_ENGINE_MAX_MAP_ITEMS 64
#define JSON_ENGINE_MAX_DICE 64

typedef struct {
    char id;
    int fund;
    int credit;
    int position;
    char status[16];
    int remaining_rounds;
    int items[3];
    int god_of_wealth_rounds;
} JsonPlayer;

typedef struct {
    int position;
    char owner;
    int level;
} JsonProperty;

typedef struct {
    int position;
    char type[16];
} JsonMapItem;

typedef struct {
    char command[16];
    int steps;
    int offset;
    int position;
    int item_type;
    char value[16];
} JsonAction;

typedef struct {
    char users[JSON_ENGINE_MAX_USERS];
    int user_count;
    char current_user;
    char phase[16];
    char game_status[16];
    JsonPlayer players[JSON_ENGINE_MAX_USERS];
    int player_count;
    JsonProperty properties[JSON_ENGINE_MAX_PROPERTIES];
    int property_count;
    JsonMapItem map_items[JSON_ENGINE_MAX_MAP_ITEMS];
    int map_item_count;
    int dice_sequence[JSON_ENGINE_MAX_DICE];
    int dice_count;
    JsonAction actions[JSON_ENGINE_MAX_ACTIONS];
    int action_count;
    char case_id[64];
    char case_name[128];
} JsonCase;

void json_case_apply_action(JsonCase *tc, const JsonAction *action);
void json_case_apply_actions(JsonCase *tc);
int json_case_write_actual(FILE *fp, const JsonCase *tc);

#endif
