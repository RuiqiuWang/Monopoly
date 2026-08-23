#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_engine.h"

#define MAX_JSON_TEXT (1 << 20)

static void skip_ws(const char **p)
{
    while (**p && isspace((unsigned char)**p)) {
        ++(*p);
    }
}

static int parse_json_string(const char **p, char *out, size_t out_size)
{
    size_t len = 0;
    if (**p != '"') {
        return 0;
    }
    ++(*p);
    while (**p && **p != '"') {
        char ch = *(*p)++;
        if (ch == '\\') {
            char esc = *(*p)++;
            switch (esc) {
            case '"': ch = '"'; break;
            case '\\': ch = '\\'; break;
            case '/': ch = '/'; break;
            case 'b': ch = '\b'; break;
            case 'f': ch = '\f'; break;
            case 'n': ch = '\n'; break;
            case 'r': ch = '\r'; break;
            case 't': ch = '\t'; break;
            default: return 0;
            }
        }
        if (len + 1 >= out_size) {
            return 0;
        }
        out[len++] = ch;
    }
    if (**p != '"') {
        return 0;
    }
    ++(*p);
    out[len] = '\0';
    return 1;
}

static int parse_json_int(const char **p, int *out)
{
    char *end = NULL;
    long value;
    if (!(**p == '-' || isdigit((unsigned char)**p))) {
        return 0;
    }
    value = strtol(*p, &end, 10);
    if (end == *p || value < -2147483648L || value > 2147483647L) {
        return 0;
    }
    *out = (int)value;
    *p = end;
    return 1;
}

static int parse_literal(const char **p, const char *lit)
{
    size_t n = strlen(lit);
    if (strncmp(*p, lit, n) != 0) {
        return 0;
    }
    *p += n;
    return 1;
}

static int skip_json_value(const char **p);
static int consume_char(const char **p, char ch);

static int skip_json_array(const char **p)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!skip_json_value(p)) {
            return 0;
        }
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int skip_json_object(const char **p)
{
    if (!consume_char(p, '{')) {
        return 0;
    }
    for (;;) {
        char key[128];
        skip_ws(p);
        if (**p == '}') {
            ++(*p);
            return 1;
        }
        if (!parse_json_string(p, key, sizeof(key))) {
            return 0;
        }
        if (!consume_char(p, ':')) {
            return 0;
        }
        if (!skip_json_value(p)) {
            return 0;
        }
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == '}') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int skip_json_value(const char **p)
{
    char tmp[256];
    int number;

    skip_ws(p);
    if (**p == '{') {
        return skip_json_object(p);
    }
    if (**p == '[') {
        return skip_json_array(p);
    }
    if (**p == '"') {
        return parse_json_string(p, tmp, sizeof(tmp));
    }
    if (**p == '-' || isdigit((unsigned char)**p)) {
        return parse_json_int(p, &number);
    }
    if (parse_literal(p, "true") || parse_literal(p, "false") || parse_literal(p, "null")) {
        return 1;
    }
    return 0;
}

static int consume_char(const char **p, char ch)
{
    skip_ws(p);
    if (**p != ch) {
        return 0;
    }
    ++(*p);
    return 1;
}

static int parse_items_object(const char **p, int items[3])
{
    memset(items, 0, sizeof(int) * 3);
    if (!consume_char(p, '{')) {
        return 0;
    }
    for (;;) {
        char key[32];
        skip_ws(p);
        if (**p == '}') {
            ++(*p);
            return 1;
        }
        if (!parse_json_string(p, key, sizeof(key))) {
            return 0;
        }
        if (!consume_char(p, ':')) {
            return 0;
        }
        skip_ws(p);
        int value;
        if (!parse_json_int(p, &value)) {
            return 0;
        }
        if (strcmp(key, "BLOCK") == 0) {
            items[0] = value;
        } else if (strcmp(key, "BOMB") == 0) {
            items[1] = value;
        } else if (strcmp(key, "ROBOT") == 0) {
            items[2] = value;
        }
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == '}') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_players_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->player_count = 0;
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!consume_char(p, '{')) {
            return 0;
        }
        JsonPlayer player;
        memset(&player, 0, sizeof(player));
        for (;;) {
            char key[64];
            skip_ws(p);
            if (**p == '}') {
                ++(*p);
                break;
            }
            if (!parse_json_string(p, key, sizeof(key))) {
                return 0;
            }
            if (!consume_char(p, ':')) {
                return 0;
            }
            skip_ws(p);
            if (strcmp(key, "id") == 0) {
                char idbuf[8];
                if (!parse_json_string(p, idbuf, sizeof(idbuf))) return 0;
                player.id = idbuf[0];
            } else if (strcmp(key, "fund") == 0) {
                if (!parse_json_int(p, &player.fund)) return 0;
            } else if (strcmp(key, "credit") == 0) {
                if (!parse_json_int(p, &player.credit)) return 0;
            } else if (strcmp(key, "position") == 0) {
                if (!parse_json_int(p, &player.position)) return 0;
            } else if (strcmp(key, "status") == 0) {
                if (!parse_json_string(p, player.status, sizeof(player.status))) return 0;
            } else if (strcmp(key, "remaining_rounds") == 0) {
                if (!parse_json_int(p, &player.remaining_rounds)) return 0;
            } else if (strcmp(key, "items") == 0) {
                if (!parse_items_object(p, player.items)) return 0;
            } else if (strcmp(key, "god_of_wealth_rounds") == 0) {
                if (!parse_json_int(p, &player.god_of_wealth_rounds)) return 0;
            } else {
                return 0;
            }
            skip_ws(p);
            if (**p == ',') {
                ++(*p);
                continue;
            }
            if (**p == '}') {
                ++(*p);
                break;
            }
            return 0;
        }
        if (tc->player_count >= JSON_ENGINE_MAX_USERS) {
            return 0;
        }
        tc->players[tc->player_count++] = player;
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_users_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->user_count = 0;
    for (;;) {
        char user[8];
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!parse_json_string(p, user, sizeof(user))) {
            return 0;
        }
        if (tc->user_count >= JSON_ENGINE_MAX_USERS) {
            return 0;
        }
        tc->users[tc->user_count++] = user[0];
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_properties_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->property_count = 0;
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!consume_char(p, '{')) {
            return 0;
        }
        JsonProperty prop;
        memset(&prop, 0, sizeof(prop));
        for (;;) {
            char key[64];
            skip_ws(p);
            if (**p == '}') {
                ++(*p);
                break;
            }
            if (!parse_json_string(p, key, sizeof(key))) return 0;
            if (!consume_char(p, ':')) return 0;
            skip_ws(p);
            if (strcmp(key, "position") == 0) {
                if (!parse_json_int(p, &prop.position)) return 0;
            } else if (strcmp(key, "owner") == 0) {
                char owner[8];
                if (!parse_json_string(p, owner, sizeof(owner))) return 0;
                prop.owner = owner[0];
            } else if (strcmp(key, "level") == 0) {
                if (!parse_json_int(p, &prop.level)) return 0;
            } else {
                return 0;
            }
            skip_ws(p);
            if (**p == ',') {
                ++(*p);
                continue;
            }
            if (**p == '}') {
                ++(*p);
                break;
            }
            return 0;
        }
        if (tc->property_count >= JSON_ENGINE_MAX_PROPERTIES) return 0;
        tc->properties[tc->property_count++] = prop;
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_map_items_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->map_item_count = 0;
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!consume_char(p, '{')) {
            return 0;
        }
        JsonMapItem item;
        memset(&item, 0, sizeof(item));
        for (;;) {
            char key[64];
            skip_ws(p);
            if (**p == '}') {
                ++(*p);
                break;
            }
            if (!parse_json_string(p, key, sizeof(key))) return 0;
            if (!consume_char(p, ':')) return 0;
            skip_ws(p);
            if (strcmp(key, "position") == 0) {
                if (!parse_json_int(p, &item.position)) return 0;
            } else if (strcmp(key, "type") == 0) {
                if (!parse_json_string(p, item.type, sizeof(item.type))) return 0;
            } else {
                return 0;
            }
            skip_ws(p);
            if (**p == ',') {
                ++(*p);
                continue;
            }
            if (**p == '}') {
                ++(*p);
                break;
            }
            return 0;
        }
        if (tc->map_item_count >= JSON_ENGINE_MAX_MAP_ITEMS) return 0;
        tc->map_items[tc->map_item_count++] = item;
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_dice_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->dice_count = 0;
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        int value;
        if (!parse_json_int(p, &value)) return 0;
        if (tc->dice_count >= JSON_ENGINE_MAX_DICE) return 0;
        tc->dice_sequence[tc->dice_count++] = value;
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_actions_array(const char **p, JsonCase *tc)
{
    if (!consume_char(p, '[')) {
        return 0;
    }
    tc->action_count = 0;
    for (;;) {
        skip_ws(p);
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        if (!consume_char(p, '{')) {
            return 0;
        }
        JsonAction action;
        memset(&action, 0, sizeof(action));
        for (;;) {
            char key[64];
            skip_ws(p);
            if (**p == '}') {
                ++(*p);
                break;
            }
            if (!parse_json_string(p, key, sizeof(key))) return 0;
            if (!consume_char(p, ':')) return 0;
            skip_ws(p);
            if (strcmp(key, "command") == 0) {
                if (!parse_json_string(p, action.command, sizeof(action.command))) return 0;
            } else if (strcmp(key, "params") == 0) {
                if (!consume_char(p, '{')) return 0;
                for (;;) {
                    char pkey[32];
                    skip_ws(p);
                    if (**p == '}') {
                        ++(*p);
                        break;
                    }
                    if (!parse_json_string(p, pkey, sizeof(pkey))) return 0;
                    if (!consume_char(p, ':')) return 0;
                    skip_ws(p);
                    if (strcmp(pkey, "steps") == 0) {
                        if (!parse_json_int(p, &action.steps)) return 0;
                    } else if (strcmp(pkey, "offset") == 0) {
                        if (!parse_json_int(p, &action.offset)) return 0;
                    } else if (strcmp(pkey, "position") == 0) {
                        if (!parse_json_int(p, &action.position)) return 0;
                    } else if (strcmp(pkey, "value") == 0) {
                        if (!parse_json_string(p, action.value, sizeof(action.value))) return 0;
                    } else {
                        return 0;
                    }
                    skip_ws(p);
                    if (**p == ',') {
                        ++(*p);
                        continue;
                    }
                    if (**p == '}') {
                        ++(*p);
                        break;
                    }
                    return 0;
                }
            } else {
                return 0;
            }
            skip_ws(p);
            if (**p == ',') {
                ++(*p);
                continue;
            }
            if (**p == '}') {
                ++(*p);
                break;
            }
            return 0;
        }
        if (tc->action_count >= JSON_ENGINE_MAX_ACTIONS) return 0;
        tc->actions[tc->action_count++] = action;
        skip_ws(p);
        if (**p == ',') {
            ++(*p);
            continue;
        }
        if (**p == ']') {
            ++(*p);
            return 1;
        }
        return 0;
    }
}

static int parse_case_object(const char *text, JsonCase *tc)
{
    const char *p = text;
    memset(tc, 0, sizeof(*tc));
    skip_ws(&p);
    if (!consume_char(&p, '{')) {
        return 0;
    }
    for (;;) {
        char key[64];
        skip_ws(&p);
        if (*p == '}') {
            ++p;
            break;
        }
        if (!parse_json_string(&p, key, sizeof(key))) return 0;
        if (!consume_char(&p, ':')) return 0;
        skip_ws(&p);
        if (strcmp(key, "schema_version") == 0) {
            if (!parse_json_string(&p, tc->case_id, sizeof(tc->case_id))) return 0;
        } else if (strcmp(key, "case_id") == 0) {
            if (!parse_json_string(&p, tc->case_id, sizeof(tc->case_id))) return 0;
        } else if (strcmp(key, "case_name") == 0) {
            if (!parse_json_string(&p, tc->case_name, sizeof(tc->case_name))) return 0;
        } else if (strcmp(key, "map_file") == 0) {
            char dummy[64];
            if (!parse_json_string(&p, dummy, sizeof(dummy))) return 0;
        } else if (strcmp(key, "preset") == 0) {
            if (!consume_char(&p, '{')) return 0;
            for (;;) {
                char pkey[64];
                skip_ws(&p);
                if (*p == '}') {
                    ++p;
                    break;
                }
                if (!parse_json_string(&p, pkey, sizeof(pkey))) return 0;
                if (!consume_char(&p, ':')) return 0;
                skip_ws(&p);
                if (strcmp(pkey, "users") == 0) {
                    if (!parse_users_array(&p, tc)) return 0;
                } else if (strcmp(pkey, "current_user") == 0) {
                    char user[8];
                    if (!parse_json_string(&p, user, sizeof(user))) return 0;
                    tc->current_user = user[0];
                } else if (strcmp(pkey, "phase") == 0) {
                    if (!parse_json_string(&p, tc->phase, sizeof(tc->phase))) return 0;
                } else if (strcmp(pkey, "game_status") == 0) {
                    if (!parse_json_string(&p, tc->game_status, sizeof(tc->game_status))) return 0;
                } else if (strcmp(pkey, "players") == 0) {
                    if (!parse_players_array(&p, tc)) return 0;
                } else if (strcmp(pkey, "properties") == 0) {
                    if (!parse_properties_array(&p, tc)) return 0;
                } else if (strcmp(pkey, "map_items") == 0) {
                    if (!parse_map_items_array(&p, tc)) return 0;
                } else if (strcmp(pkey, "dice_sequence") == 0) {
                    if (!parse_dice_array(&p, tc)) return 0;
                } else {
                    return 0;
                }
                skip_ws(&p);
                if (*p == ',') {
                    ++p;
                    continue;
                }
                if (*p == '}') {
                    ++p;
                    break;
                }
                return 0;
            }
        } else if (strcmp(key, "actions") == 0) {
            if (!parse_actions_array(&p, tc)) return 0;
        } else if (strcmp(key, "expected") == 0) {
            if (!skip_json_value(&p)) return 0;
        } else {
            return 0;
        }
        skip_ws(&p);
        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p == '}') {
            ++p;
            break;
        }
        return 0;
    }
    return 1;
}

static int execute_case(const char *input_path, const char *output_path)
{
    FILE *input = fopen(input_path, "rb");
    FILE *output;
    char *text;
    long file_size;
    JsonCase tc;

    if (input == NULL) {
        return 1;
    }
    if (fseek(input, 0, SEEK_END) != 0) {
        fclose(input);
        return 1;
    }
    file_size = ftell(input);
    if (file_size <= 0 || file_size > MAX_JSON_TEXT ||
        fseek(input, 0, SEEK_SET) != 0) {
        fclose(input);
        return 1;
    }
    text = (char *)malloc((size_t)file_size + 1);
    if (text == NULL ||
        fread(text, 1, (size_t)file_size, input) != (size_t)file_size) {
        free(text);
        fclose(input);
        return 1;
    }
    text[file_size] = '\0';
    fclose(input);

    if (!parse_case_object(text, &tc)) {
        free(text);
        return 1;
    }
    free(text);

    json_case_apply_actions(&tc);

    output = fopen(output_path, "wb");
    if (output == NULL) {
        return 1;
    }
    if (!json_case_write_actual(output, &tc)) {
        fclose(output);
        return 1;
    }
    return fclose(output) == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <input.json> <output.json>\n", argv[0]);
        return 1;
    }
    return execute_case(argv[1], argv[2]);
}
