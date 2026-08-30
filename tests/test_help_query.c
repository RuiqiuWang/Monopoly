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
    assert(strstr(text, "Rent is base price") != NULL);
    return 0;
}
