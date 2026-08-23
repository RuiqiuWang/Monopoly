#include "command.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

static const char *skip_spaces(const char *cursor)
{
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) ++cursor;
    return cursor;
}

static int word_equals(const char *start, size_t length, const char *word)
{
    size_t word_length = strlen(word);
    if (length != word_length) return 0;
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)start[i]) != tolower((unsigned char)word[i])) return 0;
    }
    return 1;
}

static CommandResult parse_positive_integer(const char **cursor, int *value)
{
    long parsed = 0;
    if (**cursor == '\0') return COMMAND_MISSING_ARGUMENT;
    if (!isdigit((unsigned char)**cursor)) return COMMAND_INVALID_NUMBER;
    while (isdigit((unsigned char)**cursor)) {
        int digit = **cursor - '0';
        if (parsed > (LONG_MAX - digit) / 10) return COMMAND_INVALID_NUMBER;
        parsed = parsed * 10 + digit;
        ++*cursor;
    }
    if (parsed <= 0 || parsed > INT_MAX) return COMMAND_OUT_OF_RANGE;
    *value = (int)parsed;
    return COMMAND_OK;
}

CommandResult Parse_Command(const char *input, Command *command)
{
    const char *cursor;
    const char *word_start;
    size_t word_length;
    CommandResult result;

    if (input == NULL || command == NULL) return COMMAND_INVALID_ARGUMENT;
    command->type = COMMAND_STEP; command->player_id = 0; command->steps = 0;
    cursor = skip_spaces(input); word_start = cursor;
    while (*cursor != '\0' && !isspace((unsigned char)*cursor)) ++cursor;
    word_length = (size_t)(cursor - word_start);
    if (word_equals(word_start, word_length, "quit")) {
        if (*skip_spaces(cursor) != '\0') return COMMAND_EXTRA_ARGUMENT;
        command->type = COMMAND_QUIT; return COMMAND_OK;
    }
    if (!word_equals(word_start, word_length, "step")) return COMMAND_UNKNOWN;
    cursor = skip_spaces(cursor);
    result = parse_positive_integer(&cursor, &command->player_id);
    if (result != COMMAND_OK) return result;
    cursor = skip_spaces(cursor);
    result = parse_positive_integer(&cursor, &command->steps);
    if (result != COMMAND_OK) return result;
    if (*skip_spaces(cursor) != '\0') return COMMAND_EXTRA_ARGUMENT;
    return COMMAND_OK;
}

const char *Command_Result_Message(CommandResult result)
{
    switch (result) {
    case COMMAND_UNKNOWN: return "错误：未知命令，请使用 step [id] [number]";
    case COMMAND_MISSING_ARGUMENT: return "错误：step 需要玩家 id 和正整数步数";
    case COMMAND_INVALID_NUMBER: return "错误：id 和 number 必须是纯数字";
    case COMMAND_OUT_OF_RANGE: return "错误：id 和 number 必须大于 0 且不超出 int 范围";
    case COMMAND_EXTRA_ARGUMENT: return "错误：命令参数过多";
    default: return "错误：无效输入";
    }
}
