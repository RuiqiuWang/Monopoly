#include <stdio.h>
#include <string.h>

#include "movement.h"

static const char *step_error_message(StepParseResult result)
{
    switch (result) {
    case STEP_PARSE_INVALID_CHARACTER:
        return "ERROR invalid character: digits only";
    case STEP_PARSE_INVALID_STEP:
        return "ERROR invalid step: must be greater than 0";
    case STEP_PARSE_OVERFLOW:
        return "ERROR step out of range";
    default:
        return "ERROR invalid input";
    }
}

int main(void)
{
    char input[128];
    Player player = {0};
    int step;
    StepParseResult parse_result;

    player.position = 0;

    while (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\r\n")] = '\0';

        parse_result = Parse_Step(input, &step);
        if (parse_result != STEP_PARSE_OK) {
            puts(step_error_message(parse_result));
            continue;
        }
        if (Move_Player(&player, step) != PLAYER_MOVE_OK) {
            puts("ERROR invalid input");
            continue;
        }

        printf("OK position=%u\n", (unsigned)player.position);
    }

    return 0;
}
