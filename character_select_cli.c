#include <stdio.h>

#include "character_select.h"

int main(void)
{
    CharacterSelection selection;
    if (!CharacterSelect_Prompt(&selection)) return 1;
    printf("Selected %d characters.\n", selection.chosen_count);
    return 0;
}
