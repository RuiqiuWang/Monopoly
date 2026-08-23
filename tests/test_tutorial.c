#include <assert.h>
#include <stdio.h>

#include "tutorial.h"

int main(void)
{
    const char *path = "tutorial_test_state.json";
    TutorialState state = {false};
    TutorialState loaded = {false};

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
