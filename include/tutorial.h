#ifndef MONOPOLY_TUTORIAL_H
#define MONOPOLY_TUTORIAL_H

#include <stdbool.h>

#include "map.h"

#define MONOPOLY_STATE_FILE ".monopoly_state.json"

typedef enum {
    TUTORIAL_CHOICE_INVALID = 0,
    TUTORIAL_CHOICE_YES = 1,
    TUTORIAL_CHOICE_NO = 2
} TutorialChoice;

typedef struct {
    bool has_run;
} TutorialState;

bool tutorial_state_load(TutorialState *state, const char *path);
bool tutorial_state_save(const TutorialState *state, const char *path);
TutorialChoice tutorial_parse_choice(const char *input);
bool tutorial_prompt_first_run(void);
void tutorial_run(const Map *map);

#endif
