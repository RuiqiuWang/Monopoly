#include <assert.h>
#include <string.h>

#include "command.h"

int main(void)
{
    Command command;
    assert(Parse_Command("step 5", &command) == COMMAND_OK);
    assert(command.type == COMMAND_STEP && command.steps == 5);
    assert(Parse_Command("STEP 70", &command) == COMMAND_OK);
    assert(Parse_Command("quit", &command) == COMMAND_OK && command.type == COMMAND_QUIT);
    assert(Parse_Command("q", &command) == COMMAND_UNKNOWN);
    assert(Parse_Command("step", &command) == COMMAND_MISSING_ARGUMENT);
    assert(Parse_Command("step 0", &command) == COMMAND_OK && command.steps == 0);
    assert(Parse_Command("step -3", &command) == COMMAND_OUT_OF_RANGE);
    assert(Parse_Command("step 1 3", &command) == COMMAND_EXTRA_ARGUMENT);
    assert(Parse_Command("wada 1 2", &command) == COMMAND_UNKNOWN);
    assert(Parse_Command("roll", &command) == COMMAND_OK && command.type == COMMAND_ROLL);
    assert(Parse_Command("query", &command) == COMMAND_OK && command.type == COMMAND_QUERY);
    assert(Parse_Command("query 2", &command) == COMMAND_EXTRA_ARGUMENT);
    assert(Parse_Command("block -2", &command) == COMMAND_OK && command.argument == -2);
    assert(Parse_Command("bomb 3", &command) == COMMAND_UNKNOWN);
    assert(Parse_Command("robot", &command) == COMMAND_OK && command.type == COMMAND_ROBOT);
    assert(Parse_Command("reset", &command) == COMMAND_OK && command.type == COMMAND_RESET);
    assert(Parse_Command("reset now", &command) == COMMAND_EXTRA_ARGUMENT);
    assert(Parse_Command(NULL, &command) == COMMAND_INVALID_ARGUMENT);
    assert(Parse_Command("step 3", NULL) == COMMAND_INVALID_ARGUMENT);
    assert(strcmp(Command_Result_Message(COMMAND_UNKNOWN),
                  "Error: unknown command. Use help for commands.") == 0);
    return 0;
}
