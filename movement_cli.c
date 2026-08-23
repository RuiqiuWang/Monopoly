#include <stdio.h>
#include <string.h>

#include "movement.h"

int main(void)
{
    char input[128];
    Player player = {0};
    int step;

    player.position = 0;

    while (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\r\n")] = '\0';

        if (Parse_Step(input, &step) != STEP_PARSE_OK ||
            Move_Player(&player, step) != PLAYER_MOVE_OK) {
            puts("ERROR invalid input");
            continue;
        }

        printf("OK position=%u\n", (unsigned)player.position);
    }

    return 0;
}
