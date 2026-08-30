#ifndef MONOPOLY_CHARACTER_SELECT_H
#define MONOPOLY_CHARACTER_SELECT_H

#include <stdbool.h>

#include "player.h"

#define CHARACTER_SELECT_MIN_PLAYERS 2
#define CHARACTER_SELECT_MAX_PLAYERS 4

typedef enum {
    CHARACTER_SELECT_OK = 0,
    CHARACTER_SELECT_INVALID_ARGUMENT = -1,
    CHARACTER_SELECT_INVALID_INPUT = -2,
    CHARACTER_SELECT_DUPLICATE_ROLE = -3,
    CHARACTER_SELECT_SELECTION_FULL = -4
} CharacterSelectResult;

typedef struct {
    int chosen_count;
    char chosen[CHARACTER_SELECT_MAX_PLAYERS];
} CharacterSelection;

void CharacterSelect_Init(CharacterSelection *selection);
CharacterSelectResult CharacterSelect_ParseRole(const char *input, char *role);
CharacterSelectResult CharacterSelect_ChooseRole(CharacterSelection *selection, char role);
CharacterSelectResult CharacterSelect_ApplyInput(CharacterSelection *selection, const char *input);
const char *CharacterSelect_RoleName(char role);
PlayerColor CharacterSelect_RoleColor(char role);
const char *CharacterSelect_ResultMessage(CharacterSelectResult result);

/* Interactive selection using the shared line-input interface. */
bool CharacterSelect_Prompt(CharacterSelection *selection);

#endif
