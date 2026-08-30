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
    return 0;
}
