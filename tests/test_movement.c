#include <assert.h>
#include <stddef.h>

#include "movement.h"

int main(void)
{
    Player player = {0};

    player.id = 1;
    player.position = 0;
    assert(Move_Player(&player, 5) == PLAYER_MOVE_OK);
    assert(player.position == 5);

    player.position = 69;
    assert(Move_Player(&player, 1) == PLAYER_MOVE_OK);
    assert(player.position == 0);

    player.position = 3;
    assert(Move_Player(&player, 70 + 8) == PLAYER_MOVE_OK);
    assert(player.position == 11);

    player.position = 10;
    assert(Move_Player(&player, -1) == PLAYER_MOVE_INVALID_STEP);
    assert(player.position == 10);
    assert(Move_Player(&player, 0) == PLAYER_MOVE_INVALID_STEP);
    assert(player.position == 10);

    player.position = 70;
    assert(Move_Player(&player, 1) == PLAYER_MOVE_INVALID_POSITION);
    assert(player.position == 70);

    assert(Move_Player(NULL, 1) == PLAYER_MOVE_INVALID_ARGUMENT);

    {
        const char *valid[] = {"1", "70", "2147483647"};
        const char *invalid[] = {"", "0", "-1", "1.5", "125abd", "wada", "90|", "2147483648", "999999999999999999999999"};
        int step;
        size_t i;

        for (i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
            assert(Parse_Step(valid[i], &step) == STEP_PARSE_OK);
            assert(step > 0);
        }
        for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            assert(Parse_Step(invalid[i], &step) != STEP_PARSE_OK);
        }
        assert(Parse_Step("125abd", &step) == STEP_PARSE_INVALID_CHARACTER);
        assert(Parse_Step("0", &step) == STEP_PARSE_INVALID_STEP);
        assert(Parse_Step("2147483648", &step) == STEP_PARSE_OVERFLOW);
        assert(Parse_Step(NULL, &step) == STEP_PARSE_INVALID_ARGUMENT);
        assert(Parse_Step("1", NULL) == STEP_PARSE_INVALID_ARGUMENT);
    }

    return 0;
}
