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
    case CHARACTER_SELECT_INVALID_ARGUMENT: return "选择失败：参数无效。";
    case CHARACTER_SELECT_INVALID_INPUT: return "选择失败：请输入1、2、3或4。";
    case CHARACTER_SELECT_DUPLICATE_ROLE: return "选择失败：该角色已经被选择。";
    case CHARACTER_SELECT_SELECTION_FULL: return "选择失败：最多只能选择四名角色。";
    default: return "选择失败：未知错误。";
    }
}

static void print_selection_instructions(void)
{
    puts("请选择2-4名角色：1=钱夫人(Q)，2=阿土伯(A)，3=孙小美(S)，4=金贝贝(J)。");
    puts("请在一行输入角色编号，例如12或1324。");
    /* Keep the command-line wording recognizable to older automation while
     * presenting the actual instructions in Chinese. */
    puts("(Choose 2-4 characters: 1=Q, 2=A, 3=S, 4=J.)");
    puts("(Enter all desired character numbers on one line, for example 12 or 1324.)");
}

bool CharacterSelect_Prompt(CharacterSelection *selection)
{
    char input[64];
    if (selection == NULL) return false;
    CharacterSelect_Init(selection);
    print_selection_instructions();
    while (selection->chosen_count < CHARACTER_SELECT_MIN_PLAYERS) {
        CharacterSelectResult result;
        if (!input_read_line("Characters> ", input, sizeof(input))) return false;
        result = CharacterSelect_ApplyInput(selection, input);
        if (result != CHARACTER_SELECT_OK) {
            input_clear_screen();
            puts("输入无效");
            puts(CharacterSelect_ResultMessage(result));
            print_selection_instructions();
            continue;
        }
        if (selection->chosen_count < CHARACTER_SELECT_MIN_PLAYERS) {
            input_clear_screen();
            puts("输入无效");
            puts("至少选择两名不同的角色。");
            print_selection_instructions();
        }
    }
    return true;
}
