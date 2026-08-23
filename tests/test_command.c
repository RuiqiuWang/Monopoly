#include <assert.h>
#include <string.h>
#include "command.h"

int main(void)
{
    Command command;
    assert(Parse_Command("step 1 5", &command) == COMMAND_OK);
    assert(command.type == COMMAND_STEP && command.player_id == 1 && command.steps == 5);
    assert(Parse_Command("STEP 2 70", &command) == COMMAND_OK);
    assert(Parse_Command("quit", &command) == COMMAND_OK && command.type == COMMAND_QUIT);
    assert(Parse_Command("step", &command) == COMMAND_MISSING_ARGUMENT);
    assert(Parse_Command("step 0 3", &command) == COMMAND_OUT_OF_RANGE);
    assert(Parse_Command("step 1 -3", &command) == COMMAND_INVALID_NUMBER);
    assert(Parse_Command("step 1 3 extra", &command) == COMMAND_EXTRA_ARGUMENT);
    assert(Parse_Command("wada 1 2", &command) == COMMAND_UNKNOWN);
    assert(Parse_Command(NULL, &command) == COMMAND_INVALID_ARGUMENT);
    assert(Parse_Command("step 1 3", NULL) == COMMAND_INVALID_ARGUMENT);
    assert(strcmp(Command_Result_Message(COMMAND_UNKNOWN), "错误：未知命令，请使用 step [id] [number]") == 0);
    return 0;
}
