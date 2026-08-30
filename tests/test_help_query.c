#include <assert.h>
#include <string.h>

#include "help_query.h"

int main(void)
{
    const char *text = HelpQuery_Text();
    assert(text != NULL);
    assert(strstr(text, "step <steps>") != NULL);
    assert(strstr(text, "gift house") != NULL);
    assert(strstr(text, "hospital for three turns") != NULL);
    assert(strstr(text, "70 blocks (0-69)") != NULL);
    assert(strstr(text, "rent is half the total investment") != NULL);
    assert(strstr(text, "prison skips the player's next two turns") != NULL);
    return 0;
}
