#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "character_select.h"

static void test_parse_role(void)
{
    char role = '\0';
    assert(CharacterSelect_ParseRole("1", &role) == CHARACTER_SELECT_OK && role == 'Q');
    assert(CharacterSelect_ParseRole("2", &role) == CHARACTER_SELECT_OK && role == 'A');
    assert(CharacterSelect_ParseRole("3", &role) == CHARACTER_SELECT_OK && role == 'S');
    assert(CharacterSelect_ParseRole("4", &role) == CHARACTER_SELECT_OK && role == 'J');
    assert(CharacterSelect_ParseRole("5", &role) == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ParseRole("W", &role) == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ParseRole("QWER", &role) == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ParseRole("#", &role) == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ParseRole("", &role) == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ParseRole(NULL, &role) == CHARACTER_SELECT_INVALID_ARGUMENT);
    assert(CharacterSelect_ParseRole("1", NULL) == CHARACTER_SELECT_INVALID_ARGUMENT);
}

static void test_apply_input(void)
{
    CharacterSelection selection;
    const char *inputs[] = {"12", "123", "1234", "1324"};
    const char expected[][4] = {
        {'Q', 'A', 0, 0},
        {'Q', 'A', 'S', 0},
        {'Q', 'A', 'S', 'J'},
        {'Q', 'S', 'A', 'J'}
    };
    const int counts[] = {2, 3, 4, 4};

    for (size_t test = 0; test < sizeof(inputs) / sizeof(inputs[0]); ++test) {
        CharacterSelect_Init(&selection);
        assert(CharacterSelect_ApplyInput(&selection, inputs[test]) == CHARACTER_SELECT_OK);
        assert(selection.chosen_count == counts[test]);
        assert(memcmp(selection.chosen, expected[test], (size_t)counts[test]) == 0);
    }

    CharacterSelect_Init(&selection);
    assert(CharacterSelect_ApplyInput(&selection, "11") == CHARACTER_SELECT_DUPLICATE_ROLE);
    assert(selection.chosen_count == 0);
    assert(CharacterSelect_ApplyInput(&selection, "1") == CHARACTER_SELECT_OK);
    assert(CharacterSelect_ApplyInput(&selection, "1") == CHARACTER_SELECT_DUPLICATE_ROLE);
    assert(selection.chosen_count == 1 && selection.chosen[0] == 'Q');
    assert(CharacterSelect_ApplyInput(&selection, "2") == CHARACTER_SELECT_OK);
    assert(selection.chosen_count == 2 && selection.chosen[1] == 'A');

    CharacterSelect_Init(&selection);
    assert(CharacterSelect_ApplyInput(&selection, "5") == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ApplyInput(&selection, "W") == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ApplyInput(&selection, "QWER") == CHARACTER_SELECT_INVALID_INPUT);
    assert(CharacterSelect_ApplyInput(&selection, "#") == CHARACTER_SELECT_INVALID_INPUT);
    assert(selection.chosen_count == 0);
    assert(CharacterSelect_ApplyInput(&selection, NULL) == CHARACTER_SELECT_INVALID_ARGUMENT);
    assert(CharacterSelect_ApplyInput(NULL, "1") == CHARACTER_SELECT_INVALID_ARGUMENT);
}

static void test_role_lookup(void)
{
    assert(strcmp(CharacterSelect_RoleName('Q'), "钱夫人") == 0);
    assert(strcmp(CharacterSelect_RoleName('A'), "阿土伯") == 0);
    assert(strcmp(CharacterSelect_RoleName('S'), "孙小美") == 0);
    assert(strcmp(CharacterSelect_RoleName('J'), "金贝贝") == 0);
    assert(strcmp(CharacterSelect_RoleName('W'), "") == 0);
    assert(CharacterSelect_RoleColor('Q') == COLOR_RED);
    assert(CharacterSelect_RoleColor('A') == COLOR_GREEN);
    assert(CharacterSelect_RoleColor('S') == COLOR_BLUE);
    assert(CharacterSelect_RoleColor('J') == COLOR_YELLOW);
}

int main(void)
{
    test_parse_role();
    test_apply_input();
    test_role_lookup();
    return 0;
}
