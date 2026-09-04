#ifndef MONOPOLY_COMMAND_H
#define MONOPOLY_COMMAND_H

typedef enum {
    COMMAND_OK = 0,
    COMMAND_INVALID_ARGUMENT = -1,
    COMMAND_UNKNOWN = -2,
    COMMAND_MISSING_ARGUMENT = -3,
    COMMAND_INVALID_NUMBER = -4,
    COMMAND_OUT_OF_RANGE = -5,
    COMMAND_EXTRA_ARGUMENT = -6
} CommandResult;

typedef enum {
    COMMAND_STEP,
    COMMAND_ROLL,
    COMMAND_QUERY,
    COMMAND_SELL,
    COMMAND_BLOCK,
    COMMAND_ROBOT,
    COMMAND_RESET,
    COMMAND_HELP,
    COMMAND_QUIT,
    COMMAND_INVALID
} CommandType;

typedef struct {
    CommandType type;
    int steps;
    int argument;
} Command;

CommandResult Parse_Command(const char *input, Command *command);
const char *Command_Result_Message(CommandResult result);
const char *Command_Type_Name(CommandType type);

#endif
