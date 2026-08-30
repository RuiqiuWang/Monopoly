#include "json_engine.h"

#include <stdio.h>
#include <string.h>

#include "movement.h"

static JsonPlayer *find_player(JsonCase *tc, char id)
{
    for (int i = 0; i < tc->player_count; ++i) {
        if (tc->players[i].id == id) {
            return &tc->players[i];
        }
    }
    return NULL;
}

static int find_user_index(const JsonCase *tc, char id)
{
    for (int i = 0; i < tc->user_count; ++i) {
        if (tc->users[i] == id) {
            return i;
        }
    }
    return -1;
}

static void advance_turn(JsonCase *tc)
{
    int current_index = find_user_index(tc, tc->current_user);

    if (current_index < 0 || tc->user_count == 0) {
        return;
    }
    for (int offset = 1; offset <= tc->user_count; ++offset) {
        int index = (current_index + offset) % tc->user_count;
        JsonPlayer *player = find_player(tc, tc->users[index]);
        if (player != NULL && strcmp(player->status, "BANKRUPT") != 0) {
            tc->current_user = tc->users[index];
            return;
        }
    }
}

static int normalize_position(int position)
{
    position %= 70;
    return position < 0 ? position + 70 : position;
}

static int move_player(JsonPlayer *player, int steps)
{
    Player engine_player;

    if (player == NULL || steps <= 0) {
        return 0;
    }
    memset(&engine_player, 0, sizeof(engine_player));
    engine_player.position = player->position;
    if (Move_Player(&engine_player, steps) != PLAYER_MOVE_OK) {
        return 0;
    }
    player->position = engine_player.position;
    return 1;
}

static int find_map_item(const JsonCase *tc, int position)
{
    for (int i = 0; i < tc->map_item_count; ++i) {
        if (tc->map_items[i].position == position) {
            return i;
        }
    }
    return -1;
}

static int find_property(const JsonCase *tc, int position)
{
    for (int i = 0; i < tc->property_count; ++i) {
        if (tc->properties[i].position == position) {
            return i;
        }
    }
    return -1;
}

static void remove_map_item(JsonCase *tc, int index)
{
    if (index < 0 || index >= tc->map_item_count) {
        return;
    }
    for (int i = index; i + 1 < tc->map_item_count; ++i) {
        tc->map_items[i] = tc->map_items[i + 1];
    }
    --tc->map_item_count;
}

static void sort_properties(JsonCase *tc)
{
    for (int i = 0; i < tc->property_count; ++i) {
        for (int j = i + 1; j < tc->property_count; ++j) {
            if (tc->properties[j].position < tc->properties[i].position) {
                JsonProperty tmp = tc->properties[i];
                tc->properties[i] = tc->properties[j];
                tc->properties[j] = tmp;
            }
        }
    }
}

static void sort_map_items(JsonCase *tc)
{
    for (int i = 0; i < tc->map_item_count; ++i) {
        for (int j = i + 1; j < tc->map_item_count; ++j) {
            if (tc->map_items[j].position < tc->map_items[i].position) {
                JsonMapItem tmp = tc->map_items[i];
                tc->map_items[i] = tc->map_items[j];
                tc->map_items[j] = tmp;
            }
        }
    }
}

static void execute_action(JsonCase *tc, const JsonAction *action)
{
    JsonPlayer *player;
    int target;
    int item_index;

    if (tc == NULL || action == NULL || strcmp(tc->phase, "ENDED") == 0) {
        return;
    }

    player = find_player(tc, tc->current_user);
    if (strcmp(action->command, "QUERY") == 0 ||
        strcmp(action->command, "HELP") == 0) {
        return;
    }
    if (strcmp(action->command, "QUIT") == 0) {
        strcpy(tc->phase, "ENDED");
        strcpy(tc->game_status, "FINISHED");
        return;
    }
    if (player == NULL) {
        return;
    }
    if (strcmp(action->command, "STEP") == 0) {
        if (move_player(player, action->steps)) {
            advance_turn(tc);
        }
        return;
    }
    if (strcmp(action->command, "ROLL") == 0) {
        if (tc->dice_count > 0 && move_player(player, tc->dice_sequence[0])) {
            memmove(tc->dice_sequence,
                    tc->dice_sequence + 1,
                    sizeof(tc->dice_sequence[0]) * (size_t)(tc->dice_count - 1));
            --tc->dice_count;
            advance_turn(tc);
        }
        return;
    }
    if (strcmp(action->command, "BLOCK") == 0 ||
        strcmp(action->command, "BOMB") == 0 ||
        (strcmp(action->command, "ITEM_USE") == 0 && action->item_type != 2)) {
        int inventory = strcmp(action->command, "ITEM_USE") == 0
            ? action->item_type
            : (strcmp(action->command, "BLOCK") == 0 ? 0 : 1);
        const char *item_name = inventory == 0 ? "BLOCK" : "BOMB";
        if (inventory < 0 || inventory > 1) {
            return;
        }
        if (action->offset < -10 || action->offset > 10 ||
            player->items[inventory] <= 0) {
            return;
        }
        target = normalize_position(player->position + action->offset);
        if (find_map_item(tc, target) >= 0 ||
            tc->map_item_count >= JSON_ENGINE_MAX_MAP_ITEMS) {
            return;
        }
        tc->map_items[tc->map_item_count].position = target;
        snprintf(tc->map_items[tc->map_item_count].type,
                 sizeof(tc->map_items[tc->map_item_count].type),
                 "%s",
                 item_name);
        ++tc->map_item_count;
        --player->items[inventory];
        return;
    }
    if (strcmp(action->command, "ROBOT") == 0 ||
        (strcmp(action->command, "ITEM_USE") == 0 && action->item_type == 2)) {
        if (player->items[2] <= 0) {
            return;
        }
        --player->items[2];
        for (int distance = 1; distance <= 10; ++distance) {
            target = normalize_position(player->position + distance);
            item_index = find_map_item(tc, target);
            if (item_index >= 0) {
                remove_map_item(tc, item_index);
            }
        }
        return;
    }
    if (strcmp(action->command, "SELL") == 0) {
        int property_index = find_property(tc, action->position);
        if (property_index >= 0 &&
            tc->properties[property_index].owner == player->id) {
            int base_price = action->position >= 29 && action->position <= 34
                                 ? 500
                                 : (action->position >= 36 && action->position <= 63
                                        ? 300
                                        : 200);
            int level = tc->properties[property_index].level;
            player->fund += 2 * (base_price + level * base_price);
            for (int i = property_index; i + 1 < tc->property_count; ++i) {
                tc->properties[i] = tc->properties[i + 1];
            }
            --tc->property_count;
        }
    }
}

static void write_json_string(FILE *fp, const char *value)
{
    fputc('"', fp);
    for (const unsigned char *p = (const unsigned char *)value; *p != '\0'; ++p) {
        switch (*p) {
        case '"': fputs("\\\"", fp); break;
        case '\\': fputs("\\\\", fp); break;
        case '\b': fputs("\\b", fp); break;
        case '\f': fputs("\\f", fp); break;
        case '\n': fputs("\\n", fp); break;
        case '\r': fputs("\\r", fp); break;
        case '\t': fputs("\\t", fp); break;
        default:
            if (*p < 0x20) {
                fprintf(fp, "\\u%04x", (unsigned)*p);
            } else {
                fputc(*p, fp);
            }
            break;
        }
    }
    fputc('"', fp);
}

static char visible_user_at_position(const JsonCase *tc, int position)
{
    JsonPlayer *current = find_player((JsonCase *)tc, tc->current_user);

    if (current != NULL && current->position == position &&
        strcmp(current->status, "BANKRUPT") != 0) {
        return current->id;
    }
    for (int i = 0; i < tc->user_count; ++i) {
        JsonPlayer *player = find_player((JsonCase *)tc, tc->users[i]);
        if (player != NULL && player->position == position &&
            strcmp(player->status, "BANKRUPT") != 0) {
            return player->id;
        }
    }
    return '\0';
}

void json_case_apply_action(JsonCase *tc, const JsonAction *action)
{
    execute_action(tc, action);
}

void json_case_apply_actions(JsonCase *tc)
{
    if (tc == NULL) {
        return;
    }
    for (int i = 0; i < tc->action_count; ++i) {
        execute_action(tc, &tc->actions[i]);
    }
}

int json_case_write_actual(FILE *fp, const JsonCase *tc)
{
    int display_positions[JSON_ENGINE_MAX_USERS];
    int display_count = 0;
    JsonCase copy;

    if (fp == NULL || tc == NULL) {
        return 0;
    }

    copy = *tc;
    sort_properties(&copy);
    sort_map_items(&copy);
    for (int i = 0; i < copy.player_count; ++i) {
        JsonPlayer *player = &copy.players[i];
        int seen = 0;
        if (strcmp(player->status, "BANKRUPT") == 0) {
            continue;
        }
        for (int j = 0; j < display_count; ++j) {
            if (display_positions[j] == player->position) {
                seen = 1;
                break;
            }
        }
        if (!seen && display_count < JSON_ENGINE_MAX_USERS) {
            display_positions[display_count++] = player->position;
        }
    }
    for (int i = 0; i < display_count; ++i) {
        for (int j = i + 1; j < display_count; ++j) {
            if (display_positions[j] < display_positions[i]) {
                int tmp = display_positions[i];
                display_positions[i] = display_positions[j];
                display_positions[j] = tmp;
            }
        }
    }

    fputs("{\n  \"schema_version\": \"1.0\",\n  \"case_id\": ", fp);
    write_json_string(fp, copy.case_id);
    fputs(",\n  \"case_name\": ", fp);
    write_json_string(fp, copy.case_name);
    fputs(",\n  \"actual\": {\n    \"users\": [", fp);
    for (int i = 0; i < copy.user_count; ++i) {
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp, "\"%c\"", copy.users[i]);
    }
    fputs("],\n    \"current_user\": ", fp);
    fprintf(fp, "\"%c\",\n    \"phase\": ", copy.current_user);
    write_json_string(fp, copy.phase);
    fputs(",\n    \"pending_prompt\": null,\n    \"game_status\": ", fp);
    write_json_string(fp, copy.game_status);
    fputs(",\n    \"winner\": null,\n    \"players\": [", fp);
    for (int i = 0; i < copy.user_count; ++i) {
        JsonPlayer *player = find_player(&copy, copy.users[i]);
        if (player == NULL) {
            continue;
        }
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp,
                "\n      {\"id\": \"%c\", \"fund\": %d, \"credit\": %d, "
                "\"position\": %d, \"status\": ",
                player->id,
                player->fund,
                player->credit,
                player->position);
        write_json_string(fp, player->status);
        fprintf(fp,
                ", \"remaining_rounds\": %d, \"items\": {\"BLOCK\": %d, "
                "\"BOMB\": %d, \"ROBOT\": %d}, \"god_of_wealth_rounds\": %d}",
                player->remaining_rounds,
                player->items[0],
                player->items[1],
                player->items[2],
                player->god_of_wealth_rounds);
    }
    fputs("\n    ],\n    \"properties\": [", fp);
    for (int i = 0; i < copy.property_count; ++i) {
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp,
                "{\"position\": %d, \"owner\": \"%c\", \"level\": %d}",
                copy.properties[i].position,
                copy.properties[i].owner,
                copy.properties[i].level);
    }
    fputs("],\n    \"map_items\": [", fp);
    for (int i = 0; i < copy.map_item_count; ++i) {
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp, "{\"position\": %d, \"type\": ", copy.map_items[i].position);
        write_json_string(fp, copy.map_items[i].type);
        fputc('}', fp);
    }
    fputs("],\n    \"display_players\": [", fp);
    for (int i = 0; i < display_count; ++i) {
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp,
                "{\"position\": %d, \"visible_user\": \"%c\"}",
                display_positions[i],
                visible_user_at_position(&copy, display_positions[i]));
    }
    fputs("]\n  }\n}\n", fp);
    return 1;
}
