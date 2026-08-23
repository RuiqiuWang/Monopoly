#include <assert.h>
#include <stdio.h>

#include "tutorial.h"

int main(void)
{
    const char *path = "tutorial_test_state.json";
    TutorialState state = {false};
    TutorialState loaded = {false};

    assert(tutorial_parse_choice("Y") == TUTORIAL_CHOICE_YES);
    assert(tutorial_parse_choice("y\n") == TUTORIAL_CHOICE_YES);
    assert(tutorial_parse_choice("N") == TUTORIAL_CHOICE_NO);
    assert(tutorial_parse_choice("n\r\n") == TUTORIAL_CHOICE_NO);
    assert(tutorial_parse_choice("X") == TUTORIAL_CHOICE_INVALID);
    assert(tutorial_parse_choice("YES") == TUTORIAL_CHOICE_INVALID);
    assert(tutorial_parse_choice("") == TUTORIAL_CHOICE_INVALID);
    assert(tutorial_parse_choice(NULL) == TUTORIAL_CHOICE_INVALID);

    assert(tutorial_state_save(&state, path));
    assert(tutorial_state_load(&loaded, path));
    assert(!loaded.has_run);

    state.has_run = true;
    assert(tutorial_state_save(&state, path));
    loaded.has_run = false;
    assert(tutorial_state_load(&loaded, path));
    assert(loaded.has_run);

    remove(path);
    return 0;
}
