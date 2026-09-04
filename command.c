#include "command.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

static const char *skip_spaces(const char *cursor)
{
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        ++cursor;
    }
    return cursor;
}

static int word_equals(const char *start, size_t length, const char *word)
{
    size_t word_length = strlen(word);
    if (length != word_length) return 0;
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)start[i]) != tolower((unsigned char)word[i])) {
            return 0;
        }
    }
    return 1;
}

static CommandResult parse_integer(const char **cursor, int *value, int positive)
{
    long parsed = 0;
    int sign = 1;

    if (**cursor == '\0') return COMMAND_MISSING_ARGUMENT;
    if (**cursor == '+' || **cursor == '-') {
        if (**cursor == '-') sign = -1;
        ++*cursor;
    }
    if (!isdigit((unsigned char)**cursor)) return COMMAND_INVALID_NUMBER;
    while (isdigit((unsigned char)**cursor)) {
        int digit = **cursor - '0';
        if (parsed > (LONG_MAX - digit) / 10) return COMMAND_INVALID_NUMBER;
        parsed = parsed * 10 + digit;
        ++*cursor;
    }
    parsed *= sign;
    if (parsed < INT_MIN || parsed > INT_MAX) return COMMAND_INVALID_NUMBER;
    if (positive && parsed <= 0) return COMMAND_OUT_OF_RANGE;
    *value = (int)parsed;
    return COMMAND_OK;
}

static CommandResult parse_one_argument(const char **cursor, int *argument)
{
    CommandResult result;
    *cursor = skip_spaces(*cursor);
    result = parse_integer(cursor, argument, 0);
    if (result != COMMAND_OK) return result;
    if (**cursor != '\0' && !isspace((unsigned char)**cursor)) {
        return COMMAND_EXTRA_ARGUMENT;
    }
    if (*skip_spaces(*cursor) != '\0') return COMMAND_EXTRA_ARGUMENT;
    return COMMAND_OK;
}

CommandResult Parse_Command(const char *input, Command *command)
{
    const char *cursor;
    const char *word_start;
    size_t word_length;
    CommandResult result;

    if (input == NULL || command == NULL) return COMMAND_INVALID_ARGUMENT;
    command->type = COMMAND_INVALID;
    command->player_id = 0;
    command->steps = 0;
    command->argument = 0;

    cursor = skip_spaces(input);
    word_start = cursor;
    while (*cursor != '\0' && !isspace((unsigned char)*cursor)) ++cursor;
    word_length = (size_t)(cursor - word_start);
    if (word_length == 0) return COMMAND_UNKNOWN;

    if (word_equals(word_start, word_length, "q")) {
        command->type = COMMAND_QUIT;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "help")) {
        command->type = COMMAND_HELP;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "reset")) {
        command->type = COMMAND_RESET;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "roll")) {
        command->type = COMMAND_ROLL;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "query")) {
        command->type = COMMAND_QUERY;
        cursor = skip_spaces(cursor);
        if (*cursor == '\0') return COMMAND_OK;
        result = parse_integer(&cursor, &command->player_id, 1);
        if (result != COMMAND_OK) return result;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "robot")) {
        command->type = COMMAND_ROBOT;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "step")) {
        command->type = COMMAND_STEP;
        cursor = skip_spaces(cursor);
        result = parse_integer(&cursor, &command->steps, 0);
        if (result != COMMAND_OK) return result;
        if (command->steps < 0) return COMMAND_OUT_OF_RANGE;
        return *skip_spaces(cursor) == '\0' ? COMMAND_OK : COMMAND_EXTRA_ARGUMENT;
    }
    if (word_equals(word_start, word_length, "sell")) command->type = COMMAND_SELL;
    else if (word_equals(word_start, word_length, "block")) command->type = COMMAND_BLOCK;
    else return COMMAND_UNKNOWN;

    return parse_one_argument(&cursor, &command->argument);
}

const char *Command_Result_Message(CommandResult result)
{
    switch (result) {
    case COMMAND_UNKNOWN: return "Error: unknown command. Use help for commands.";
    case COMMAND_MISSING_ARGUMENT: return "Error: a required argument is missing.";
    case COMMAND_INVALID_NUMBER: return "Error: arguments must be integers.";
    case COMMAND_OUT_OF_RANGE: return "Error: numeric argument is out of range.";
    case COMMAND_EXTRA_ARGUMENT: return "Error: too many command arguments.";
    case COMMAND_INVALID_ARGUMENT: return "Error: invalid command input.";
    default: return "Error: command failed.";
    }
}

const char *Command_Type_Name(CommandType type)
{
    switch (type) {
    case COMMAND_STEP: return "step";
    case COMMAND_ROLL: return "roll";
    case COMMAND_QUERY: return "query";
    case COMMAND_SELL: return "sell";
    case COMMAND_BLOCK: return "block";
    case COMMAND_ROBOT: return "robot";
    case COMMAND_RESET: return "reset";
    case COMMAND_HELP: return "help";
    case COMMAND_QUIT: return "q";
    default: return "invalid";
    }
}
