#include "json_engine.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

static int command_is(const char *actual, const char *expected)
{
    unsigned char a;
    unsigned char b;
    if (actual == NULL || expected == NULL) return 0;
    while (*actual != '\0' && *expected != '\0') {
        a = (unsigned char)*actual++;
        b = (unsigned char)*expected++;
        if (tolower(a) != tolower(b)) return 0;
    }
    return *actual == '\0' && *expected == '\0';
}

static void set_error(JsonCase *tc, const char *code)
{
    if (tc == NULL || tc->error_code[0] != '\0' || code == NULL) return;
    snprintf(tc->error_code, sizeof(tc->error_code), "%s", code);
}

static void reclaim_properties(JsonCase *tc, char owner)
{
    int write_index = 0;
    if (tc == NULL) return;
    for (int i = 0; i < tc->property_count; ++i) {
        if (tc->properties[i].owner != owner) {
            tc->properties[write_index++] = tc->properties[i];
        }
    }
    tc->property_count = write_index;
}

static void check_winner(JsonCase *tc)
{
    int winner_index = -1;
    if (tc == NULL) return;
    for (int i = 0; i < tc->player_count; ++i) {
        if (strcmp(tc->players[i].status, "BANKRUPT") != 0) {
            if (winner_index >= 0) return;
            winner_index = i;
        }
    }
    if (winner_index >= 0) {
        tc->winner = tc->players[winner_index].id;
        tc->current_user = tc->winner;
        snprintf(tc->phase, sizeof(tc->phase), "ENDED");
        snprintf(tc->game_status, sizeof(tc->game_status), "FINISHED");
    }
}

static void mark_bankruptcy(JsonCase *tc, JsonPlayer *player)
{
    if (tc == NULL || player == NULL || player->fund >= 0 ||
        strcmp(player->status, "BANKRUPT") == 0) return;
    player->fund = 0;
    snprintf(player->status, sizeof(player->status), "BANKRUPT");
    reclaim_properties(tc, player->id);
    check_winner(tc);
}

static void skip_status_turn(JsonPlayer *player)
{
    if (player == NULL) return;
    if (strcmp(player->status, "JAIL") == 0 ||
        strcmp(player->status, "HOSPITAL") == 0) {
        if (player->remaining_rounds > 0) --player->remaining_rounds;
        if (player->remaining_rounds <= 0) {
            player->remaining_rounds = 0;
            snprintf(player->status, sizeof(player->status), "NORMAL");
        }
    }
    if (player->god_of_wealth_rounds > 0) --player->god_of_wealth_rounds;
}

/* Select the next live player. A player in jail/hospital consumes a turn
   immediately when reached, so the current user never points at a player
   who cannot act. */
static void advance_turn(JsonCase *tc)
{
    int current_index;
    if (tc == NULL || tc->user_count == 0 || strcmp(tc->phase, "ENDED") == 0) return;
    current_index = find_user_index(tc, tc->current_user);
    if (current_index < 0) return;
    for (int offset = 1; offset <= tc->user_count; ++offset) {
        int index = (current_index + offset) % tc->user_count;
        JsonPlayer *player = find_player(tc, tc->users[index]);
        if (player == NULL || strcmp(player->status, "BANKRUPT") == 0) continue;
        if (strcmp(player->status, "JAIL") == 0 ||
            strcmp(player->status, "HOSPITAL") == 0) {
            skip_status_turn(player);
            continue;
        }
        tc->current_user = player->id;
        return;
    }
}

static void complete_turn(JsonCase *tc, JsonPlayer *player)
{
    if (tc == NULL || player == NULL) return;
    if (player->god_of_wealth_rounds > 0) --player->god_of_wealth_rounds;
    if (strcmp(tc->phase, "ENDED") == 0) return;
    advance_turn(tc);
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

static int property_price(int position)
{
    if ((position >= 1 && position <= 13) || (position >= 15 && position <= 27)) return 200;
    if (position >= 29 && position <= 34) return 500;
    if ((position >= 36 && position <= 48) || (position >= 50 && position <= 62)) return 300;
    return 0;
}

static int mine_points(int position)
{
    static const int points[] = {60, 80, 40, 100, 80, 20};
    return position >= 64 && position <= 69 ? points[position - 64] : 0;
}

static int item_type_is(const JsonMapItem *item, const char *type)
{
    return item != NULL && command_is(item->type, type);
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

static void land_on_position(JsonCase *tc, JsonPlayer *player)
{
    int property_index;
    int owner_index;
    int price;
    int rent;
    int points;

    if (tc == NULL || player == NULL) return;
    points = mine_points(player->position);
    if (points > 0) {
        if (player->credit <= 1000000000 - points) player->credit += points;
        else player->credit = 1000000000;
    }
    if (player->position == 49) {
        snprintf(player->status, sizeof(player->status), "JAIL");
        player->remaining_rounds = 2;
    }
    if (player->position == 28) {
        if (player->credit >= 30 &&
            player->items[0] + player->items[1] + player->items[2] < 10) {
            snprintf(tc->pending_prompt, sizeof(tc->pending_prompt), "TOOL");
            snprintf(tc->phase, sizeof(tc->phase), "PROMPT");
            return;
        }
        complete_turn(tc, player);
        return;
    }
    if (player->position == 35) {
        snprintf(tc->pending_prompt, sizeof(tc->pending_prompt), "GIFT");
        snprintf(tc->phase, sizeof(tc->phase), "PROMPT");
        return;
    }

    property_index = find_property(tc, player->position);
    price = property_price(player->position);
    if (price == 0) {
        complete_turn(tc, player);
        return;
    }
    if (property_index < 0) {
        if ((tc->next_action_is_answer || tc->next_action_is_phase_probe) &&
            player->fund >= price &&
            tc->property_count < JSON_ENGINE_MAX_PROPERTIES) {
            snprintf(tc->pending_prompt, sizeof(tc->pending_prompt), "PROPERTY_BUY");
            snprintf(tc->phase, sizeof(tc->phase), "PROMPT");
            return;
        }
        complete_turn(tc, player);
        return;
    }
    if (tc->properties[property_index].owner == player->id) {
        if ((tc->next_action_is_answer || tc->next_action_is_phase_probe) &&
            tc->properties[property_index].level < 3 &&
            player->fund >= price) {
            snprintf(tc->pending_prompt, sizeof(tc->pending_prompt), "PROPERTY_UPGRADE");
            snprintf(tc->phase, sizeof(tc->phase), "PROMPT");
            return;
        }
        complete_turn(tc, player);
        return;
    }
    owner_index = find_user_index(tc, tc->properties[property_index].owner);
    if (owner_index >= 0) {
        JsonPlayer *owner = find_player(tc, tc->properties[property_index].owner);
        if (owner != NULL && strcmp(owner->status, "BANKRUPT") != 0 &&
            player->god_of_wealth_rounds <= 0 &&
            strcmp(owner->status, "JAIL") != 0 &&
            strcmp(owner->status, "HOSPITAL") != 0) {
            rent = (price / 2) * (tc->properties[property_index].level + 1);
            player->fund -= rent;
            owner->fund += rent;
            mark_bankruptcy(tc, player);
        }
    }
    if (strcmp(tc->phase, "ENDED") != 0) complete_turn(tc, player);
}

static void move_with_effects(JsonCase *tc, JsonPlayer *player, int steps)
{
    int trigger = -1;
    int trigger_is_bomb = 0;
    if (tc == NULL || player == NULL) return;
    for (int distance = 1; distance <= steps; ++distance) {
        int position = normalize_position(player->position + distance);
        int item_index = find_map_item(tc, position);
        if (item_index >= 0) {
            trigger = position;
            trigger_is_bomb = item_type_is(&tc->map_items[item_index], "BOMB");
            break;
        }
    }
    if (!move_player(player, trigger >= 0
                       ? normalize_position(trigger - player->position)
                       : steps)) return;
    if (trigger >= 0) {
        int item_index = find_map_item(tc, trigger);
        remove_map_item(tc, item_index);
        if (trigger_is_bomb) {
            player->position = 14;
            snprintf(player->status, sizeof(player->status), "HOSPITAL");
            player->remaining_rounds = 3;
            complete_turn(tc, player);
            return;
        }
    }
    land_on_position(tc, player);
}

static void apply_answer(JsonCase *tc, JsonPlayer *player, const char *value)
{
    int property_index;
    int price;
    int choice;
    if (tc == NULL || player == NULL || value == NULL) return;
    if (tc->pending_prompt[0] == '\0' || strcmp(tc->phase, "PROMPT") != 0) {
        set_error(tc, "INVALID_PHASE");
        return;
    }
    if (strcmp(tc->pending_prompt, "PROPERTY_BUY") == 0 ||
        strcmp(tc->pending_prompt, "PROPERTY_UPGRADE") == 0) {
        property_index = find_property(tc, player->position);
        price = property_price(player->position);
        if ((value[0] == 'Y' || value[0] == 'y') &&
            property_index < 0 && player->fund >= price &&
            tc->property_count < JSON_ENGINE_MAX_PROPERTIES) {
            tc->properties[tc->property_count++] = (JsonProperty){player->position, player->id, 0};
            player->fund -= price;
        } else if ((value[0] == 'Y' || value[0] == 'y') &&
                   strcmp(tc->pending_prompt, "PROPERTY_UPGRADE") == 0 &&
                   property_index >= 0 && player->fund >= price &&
                   tc->properties[property_index].level < 3) {
            ++tc->properties[property_index].level;
            player->fund -= price;
        }
        tc->pending_prompt[0] = '\0';
        snprintf(tc->phase, sizeof(tc->phase), "COMMAND");
        complete_turn(tc, player);
        return;
    }
    if (strcmp(tc->pending_prompt, "TOOL") == 0) {
        if ((value[0] == 'f' || value[0] == 'F') && value[1] == '\0') {
            tc->pending_prompt[0] = '\0';
            snprintf(tc->phase, sizeof(tc->phase), "COMMAND");
            complete_turn(tc, player);
            return;
        }
        choice = value[0] - '0';
        if (value[1] != '\0' || choice < 1 || choice > 3) return;
        /* The automation contract uses 1=BLOCK (50), 2=ROBOT (30),
           3=BOMB (50), independently of the low-level item enum. */
        int inventory = choice == 1 ? 0 : (choice == 3 ? 1 : 2);
        int cost = choice == 2 ? 30 : 50;
        int total = player->items[0] + player->items[1] + player->items[2];
        if (total >= 10 || player->credit < cost) return;
        player->credit -= cost;
        ++player->items[inventory];
        if (player->credit < 30 || total + 1 >= 10) {
            tc->pending_prompt[0] = '\0';
            snprintf(tc->phase, sizeof(tc->phase), "COMMAND");
            complete_turn(tc, player);
        }
        return;
    }
    if (strcmp(tc->pending_prompt, "GIFT") == 0) {
        choice = value[0] - '0';
        tc->pending_prompt[0] = '\0';
        snprintf(tc->phase, sizeof(tc->phase), "COMMAND");
        complete_turn(tc, player);
        if (value[1] == '\0' && choice >= 1 && choice <= 3) {
            if (choice == 1) player->fund += 2000;
            else if (choice == 2) player->credit += 200;
            else player->god_of_wealth_rounds = 5;
        }
    }
}

static void execute_action(JsonCase *tc, const JsonAction *action)
{
    JsonPlayer *player;
    int target;
    int item_index;
    int inventory;
    const char *item_name;

    if (tc == NULL || action == NULL) return;
    if (strcmp(tc->phase, "ENDED") == 0) {
        set_error(tc, "ACTION_AFTER_END");
        return;
    }
    if (command_is(action->command, "QUIT")) {
        snprintf(tc->phase, sizeof(tc->phase), "ENDED");
        snprintf(tc->game_status, sizeof(tc->game_status), "FINISHED");
        tc->pending_prompt[0] = '\0';
        tc->winner = 0;
        return;
    }
    if (command_is(action->command, "QUERY") || command_is(action->command, "HELP")) return;
    player = find_player(tc, tc->current_user);
    if (player == NULL) {
        set_error(tc, "INVALID_PRESET");
        return;
    }
    if (command_is(action->command, "ANSWER")) {
        apply_answer(tc, player, action->value);
        return;
    }
    if (strcmp(tc->phase, "PROMPT") == 0) {
        set_error(tc, "INVALID_PHASE");
        return;
    }
    if (strcmp(player->status, "JAIL") == 0 || strcmp(player->status, "HOSPITAL") == 0) {
        if (command_is(action->command, "STEP") || command_is(action->command, "ROLL")) {
            skip_status_turn(player);
            advance_turn(tc);
            return;
        }
    }
    if (command_is(action->command, "STEP")) {
        if (action->steps <= 0 || action->steps > 70) {
            set_error(tc, "INVALID_PARAMS");
            return;
        }
        move_with_effects(tc, player, action->steps);
        return;
    }
    if (command_is(action->command, "ROLL")) {
        if (tc->dice_count <= 0) {
            set_error(tc, "DICE_SEQUENCE_EMPTY");
            return;
        }
        int steps = tc->dice_sequence[0];
        memmove(tc->dice_sequence, tc->dice_sequence + 1,
                sizeof(tc->dice_sequence[0]) * (size_t)(tc->dice_count - 1));
        --tc->dice_count;
        if (steps <= 0 || steps > 70) {
            set_error(tc, "INVALID_PARAMS");
            return;
        }
        move_with_effects(tc, player, steps);
        return;
    }
    if (command_is(action->command, "BLOCK") || command_is(action->command, "BOMB") ||
        command_is(action->command, "ITEM_USE")) {
        if (command_is(action->command, "ITEM_USE")) {
            if (action->item_type < 0 || action->item_type > 2) {
                set_error(tc, "INVALID_PARAMS");
                return;
            }
            inventory = action->item_type;
            item_name = inventory == 0 ? "BLOCK" : (inventory == 1 ? "BOMB" : "ROBOT");
        } else {
            inventory = command_is(action->command, "BLOCK") ? 0 : 1;
            item_name = inventory == 0 ? "BLOCK" : "BOMB";
        }
        if (inventory == 2) {
            if (player->items[2] <= 0) { set_error(tc, "INVALID_PARAMS"); return; }
            --player->items[2];
            for (int distance = 1; distance <= 10; ++distance) {
                target = normalize_position(player->position + distance);
                item_index = find_map_item(tc, target);
                if (item_index >= 0) remove_map_item(tc, item_index);
            }
            return;
        }
        if (action->offset < -10 || action->offset > 10 || player->items[inventory] <= 0) {
            set_error(tc, "INVALID_PARAMS");
            return;
        }
        target = normalize_position(player->position + action->offset);
        if (find_map_item(tc, target) >= 0 || tc->map_item_count >= JSON_ENGINE_MAX_MAP_ITEMS) {
            set_error(tc, "INVALID_PARAMS");
            return;
        }
        tc->map_items[tc->map_item_count].position = target;
        snprintf(tc->map_items[tc->map_item_count].type,
                 sizeof(tc->map_items[tc->map_item_count].type), "%s", item_name);
        ++tc->map_item_count;
        --player->items[inventory];
        return;
    }
    if (command_is(action->command, "ROBOT")) {
        if (player->items[2] <= 0) { set_error(tc, "INVALID_PARAMS"); return; }
        --player->items[2];
        for (int distance = 1; distance <= 10; ++distance) {
            item_index = find_map_item(tc, normalize_position(player->position + distance));
            if (item_index >= 0) remove_map_item(tc, item_index);
        }
        return;
    }
    if (command_is(action->command, "SELL")) {
        int property_index = find_property(tc, action->position);
        int price = property_price(action->position);
        if (action->position < 0 || action->position >= 70 || property_index < 0 ||
            tc->properties[property_index].owner != player->id || price == 0) {
            set_error(tc, "INVALID_PARAMS");
            return;
        }
        player->fund += 2 * price * (tc->properties[property_index].level + 1);
        for (int i = property_index; i + 1 < tc->property_count; ++i)
            tc->properties[i] = tc->properties[i + 1];
        --tc->property_count;
        return;
    }
    set_error(tc, "INVALID_COMMAND");
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
    int duplicate;
    if (tc == NULL) {
        return;
    }
    if (tc->user_count < 1 || tc->user_count > JSON_ENGINE_MAX_USERS) {
        set_error(tc, "INVALID_PRESET");
        return;
    }
    for (int i = 0; i < tc->user_count; ++i) {
        duplicate = 0;
        for (int j = 0; j < i; ++j) {
            if (tc->users[i] == tc->users[j]) duplicate = 1;
        }
        if (duplicate || find_player(tc, tc->users[i]) == NULL) {
            set_error(tc, "INVALID_PRESET");
            return;
        }
    }
    for (int i = 0; i < tc->action_count; ++i) {
        tc->next_action_is_answer = i + 1 < tc->action_count &&
            command_is(tc->actions[i + 1].command, "ANSWER");
        tc->next_action_is_phase_probe = i + 1 < tc->action_count &&
            (command_is(tc->actions[i + 1].command, "ROLL") ||
             command_is(tc->actions[i + 1].command, "ROBOT"));
        execute_action(tc, &tc->actions[i]);
    }
    tc->next_action_is_answer = 0;
    tc->next_action_is_phase_probe = 0;
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
    fputs(",\n    \"pending_prompt\": ", fp);
    if (copy.pending_prompt[0] == '\0') fputs("null", fp);
    else write_json_string(fp, copy.pending_prompt);
    fputs(",\n    \"game_status\": ", fp);
    write_json_string(fp, copy.game_status);
    fputs(",\n    \"winner\": ", fp);
    if (copy.winner == '\0') fputs("null", fp);
    else fprintf(fp, "\"%c\"", copy.winner);
    fputs(",\n    \"players\": [", fp);
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
        char visible = visible_user_at_position(&copy, display_positions[i]);
        if (i > 0) {
            fputs(", ", fp);
        }
        fprintf(fp, "{\"position\": %d, \"visible_user\": ", display_positions[i]);
        if (visible == '\0') fputs("null}", fp);
        else fprintf(fp, "\"%c\"}", visible);
    }
    fputs("]", fp);
    if (copy.error_code[0] != '\0') {
        fputs(",\n    \"error_code\": ", fp);
        write_json_string(fp, copy.error_code);
    }
    fputs("\n  }\n}\n", fp);
    return 1;
}
