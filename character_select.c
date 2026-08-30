#include "character_select.h"

#include "input.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char role_from_digit(char digit)
{
    switch (digit) {
    case '1': return 'Q';
    case '2': return 'A';
    case '3': return 'S';
    case '4': return 'J';
    default: return '\0';
    }
}

void CharacterSelect_Init(CharacterSelection *selection)
{
    if (selection == NULL) return;
    selection->chosen_count = 0;
    memset(selection->chosen, 0, sizeof(selection->chosen));
}

static int is_space_char(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

CharacterSelectResult CharacterSelect_ParseRole(const char *input, char *role)
{
    const char *cursor;
    const char *end;
    char value;
    if (input == NULL || role == NULL) return CHARACTER_SELECT_INVALID_ARGUMENT;
    cursor = input;
    while (*cursor != '\0' && is_space_char(*cursor)) ++cursor;
    end = cursor + strlen(cursor);
    while (end > cursor && is_space_char(end[-1])) --end;
    if (end - cursor != 1) return CHARACTER_SELECT_INVALID_INPUT;
    value = role_from_digit(*cursor);
    if (value == '\0') return CHARACTER_SELECT_INVALID_INPUT;
    *role = value;
    return CHARACTER_SELECT_OK;
}

static int is_valid_role(char role)
{
    return role == 'Q' || role == 'A' || role == 'S' || role == 'J';
}

CharacterSelectResult CharacterSelect_ChooseRole(CharacterSelection *selection, char role)
{
    char normalized;
    if (selection == NULL) return CHARACTER_SELECT_INVALID_ARGUMENT;
    normalized = (char)toupper((unsigned char)role);
    if (!is_valid_role(normalized)) return CHARACTER_SELECT_INVALID_INPUT;
    if (selection->chosen_count >= CHARACTER_SELECT_MAX_PLAYERS) {
        return CHARACTER_SELECT_SELECTION_FULL;
    }
    for (int i = 0; i < selection->chosen_count; ++i) {
        if (selection->chosen[i] == normalized) return CHARACTER_SELECT_DUPLICATE_ROLE;
    }
    selection->chosen[selection->chosen_count++] = normalized;
    return CHARACTER_SELECT_OK;
}

CharacterSelectResult CharacterSelect_ApplyInput(CharacterSelection *selection, const char *input)
{
    CharacterSelection copy;
    if (selection == NULL || input == NULL) return CHARACTER_SELECT_INVALID_ARGUMENT;
    copy = *selection;
    for (const char *cursor = input; *cursor != '\0'; ++cursor) {
        char role;
        if (is_space_char(*cursor)) continue;
        role = role_from_digit(*cursor);
        if (role == '\0') return CHARACTER_SELECT_INVALID_INPUT;
        if (CharacterSelect_ChooseRole(&copy, role) != CHARACTER_SELECT_OK) {
            for (int i = 0; i < copy.chosen_count; ++i) {
                if (copy.chosen[i] == role) return CHARACTER_SELECT_DUPLICATE_ROLE;
            }
            return CHARACTER_SELECT_SELECTION_FULL;
        }
    }
    *selection = copy;
    return CHARACTER_SELECT_OK;
}

const char *CharacterSelect_RoleName(char role)
{
    switch (toupper((unsigned char)role)) {
    case 'Q': return "钱夫人";
    case 'A': return "阿土伯";
    case 'S': return "孙小美";
    case 'J': return "金贝贝";
    default: return "";
    }
}

PlayerColor CharacterSelect_RoleColor(char role)
{
    switch (toupper((unsigned char)role)) {
    case 'Q': return COLOR_RED;
    case 'A': return COLOR_GREEN;
    case 'S': return COLOR_BLUE;
    case 'J': return COLOR_YELLOW;
    default: return COLOR_GREEN;
    }
}

const char *CharacterSelect_ResultMessage(CharacterSelectResult result)
{
    switch (result) {
    case CHARACTER_SELECT_OK: return "Selection accepted.";
    case CHARACTER_SELECT_INVALID_ARGUMENT: return "Invalid selection argument.";
    case CHARACTER_SELECT_INVALID_INPUT: return "Choose characters 1, 2, 3, or 4.";
    case CHARACTER_SELECT_DUPLICATE_ROLE: return "That character is already selected.";
    case CHARACTER_SELECT_SELECTION_FULL: return "Four characters are already selected.";
    default: return "Unknown selection error.";
    }
}

bool CharacterSelect_Prompt(CharacterSelection *selection)
{
    char input[64];
    if (selection == NULL) return false;
    CharacterSelect_Init(selection);
    puts("Choose 2-4 characters: 1=Q, 2=A, 3=S, 4=J.");
    puts("Enter all desired character numbers on one line, for example 12 or 1324.");
    while (selection->chosen_count < CHARACTER_SELECT_MIN_PLAYERS) {
        CharacterSelectResult result;
        if (!input_read_line("Characters> ", input, sizeof(input))) return false;
        result = CharacterSelect_ApplyInput(selection, input);
        if (result != CHARACTER_SELECT_OK) {
            puts(CharacterSelect_ResultMessage(result));
            continue;
        }
        if (selection->chosen_count < CHARACTER_SELECT_MIN_PLAYERS) {
            puts("Select at least two different characters.");
        }
    }
    return true;
}
